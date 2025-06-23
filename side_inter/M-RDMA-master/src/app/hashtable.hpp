/*
 * TODO
 * 1. multi-version hashtable, circular entries
 * 2. exploit CRC validation code to handle conflict
 * 3. add read/write circular buffer
 */
#ifndef HASHTABLE_HPP

#define HASHTABLE_HPP

#include <functional>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <thread>

extern "C" {
#include "src/libmrdma/libmrdma.h"
#include "src/libmrdma/spin_lock.h"
#include "src/libmrdma/barrier.h"
}

#include "src/tool/CRC.h"
#include "src/libmrdma/wrap_rdma.hpp"
#include "src/tool/random_gen.hpp"
#include "src/tool/zipf_gen.hpp"
#include "src/app/kv.hpp"


namespace hashtable{
#define USE_CACHE
#define USE_ZIPF

/* ZIPF Config */
#ifdef USE_ZIPF
const double zipf_alpha = 0.99;
#endif

/* Back End */
const long long meta_area_size = 1 << 5;
const long long meta_area_offset = 0;
const long long data_area_size = 1 << 25;
const long long data_area_offset = meta_area_offset + meta_area_size;
const long long max_reg_mem_size_back = data_area_size + meta_area_size;

/* Front End */
const long long cache_size = 1 << 20;
const long long cache_offset = meta_area_offset + meta_area_size;
const long long write_buffer_size = 1 << 12;
const long long write_buffer_offset = cache_offset + cache_size;
const long long read_buffer_size = 1 << 12;
const long long read_buffer_offset = write_buffer_offset + write_buffer_size;
const long long max_reg_mem_size_front = meta_area_size + cache_size + write_buffer_size + read_buffer_size;

/* Config */
const int max_front_num = 8;
const int max_iter_num = 10000000;
const int max_hashtable_len = 1 << 17;

/* Re-order */
const int max_hot_ratio = 1 << 5;
const int max_hashtable_hot_len = max_hashtable_len / max_hot_ratio;
const int max_batch_size = 1 << 4;
const int max_burst_entry_len = max_hashtable_hot_len / max_batch_size;
const int max_client_machine_num = 7;
// const int hot_data_area_size = 1 << 19;
// const int hot_data_area_offset = data_area_offset;
// const int cold_data_area_size = data_area_size - hot_data_area_size;
// const int cold_data_area_offset = data_area_offset + hot_data_area_size;
const int burst_entry_size = kv_size * max_batch_size;

static_assert(max_hashtable_hot_len * kv_size < cache_size, "Small Cache Size");

// static_assert(cache_size >= max_hashtable_hot_len * kv_size);

int burst_entry_counter[max_burst_entry_len];

uint64_t no_hash(uint64_t key) {
    return static_cast<uint64_t>(key % max_hashtable_len);
}

bool in_hot_data_area(uint64_t index) {
    return index < max_hashtable_hot_len;
}

forceinline uint64_t get_cache_offset(uint64_t index) {
    return cache_offset + index * kv_size;
}

forceinline uint64_t get_burst_entry_index(uint64_t index) {
    return index / max_batch_size;
}

forceinline std::pair<uint64_t, uint64_t> get_burst_entry_offset(uint64_t index) {
    uint64_t offset = index * max_batch_size * kv_size;
    return std::make_pair(data_area_offset + offset, cache_offset + offset);
}

forceinline uint64_t get_kv_offset(uint64_t index) {
    return index * kv_size;
}

class HashTable
{
public:
    /* port num */
    char *reg_mem;
    int port_num;
    int front_num;
    KV *hashtable_data;

    HashTable(int port_num_, int front_num_): port_num(port_num_), front_num(front_num_) {

    };
    ~HashTable() {}

    KV *get_kv(uint64_t key);
    void put_kv(KV *kv);

    KV *cuckoo_get_kv(uint64_t key);
    void cuckoo_put_kv(KV *kv);
};

class HashTableFront:HashTable
{
private:
    /* data */
    char *cache_mem;
    char *write_buffer;
    char *read_buffer;

    char *server_name;

    /* RDMA */
    WrapRdma *wrap_rdma;
    m_ibv_res *ibv_res;
    std::function<uint64_t(uint64_t)> hash_func;

    /* Random Gen */
    RandomGen *rand_gen;

    /* Batch */
    int max_counter_size;

    /* Workload */
    int workload_ratio;

    forceinline char *get_cache_addr(uint64_t index) {
        return cache_mem + index * kv_size;
    }

    forceinline char *OFFSET2ADDR(uint64_t offset) {
        return reg_mem + offset;
    }

public:
    HashTableFront(int port_num_, int front_num_, int max_counter_size_, char *server_name_): 
        HashTable(port_num_, front_num_), server_name(server_name_), max_counter_size(max_counter_size_) {

        init_all();

        small_test();
        general_test();
    };
    ~HashTableFront() = delete;

    void init_all() {

        char* env_var = getenv("workload_ratio");
        if (env_var != NULL){
            workload_ratio = atoi(env_var);
        } else {
            workload_ratio = 1;
        }

        printf("[Workload Ratio]%d\n", workload_ratio);

        reg_mem = new char[max_reg_mem_size_front];

        cache_mem = reg_mem + cache_offset;
        write_buffer = reg_mem + write_buffer_offset;
        read_buffer = reg_mem + read_buffer_offset;

        rand_gen = new RandomGen(max_iter_num + 200);

        wrap_rdma = new WrapRdma(1, false);
        wrap_rdma->establish_conn_async(reg_mem, max_reg_mem_size_front, port_num, server_name);
        ibv_res = wrap_rdma->ibv_res;

        hash_func = no_hash;
    }

    KV *get_kv(uint64_t key) {
        uint64_t index = hash_func(key);
        uint64_t offset = get_kv_offset(index);

#ifdef USE_UNSIG
        wrap_rdma->wrap_post_read_offset(read_buffer, kv_size, offset, 0);
#else
        wrap_rdma->wrap_post_read_offset_force(read_buffer, kv_size, offset, 0);
#endif
        return reinterpret_cast<KV *>(read_buffer);
           
    }

    void put_kv_burst(KV *kv) {
        uint64_t index = hash_func(kv->key);
        if (in_hot_data_area(index)) {

            char *dst_addr = get_cache_addr(index);

            std::memcpy(dst_addr, (char *)kv, kv_size);

            uint64_t burst_index = get_burst_entry_index(index);

            burst_entry_counter[burst_index] ++; 

            if (burst_entry_counter[burst_index] == max_counter_size) {

                auto offset = get_burst_entry_offset(burst_index);
#ifdef USE_UNSIG
                wrap_rdma->wrap_post_write_offset(OFFSET2ADDR(offset.second), burst_entry_size, offset.first, 0);
#else
                wrap_rdma->wrap_post_write_offset_force(OFFSET2ADDR(offset.second), burst_entry_size, offset.first, 0);
#endif
                burst_entry_counter[burst_index] = 0; 
            }

        } else {

            KV *kv_buffer = reinterpret_cast<KV *>(write_buffer);

            kv_buffer->key = kv->key;
            std::memcpy(kv_buffer->value, kv->value, max_value_size);

            uint64_t offset = get_kv_offset(index);

#ifdef USE_UNSIG
            wrap_rdma->wrap_post_write_offset(write_buffer, kv_size, offset, 0);
#else
            wrap_rdma->wrap_post_write_offset_force(write_buffer, kv_size, offset, 0);
#endif
        }

    }

    void put_kv(KV *kv) {
        uint64_t index = hash_func(kv->key);
        uint64_t offset = get_kv_offset(index);

        KV *kv_buffer = reinterpret_cast<KV *>(write_buffer);
    
        kv_buffer->key = kv->key;
        std::memcpy(kv_buffer->value, kv->value, max_value_size);

#ifdef USE_UNSIG
        wrap_rdma->wrap_post_write_offset(read_buffer, kv_size, offset, 0);        
#else
        wrap_rdma->wrap_post_write_offset_force(read_buffer, kv_size, offset, 0);        
#endif
    }

    void small_test() {
        
        m_client_sync(wrap_rdma->ibv_res, reg_mem + meta_area_offset, 0, front_num);
        LOG("Everything is OK~\n");
    }

    void general_test() {

#ifdef USE_ZIPF
        ZipfGen *zipf = new ZipfGen(zipf_alpha, max_hashtable_len);
#else
        rand_gen->gen_shuffle_req_data(max_iter_num + 100);
#endif

		auto begin = std::chrono::system_clock::now();

        for(int iter = 0; iter <= max_iter_num; iter ++) {
#ifdef USE_ZIPF
            uint64_t get_num = zipf->next_long();

            // if (iter % 1000000 == 0) {
            //     printf("[%d]%d\n", port_num, iter);
            // }
            // printf("%ld\n", get_num);
#else
            uint64_t get_num = rand_gen->get_shuffle_req_num_inf();
#endif
            KV kv(get_num, nullptr);
#ifdef USE_CACHE
            if (iter % workload_ratio == 0) {
                put_kv_burst(&kv);
            } else {
                get_kv(kv.key);
            }
#else
            put_kv(&kv);
#endif
        }

		auto end = std::chrono::system_clock::now();
		std::chrono::duration<double> diff = end-begin;
		printf("[TIME]%lf\n", diff.count());
		printf("[TPUT]%lf\n", max_iter_num / diff.count());
    }

};

class HashTableBack:HashTable
{
private:
    /* data */
    char *data_mem;
    char *meta_mem;

    /* Remote */
    char *r_cache_mem;

    /* RDMA */
    WrapRdma *wrap_rdma_list[max_front_num];

public:
    HashTableBack(int port_num_, int front_num_): HashTable(port_num_, front_num_) {
        init_all();

        printf("OK~Sleep...zzzz\n");

        sleep(40);
        if (front_num > 7) {
            sleep(40);
        }
        if (front_num > 14) {
            sleep(30);
        }
    };
    ~HashTableBack() = delete;

    void init_all() {

        reg_mem = new char[max_reg_mem_size_back];

        data_mem = reg_mem + data_area_offset;
        meta_mem = reg_mem + meta_area_offset;        

        char *null_server = nullptr;

        std::vector<std::thread> th_vec;

        for (int i = 0; i < front_num; i ++ ) {
#ifdef USE_ASYN_CONN
            th_vec.push_back(std::thread([&](int i) {
                wrap_rdma_list[i] = new WrapRdma(1, true);
                wrap_rdma_list[i]->establish_conn_async(reg_mem, max_reg_mem_size_back, port_num + i, null_server); 
            }, i));
#else
            wrap_rdma_list[i] = new WrapRdma(1, true);
            if (i > max_client_machine_num) {
                wrap_rdma_list[i]->replica_ctx(wrap_rdma_list[i % max_client_machine_num]);
            }
            wrap_rdma_list[i]->establish_conn_async(reg_mem, max_reg_mem_size_back, port_num + i, null_server); 
#endif     
        }

#ifdef USE_ASYN_CONN
        for (auto &th: th_vec) {
            th.join();
        }
#endif

    }
/*
    void init_all() {

        reg_mem = new char[max_reg_mem_size_back];

        data_mem = reg_mem + data_area_offset;
        meta_mem = reg_mem + meta_area_offset;        

        char *null_server = nullptr;

        for (int i = 0; i < front_num; i ++ ) {
            wrap_rdma_list[i] = new WrapRdma(1, true);
            wrap_rdma_list[i]->establish_conn(reg_mem, max_reg_mem_size_back, port_num + i, null_server); 
        }

    }
*/
};

}

#endif