#ifndef DISTRIBUTED_LOG_HPP

#define DISTRIBUTED_LOG_HPP

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


namespace distributed_log {
#define USE_CACHE
#define USE_ZIPF
#define USE_SIMPLE_TABLE

/* ZIPF Config */
#ifdef USE_ZIPF
const double zipf_alpha = 0.99;
#endif

/* Log Structure */
struct LOG_ENTRY {
    uint32_t op;
    uint32_t len;
    char data[0]; // VAR_STR

    LOG_ENTRY(uint32_t op_, uint32_t len_): op(op_), len(len_) {
    }
};

/* Back End */
const long long meta_area_size = 1 << 5;
const long long meta_area_offset = 0;
const long long log_ptr_offset = CACHE_LINE_SIZE;
const long long data_area_size = 1 << 30;
const long long data_area_offset = meta_area_offset + meta_area_size;
const long long max_reg_mem_size_back = data_area_size + meta_area_size;

/* Front End */
const long long table_size = 1 << 25;
const long long table_offset = meta_area_offset + meta_area_size;
const long long write_buffer_size = 1 << 12;
const long long write_buffer_offset = table_offset + table_size;
const long long read_buffer_size = 1 << 4;
const long long read_buffer_offset = write_buffer_offset + write_buffer_size;
const long long max_reg_mem_size_front = table_size + write_buffer_size + read_buffer_size + 64;

/* Read Circular Buffer */
const long long read_buffer_item_len = sizeof(uint64_t);
const long long read_buffer_item_num = read_buffer_size / read_buffer_item_len;

static_assert(read_buffer_item_num >= 2, "Small Circular Buffer");

/* Table */
#if (defined USE_SIMPLE_TABLE)
const long long table_item_len = 32;
const long long table_item_num = table_size / table_item_len;
#endif

/* Config */
const int max_front_num = 8;
const int max_iter_num = 10000000;
const int max_hashtable_len = 1 << 17;
const int max_client_machine_num = 7;

class DistributedLog
{
public:
    /* port num */
    char *reg_mem;
    int port_num;
    int front_num;

    DistributedLog(int port_num_, int front_num_): port_num(port_num_), front_num(front_num_) {

    };
    ~DistributedLog() {}
};

class DistributedLogFront:DistributedLog
{
private:
    /* data */
    char *table;
    char *write_buffer;
    char *read_buffer;
    char *server_name;

    /* RDMA */
    WrapRdma *wrap_rdma;
    m_ibv_res *ibv_res;

    /* Random Gen */
    RandomGen *rand_gen;
    ZipfGen *zipf;

    /* Batch */
    int batch_size;
    int counter;
    int read_buffer_counter;
    char_ptr *sgl_buffer_addr;
    uint64_t *size_list;

    forceinline char *OFFSET2ADDR(uint64_t offset) {
        return reg_mem + offset;
    }

public:
    DistributedLogFront(int port_num_, int front_num_, int batch_size_, char *server_name_): 
        DistributedLog(port_num_, front_num_), server_name(server_name_), batch_size (batch_size_) {

        init_all();

        small_test();
        general_test();
    };
    ~DistributedLogFront() = delete;

    void init_all() {

        reg_mem = new char[max_reg_mem_size_front];

        sgl_buffer_addr = new char_ptr[batch_size];
        size_list = new uint64_t[batch_size];

        write_buffer = reg_mem + write_buffer_offset;
        read_buffer = reg_mem + read_buffer_offset;
        table = reg_mem + table_offset;

        rand_gen = new RandomGen(max_iter_num + 200);

        read_buffer_counter = 0;

        wrap_rdma = new WrapRdma(1, false);
        wrap_rdma->establish_conn_async(reg_mem, max_reg_mem_size_front, port_num, server_name);
        ibv_res = wrap_rdma->ibv_res;

    }

    char *get_read_buffer_addr() {
        return read_buffer + (read_buffer_counter ++ % read_buffer_item_num) * read_buffer_item_len;
    }

    uint64_t get_log_addr() {

        char *read_buffer_addr = get_read_buffer_addr();
        assert(read_buffer_addr < OFFSET2ADDR(max_reg_mem_size_front));
        return wrap_rdma->wrap_post_faa(read_buffer_addr, batch_size * table_item_len, log_ptr_offset, 0);
    }

#if (defined USE_SIMPLE_TABLE)
    void gen_tx_log(uint64_t rand_num) {
        
        int offset = rand_num % table_item_num * table_item_len;
    }

    void gen_tx_log_sgl() {

        uint64_t remote_log_addr = get_log_addr();

        for (int i = 0; i < batch_size; i ++ ) {
#ifdef USE_ZIPF
            uint64_t get_num = zipf->next_long();
#else
            uint64_t get_num = rand_gen->get_shuffle_req_num_inf();
#endif
            uint64_t offset = table_offset + get_num % table_item_num * table_item_len;
            sgl_buffer_addr[i] = OFFSET2ADDR(offset);
        }

        std::fill(size_list, size_list + batch_size, table_item_len);
        wrap_rdma->wrap_post_batch_write_offset(
            sgl_buffer_addr,
            size_list,
            batch_size,
            remote_log_addr, 
            0
        );
    }

    void gen_tx_log_doorbell() {

    }
#endif

    void small_test() {
        
        m_client_sync(wrap_rdma->ibv_res, reg_mem + meta_area_offset, 0, front_num);
        LOG("Everything is OK~\n");
    }

    void general_test() {

#ifdef USE_ZIPF
        zipf = new ZipfGen(zipf_alpha, max_hashtable_len);
#else
        rand_gen->gen_shuffle_req_data(max_iter_num + 100);
#endif

		auto begin = std::chrono::system_clock::now();

        for(int iter = 0; iter <= max_iter_num / batch_size; iter ++) {

            // printf("%d\n", iter);
            gen_tx_log_sgl();
        }

		auto end = std::chrono::system_clock::now();
		std::chrono::duration<double> diff = end-begin;
		printf("[TIME]%lf\n", diff.count());
		printf("[TPUT]%lf\n", max_iter_num / diff.count());
    }

};

class DistributedLogBack:DistributedLog
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
    DistributedLogBack(int port_num_, int front_num_): DistributedLog(port_num_, front_num_) {
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
    ~DistributedLogBack() = delete;

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
};

}

#endif