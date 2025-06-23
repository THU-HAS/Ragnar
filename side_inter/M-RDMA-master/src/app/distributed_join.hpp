#if !defined(DISTRIBUTED_JOIN_HPP)
#define DISTRIBUTED_JOIN_HPP

#include <functional>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <thread>
#include <algorithm>
#include <type_traits>
#include <ext/hash_map>
#include <omp.h>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <tbb/concurrent_hash_map.h>

extern "C" {
#include "src/libmrdma/libmrdma.h"
#include "src/libmrdma/spin_lock.h"
#include "src/libmrdma/barrier.h"
}

#include "src/libmrdma/wrap_rdma.hpp"
#include "src/tool/CRC.h"
#include "src/tool/metric.h"
#include "src/tool/random_gen.hpp"
#include "src/tool/zipf_gen.hpp"
#include "src/app/kv.hpp"

#define JOIN_USE_SYNC
// #define JOIN_USE_SGL
#define JOIN_USE_SP

#define JOIN_USE_SIMPLE
#define JOIN_USE_TBB
#define JOIN_USE_OPENMP

namespace join {

const long long meta_area_size = 1 << 21;
const long long meta_area_offset = 0;
const long long data_area_size = (1LL << 30) + (1LL << 30) + (1LL << 29);
const long long data_area_offset = meta_area_offset + meta_area_size;
// const long long max_reg_mem_size_back = data_area_size + meta_area_size;
const long long max_reg_mem_size_join = (1LL << 31) + (1LL << 30);

static_assert(max_reg_mem_size_join > data_area_size + meta_area_size, "data size out of range");

const long long scroll_mem_offset = data_area_offset;
const long long scroll_mem_term_size = 1LL << 29;
const long long max_scroll_mem_term_num = 4;
const long long scroll_mem_size = scroll_mem_term_size * max_scroll_mem_term_num;

const int r_scroll_mem_term_offset = 0;
const int s_scroll_mem_term_offset = 2;

/* Config */
const int max_join_num = 16;
const int max_iter_num = 10000000;
const int max_init_data_size = 1 << 25;
const int data_padding_size = 1 << 14;
const int max_round_num = 10;
const int max_batch_size = 32;
// const int batch_size = 4;

const long long write_mem_buffers_offset = 8 * sizeof(uint64_t);
const long long write_mem_buffers_len = 2;
const long long write_mem_buffers_size = max_batch_size * kv_size;

static_assert(write_mem_buffers_offset + write_mem_buffers_len * write_mem_buffers_size < data_area_offset, "buffer is not enough");
static_assert(scroll_mem_size < data_area_size, "scroll size out of range");

/* Global */
std::random_device global_rd;
uint64_t now_global_rd;

auto rand_distributed_rule = [](uint64_t key, int join_num) -> int {
    return key % join_num;
};

forceinline int rand_distributed_rule_tmp(uint64_t key, int join_num, int src_node) {
    return key % join_num;
};

auto rand_distributed_rule_ex = [](uint64_t key, int join_num, int src_node) -> int {
    return key % join_num;
};

class join_hash_table {
public:
    join_hash_table() {
    }
    ~join_hash_table() = delete;

#if (defined JOIN_USE_TBB)
    tbb::concurrent_hash_map<uint64_t,char*> r_ht;
    void insert(uint64_t key, char *value) {

        tbb::concurrent_hash_map<uint64_t,char*>::accessor ac;
        r_ht.insert(ac, key);
        ac->second = value;
        ac.release();
    }
    bool find(uint64_t key) {

        tbb::concurrent_hash_map<uint64_t,char*>::accessor ac;
        if(r_ht.find(ac, key)) {
            ac.release();
            return false;
        }
        ac.release();
        return true;
    }
#else
    /* Simple Hash Table */
    __gnu_cxx::hash_map<uint64_t, char *> r_ht;
    void insert(uint64_t key, char *value) {
        r_ht[key] = value;
    }
    bool find(uint64_t key) {
        if(r_ht.find(key) == r_ht.end()) {
            return false;
        }
        return true;
    }
#endif
};

class join_node
{
private:
    /* data */
    char *reg_mem;
    char *meta_area_ptr;
    char *write_mem_buffers_ptr;
    char *global_sync_local_counter;
    char *global_sync_faa_buffer;
    char *clear_zero_buffer;

    int src_id;
    int join_num;
    KV *scroll_mem_term_kv_data[max_scroll_mem_term_num];
    KV **per_term_buffer_data;
    uint64_t *per_term_buffer_data_offset;
    uint64_t per_term_buffer_data_size;

    int init_data_size;

    /* RDMA */
    std::vector<WrapRdma *> wrap_rdma_list;
    m_ibv_res **ibv_res_list;
    std::function<uint64_t(uint64_t, int, int)> distributed_rule;

    /* Random Gen */
    RandomGen *rand_gen;
    ZipfGen * zipf;

    /* R hash table */
    join_hash_table *r_ht;

    /* batch */
    int batch_size;

    /* scroll counter */
    int scroll_counter;
    forceinline uint64_t get_scroll_mem_offset()  {
        auto res = scroll_mem_offset + scroll_counter * scroll_mem_term_size;
        return res;
    };

    forceinline int get_scroll_mem_counter() {
        return scroll_counter;
    }

    forceinline void incre_scroll_mem_counter() {
        scroll_counter = (scroll_counter + 1) % max_scroll_mem_term_num;
    }

    /* buffer counter */
    int write_buffer_counter;
    forceinline int get_write_buffer_counter() {
        return write_buffer_counter;
    }

    forceinline int get_write_buffer_offset() {
        return write_buffer_counter * write_mem_buffers_size + write_mem_buffers_offset;
    }

    forceinline void incre_write_buffer_counter() {
        write_buffer_counter = (write_buffer_counter + 1) % write_mem_buffers_len;
    }

public:

    join_node(int src_id_, int join_num_, int batch_size_, std::function<int(uint64_t, int, int)> distributed_rule_ = rand_distributed_rule_ex):
        src_id(src_id_), batch_size(batch_size_), join_num(join_num_), distributed_rule(distributed_rule_) {
        
        assert(batch_size <= max_batch_size);

        assert(join_num <= max_join_num);

        init_all();
    }
    ~join_node() {

    }

    void init_per_term_buffer_data_offset() {
        uint64_t base_offset = get_scroll_mem_offset();
        for (int i = 0; i < join_num; i ++ ) {

            per_term_buffer_data_offset[i] = base_offset + i * per_term_buffer_data_size;
        }
    }

    void init_per_term_buffer_data_offset(uint64_t base_scroll) {

        uint64_t base_offset = scroll_mem_offset + base_scroll * scroll_mem_term_size;
        for (int i = 0; i < join_num; i ++ ) {

            per_term_buffer_data_offset[i] = base_offset + i * per_term_buffer_data_size;
        }
    }

    void init_per_term_buffer_data() {
        uint64_t base_offset = get_scroll_mem_offset();
        for (int i = 0; i < join_num; i ++ ) {
 
            per_term_buffer_data[i] = reinterpret_cast<KV *>(reg_mem + base_offset + i * per_term_buffer_data_size);

            assert((char *)per_term_buffer_data[i] < reg_mem + max_reg_mem_size_join);
        }
    }

    void init_per_term_buffer_data(uint64_t base_scroll) {
        uint64_t base_offset = scroll_mem_offset + base_scroll * scroll_mem_term_size;
        for (int i = 0; i < join_num; i ++ ) {
 
            per_term_buffer_data[i] = reinterpret_cast<KV *>(reg_mem + base_offset + i * per_term_buffer_data_size);

            assert((char *)per_term_buffer_data[i] < reg_mem + max_reg_mem_size_join);
        }
    }

    void init_all() {

        // CPU_init();

        init_data_size = max_init_data_size / join_num / join_num; 

        r_ht = new join_hash_table();

        // reg_mem = new char[max_reg_mem_size_join];
        reg_mem = (char *)malloc(max_reg_mem_size_join);
        printf("%p\n", reg_mem);

        meta_area_ptr = reg_mem + meta_area_offset;
        /* Init Sync Signal */
        global_sync_local_counter = meta_area_ptr;
        global_sync_faa_buffer = meta_area_ptr + sizeof(uint64_t); 

        clear_zero_buffer = meta_area_ptr + 2 * sizeof(uint64_t);
        std::memset(clear_zero_buffer, 0, sizeof(uint64_t));  

        /* Init Write Buffers */
        write_mem_buffers_ptr = reg_mem + write_mem_buffers_offset;

        per_term_buffer_data_size = scroll_mem_term_size / join_num;

        rand_gen = new RandomGen(max_iter_num);

        for (int i = 0; i < max_scroll_mem_term_num; i ++ ) {
            scroll_mem_term_kv_data[i] = reinterpret_cast<KV *>(reg_mem + scroll_mem_offset + i * scroll_mem_term_size);
        }

        per_term_buffer_data = new KV_PTR[join_num];
        per_term_buffer_data_offset = new uint64_t[join_num];
 
        char *null_server = nullptr;

        std::vector<std::thread> th_vec;

        for (int i = 0; i < join_num; i ++ ) {

            int dst_id = i;

            if (dst_id == src_id) {
                wrap_rdma_list.push_back(nullptr);
                continue;
            } else if (dst_id > src_id) {
                wrap_rdma_list.push_back(new WrapRdma(1, false));
            } else if (dst_id < src_id) {
                wrap_rdma_list.push_back(new WrapRdma(1, true));
            }
            // th_vec.push_back(std::thread([=]() {
            wrap_rdma_list[i]->establish_conn_async(reg_mem, max_reg_mem_size_join, transfer_port(src_id, dst_id), null_server); 
            // }));
        }

        // for (auto &th: th_vec) {
        //     th.join();
        // }

        global_sync();

        REDLOG("Init Succeed!\n");

        sleep(2);
    }

    void init_data() {

        rand_gen->gen_shuffle_req_data(max_iter_num + 100);
        init_per_term_buffer_data(r_scroll_mem_term_offset);

        for (int bl = 0; bl < join_num; bl ++) {

            for (int i = 0; i < init_data_size; i ++ ) {

                uint64_t get_num = rand_gen->get_shuffle_req_num_inf() + 1;
                update_KV(&per_term_buffer_data[bl][i], get_num, nullptr);
            }
        }

        rand_gen->gen_shuffle_req_data(max_iter_num + 100);
        init_per_term_buffer_data(s_scroll_mem_term_offset);
        
        for (int bl = 0; bl < join_num; bl ++) {

            for (int i = 0; i < init_data_size; i ++ ) {

                uint64_t get_num = rand_gen->get_shuffle_req_num_inf() + 1;
                update_KV(&per_term_buffer_data[bl][i], get_num, nullptr);
            }
        }

    }

#if (defined JOIN_USE_OPENMP)

    void construct_r_ht() {

        init_per_term_buffer_data(r_scroll_mem_term_offset);

        for (int bl = 0; bl < join_num; bl ++) {

            // int i;
            // uint64_t key;
            // volatile bool flag = false;
#pragma omp parallel for
            for (int i = 0; i < init_data_size + data_padding_size; i ++ ) {

                // if (flag) continue;

                uint64_t key = per_term_buffer_data[bl][i].key;
                if (key == 0ULL) {
                    continue;
                }
                r_ht->insert(key, per_term_buffer_data[bl][i].value);
            }
        }
#pragma omp barrier 
    }

    void scan_s() {

        init_per_term_buffer_data(s_scroll_mem_term_offset);
        int counter = 0;

        for (int bl = 0; bl < join_num; bl ++) {

            // int i;
            // uint64_t key;
            // volatile bool flag = false;
#pragma omp parallel for reduction(+:counter)
            for (int i = 0; i < init_data_size + data_padding_size ; i ++ ) {
                
                // if (flag) continue;

                uint64_t key = per_term_buffer_data[bl][i].key;
                if (key == 0ULL) {
                    continue;
                }
                if (r_ht->find(key)) {
                    counter ++;
                }
            }
        }
#pragma omp barrier 
        printf("[Join Result] %d\n", counter);
    }
#elif (defined JOIN_USE_SIMPLE)
     void construct_r_ht() {

        init_per_term_buffer_data(r_scroll_mem_term_offset);

        for (int bl = 0; bl < join_num; bl ++) {

            for (int i = 0; i < init_data_size + data_padding_size; i ++ ) {

                uint64_t key = per_term_buffer_data[bl][i].key;
                if (key == 0ULL) {
                    break;
                }
                r_ht->insert(key, per_term_buffer_data[bl][i].value);
            }
        }
    }

    void scan_s() {

        init_per_term_buffer_data(s_scroll_mem_term_offset);
        int counter = 0;

        for (int bl = 0; bl < join_num; bl ++) {

            for (int i = 0; i < init_data_size + data_padding_size ; i ++ ) {
                
                uint64_t key = per_term_buffer_data[bl][i].key;
                if (key == 0ULL) {
                    break;
                }
                if (r_ht->find(key)) {
                    counter ++;
                }
            }
        }
        printf("[Join Result] %d\n", counter);
    }   
#else

    void construct_r_ht() {

        init_per_term_buffer_data(r_scroll_mem_term_offset);

        for (int bl = 0; bl < join_num; bl ++) {

            // std::for_each(
            //     std::execution::par_unseq,
            //     per_term_buffer_data[bl],
            //     per_term_buffer_data[bl] + init_data_size + data_padding_size,
            //     [&](auto&& item) {
                    
            //     }
            // );
        }
    }

    void scan_s() {

        init_per_term_buffer_data(s_scroll_mem_term_offset);
        int counter = 0;

        for (int bl = 0; bl < join_num; bl ++) {

        }
        printf("[Join Result] %d\n", counter);
    }

#endif

    void transfer_data() {

        printf("Doorbell Method\n");
        
        uint64_t *send_offset = new uint64_t[join_num];
        std::memset(send_offset, 0, sizeof(uint64_t) * join_num);

        incre_scroll_mem_counter();
        init_per_term_buffer_data_offset();

        for(int bl = 0; bl < join_num; bl ++ ) {

            // if (bl == src_id) {
            //     continue;
            // }

            for (int i = 0; ; i ++ ) {

                uint64_t key = per_term_buffer_data[bl][i].key; 

                if (key == 0ULL) {
                    printf("[%d][BUFFER LEN]%d\n", bl, i);
                    break;
                }

#if 0
                int dst_node = distributed_rule(key, join_num, src_id);
#else
                int dst_node = rand_distributed_rule_tmp(key, join_num, src_id);
                if (dst_node == src_id) {
                    std::memmove(reg_mem + per_term_buffer_data_offset[src_id] + send_offset[dst_node],  reinterpret_cast<char *>(&per_term_buffer_data[bl][i]), kv_size);
                    send_offset[dst_node] += kv_size;
                    continue;
                }
#endif
        
                // printf("%d %d %p %d %lld\n", bl, i, &per_term_buffer_data[bl][i], dst_node, (long long)(per_term_buffer_data_offset[dst_node] + send_offset[dst_node]));
#ifdef USE_UNSIG
                wrap_rdma_list[dst_node]->wrap_post_write_offset(
                        reinterpret_cast<char *>(&per_term_buffer_data[bl][i]),
                        kv_size,
                        per_term_buffer_data_offset[src_id] + send_offset[dst_node], 
                        0);
#else
                wrap_rdma_list[dst_node]->wrap_post_write_offset_force(
                        reinterpret_cast<char *>(&per_term_buffer_data[bl][i]),
                        kv_size,
                        per_term_buffer_data_offset[src_id] + send_offset[dst_node], 
                        0);
#endif
                send_offset[dst_node] += kv_size;
            }
        }

        for(int id = 0; id < join_num; id ++ ) {
            if (id != src_id) {
#ifdef USE_UNSIG
                wrap_rdma_list[id]->wrap_post_write_offset(
                        clear_zero_buffer,
                        kv_size,
                        per_term_buffer_data_offset[src_id] + send_offset[id], 
                        0);
#else
                wrap_rdma_list[id]->wrap_post_write_offset_force(
                        clear_zero_buffer,
                        kv_size,
                        per_term_buffer_data_offset[src_id] + send_offset[id], 
                        0);
#endif
            }
        }

        init_per_term_buffer_data();
    }

    void transfer_data_sgl() {
        
        printf("SGL Method\n");
        printf("SRC %d\n", src_id);
        
        uint64_t *sgl_batch_counter = new uint64_t[join_num];
        std::memset(sgl_batch_counter, 0, sizeof(uint64_t) * join_num);

        char_ptr **sgl_buffer_addr_list = new char_ptr *[join_num];
        for (int i = 0; i < join_num; i ++ ) {
            sgl_buffer_addr_list[i] = new char_ptr[batch_size];
        }

        uint64_t *size_list = new uint64_t[batch_size];
        std::fill(size_list, size_list + batch_size, kv_size);
        
        uint64_t *send_offset = new uint64_t[join_num];
        std::memset(send_offset, 0, sizeof(uint64_t) * join_num);

        incre_scroll_mem_counter();
        init_per_term_buffer_data_offset();

        for(int bl = 0; bl < join_num; bl ++ ) {

            // if (bl == src_id) {

            //     continue;
            // }

            for (int i = 0; ; i ++ ) {

                uint64_t key = per_term_buffer_data[bl][i].key; 

                if (key == 0ULL) {
                    for(int j = 0; j < join_num; j ++ ) {
                        if (j != src_id, sgl_batch_counter[j] != 0) {
#ifdef USE_UNSIG
                            wrap_rdma_list[j]->wrap_post_batch_write_offset(sgl_buffer_addr_list[j],
                                    size_list,
                                    sgl_batch_counter[j],
                                    per_term_buffer_data_offset[src_id] + send_offset[j],
                                    0);
#else
                            wrap_rdma_list[j]->wrap_post_batch_write_offset_force(sgl_buffer_addr_list[j],
                                    size_list,
                                    sgl_batch_counter[j],
                                    per_term_buffer_data_offset[src_id] + send_offset[j],
                                    0);
#endif
                        }
                    }
                    printf("[%d][BUFFER LEN]%d\n", bl, i);
                    break;
                }

#if 0
                int dst_node = distributed_rule(key, join_num, src_id);
#else
                int dst_node = rand_distributed_rule_tmp(key, join_num, src_id);
                if (dst_node == src_id) {
                    std::memmove(reg_mem + per_term_buffer_data_offset[src_id] + send_offset[dst_node],  reinterpret_cast<char *>(&per_term_buffer_data[bl][i]), kv_size);
                    send_offset[dst_node] += kv_size;
                    continue;
                }
#endif
        
                sgl_buffer_addr_list[dst_node][sgl_batch_counter[dst_node]] = reinterpret_cast<char *>(&per_term_buffer_data[bl][i]),
                sgl_batch_counter[dst_node] ++;

                if (sgl_batch_counter[dst_node] == batch_size) {
#ifdef USE_UNSIG
                    wrap_rdma_list[dst_node]->wrap_post_batch_write_offset(sgl_buffer_addr_list[dst_node],
                            size_list,
                            batch_size,
                            per_term_buffer_data_offset[src_id] + send_offset[dst_node], 
                            0);
#else
                    wrap_rdma_list[dst_node]->wrap_post_batch_write_offset_force(sgl_buffer_addr_list[dst_node],
                            size_list,
                            batch_size,
                            per_term_buffer_data_offset[src_id] + send_offset[dst_node], 
                            0);
#endif
                    sgl_batch_counter[dst_node] = 0;

                    send_offset[dst_node] += kv_size * batch_size;
                }

            }
        }

        for(int id = 0; id < join_num; id ++ ) {
            if (id != src_id) {
#ifdef USE_UNSIG
                wrap_rdma_list[id]->wrap_post_write_offset(
                        clear_zero_buffer,
                        kv_size,
                        per_term_buffer_data_offset[src_id] + send_offset[id], 
                        0);
#else
                wrap_rdma_list[id]->wrap_post_write_offset_force(
                        clear_zero_buffer,
                        kv_size,
                        per_term_buffer_data_offset[src_id] + send_offset[id], 
                        0);
#endif
            }
        }

        init_per_term_buffer_data();
    }

    void transfer_data_sp(int base_offset) {

        printf("SP Method\n");
        
        uint64_t *sp_batch_counter = new uint64_t[join_num];
        std::memset(sp_batch_counter, 0, sizeof(uint64_t) * join_num);

        char_ptr **sp_buffer_addr_list = new char_ptr *[join_num];
        for (int i = 0; i < join_num; i ++ ) {
            sp_buffer_addr_list[i] = new char_ptr[batch_size];
        }

        uint64_t *size_list = new uint64_t[batch_size];
        std::fill(size_list, size_list + batch_size, kv_size);
        
        uint64_t *send_offset = new uint64_t[join_num];
        std::memset(send_offset, 0, sizeof(uint64_t) * join_num);

        // incre_scroll_mem_counter();
        init_per_term_buffer_data_offset(base_offset + 1);
        init_per_term_buffer_data(base_offset);

        int c_i = 0, c_j = 0;

        for(int bl = 0; bl < join_num; bl ++ ) {

            // if (bl == src_id) {

            //     continue;
            // }

            for (int i = 0; ; i ++ ) {

                uint64_t key = per_term_buffer_data[bl][i].key; 

                if (key == 0ULL) {
                    for(int j = 0; j < join_num; j ++ ) {
                        if (j != src_id, sp_batch_counter[j] != 0) {
#ifdef USE_UNSIG
                            wrap_rdma_list[j]->wrap_post_write_offset(reg_mem + get_write_buffer_offset(),
                                    sp_batch_counter[j] * batch_size,
                                    per_term_buffer_data_offset[src_id] + send_offset[j],
                                    0);
#else
                            wrap_rdma_list[j]->wrap_post_write_offset_force(reg_mem + get_write_buffer_offset(),
                                    sp_batch_counter[j] * batch_size,
                                    per_term_buffer_data_offset[src_id] + send_offset[j],
                                    0);
#endif
                            incre_write_buffer_counter();
                        }
                    }
                    printf("[%d][BUFFER LEN]%d\n", bl, i);
                    break;
                }

                int dst_node = distributed_rule(key, join_num, src_id);
                // printf("%d %d %d\n", key, src_id, dst_node)

                if (dst_node == src_id) {
                    std::memmove(reg_mem + per_term_buffer_data_offset[src_id] + send_offset[dst_node],  reinterpret_cast<char *>(&per_term_buffer_data[bl][i]), kv_size);
                    send_offset[dst_node] += kv_size;
                    c_i ++;
                    continue;
                }

                sp_buffer_addr_list[dst_node][sp_batch_counter[dst_node]] = reinterpret_cast<char *>(&per_term_buffer_data[bl][i]),
                sp_batch_counter[dst_node] ++;

                if (sp_batch_counter[dst_node] == batch_size) {

                    for (int j = 0; j < batch_size; j ++ ) {
                        
                        assert(sp_buffer_addr_list[dst_node][j] != nullptr);
                        assert(reg_mem + get_write_buffer_offset() + j * kv_size != nullptr);

                        std::memcpy(reg_mem + get_write_buffer_offset() + j * kv_size, 
                                sp_buffer_addr_list[dst_node][j], size_list[j]);
                    }
#ifdef USE_UNSIG
                    wrap_rdma_list[dst_node]->wrap_post_write_offset(reg_mem + get_write_buffer_offset(),
                            batch_size * kv_size,
                            per_term_buffer_data_offset[src_id] + send_offset[dst_node], 
                            0);
                    c_j ++;
#else
                    wrap_rdma_list[dst_node]->wrap_post_write_offset_force(reg_mem + get_write_buffer_offset(),
                            batch_size * kv_size,
                            per_term_buffer_data_offset[src_id] + send_offset[dst_node], 
                            0);
#endif
                    sp_batch_counter[dst_node] = 0;
                    incre_write_buffer_counter();
                    send_offset[dst_node] += kv_size * batch_size;
                }
            }
            // printf("%d %d\n", c_i, c_j);

        }

        for(int id = 0; id < join_num; id ++ ) {
            if (id != src_id) {
                wrap_rdma_list[id]->wrap_post_write_offset_force(
                        clear_zero_buffer,
                        kv_size,
                        per_term_buffer_data_offset[src_id] + send_offset[id], 
                        0);
            } else {
                std::memcpy(reg_mem + per_term_buffer_data_offset[src_id] + send_offset[id], clear_zero_buffer, kv_size);
            }
        }
    }

    void global_sync() {
        for (int i = 0; i < join_num; i ++ ) {
            if (i != src_id) {
                wrap_rdma_list[i]->wrap_post_faa(global_sync_faa_buffer, 1ULL, 0, 0);
            }
        }

        volatile uint64_t *global_sync_local = (volatile uint64_t *)global_sync_local_counter;
        while(*global_sync_local < join_num - 1);
        *global_sync_local = 0ULL;
    }

    void join_op(int base_offset) {

#ifdef JOIN_USE_SYNC
        global_sync();
#endif

#if (defined JOIN_USE_SGL)
        transfer_data_sgl(base_offset);
#elif (defined JOIN_USE_SP)
        transfer_data_sp(base_offset);
#else // Default
        transfer_data(base_offset);
#endif
    }

    void join_run() {
        init_data();

		auto begin = std::chrono::system_clock::now();
        uint64_t begin_rd = rdtsc();

        join_op(r_scroll_mem_term_offset); // preparation of R
        // printf("C:%d\n", get_scroll_mem_counter());

        join_op(s_scroll_mem_term_offset); // preparation of S
        // printf("C:%d\n", get_scroll_mem_counter());

        // Partition Join
        // 1. Insert R Table to HT
        construct_r_ht();

        // 2. Scan S Table with R HT
        scan_s();

        uint64_t end_rd = rdtsc();
        auto end = std::chrono::system_clock::now();
		std::chrono::duration<double> diff = end-begin;
		printf("[TIME]%lf\n", diff.count());
		printf("[TPUT]%lf\n", max_round_num * init_data_size * join_num / diff.count());

#ifdef USE_CLOCK_MEASURE
        uint64_t res = std::accumulate(wrap_rdma_list.begin(), wrap_rdma_list.end(), 0LL, [](uint64_t sum, WrapRdma *wrap_rdma) {
            if (wrap_rdma != nullptr) {

                sum += wrap_rdma->get_clock_total();
            }
            return sum;
        });
		printf("[CLOCK]%lld - %lld = diff: %lld\n", (long long)end_rd - begin_rd, (long long)res, (long long)end_rd - begin_rd - res);
#endif

        sleep(1);
    }
};

}

#endif // DISTRIBUTED_JOIN_HPP
