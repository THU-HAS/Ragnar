#define IB_DEVICE_NAME "mlx5_0"

#ifndef WRAP_RDMA_HPP
#define WRAP_RDMA_HPP


/* RDMA parameters */
// const int POLL_DIS = 1;
// const int W_POLL_DIS = 4;
// const int R_POLL_DIS = 4;
const int W_POLL_DIS = 32;
const int R_POLL_DIS = 32;

extern "C"{
#include "libmrdma.h"
}

#include "src/tool/utils.hpp"

#include <malloc.h>

#define USE_UNSIG
#define MAX_QP_LEN 1024
// #define USE_RDMA_READ_OPT
#define USE_CLOCK_MEASURE
#include <string.h>

class WrapRdma{
public:
    bool is_server;
    int qp_sum;

    long sig_counter;
    m_ibv_res *ibv_res;
    int max_inline_size;
    int counter[5];
    unsigned long long start_time;
    long long reg_mem_max_size;
    char *reg_mem;

#ifdef USE_CLOCK_MEASURE
    long long clock_total;
#endif
    // NVMConfig* nvm = nullptr;

    WrapRdma(int qp_sum_ = 1, bool is_server_ = true): qp_sum(qp_sum_), is_server(is_server_) {
        ibv_res = new m_ibv_res();

        // printf("[QP_SUM]%d\t[IS_SERVER]%d\n", qp_sum, is_server);

        FILL(*ibv_res);
        ibv_res->is_server = is_server;

#ifdef M_USE_ROCE
        m_init_parameter(ibv_res, 2, 7000, 0xdeadbeaf, M_RC, qp_sum);
#else
        m_init_parameter(ibv_res, 1, 7000, 0xdeadbeaf, M_RC, qp_sum);
#endif

        max_inline_size = MAX_INLINE_DATA;
        sig_counter = 1;

        LOG("/====rdmaer created success====/\n");
    }
    virtual ~WrapRdma() {

    }

    forceinline void replica_ctx(WrapRdma *src) {

        m_copy_ctx_and_pd(ibv_res, src->ibv_res);
    }

    forceinline void establish_conn(char *reg_mem_, long long reg_mem_max_size_, int tcp_port, char *server_name) {

        reg_mem = reg_mem_;
        reg_mem_max_size = reg_mem_max_size_;

        if (ibv_res->ctx == nullptr) {

			printf("Open Device and Alloc\n");
            m_open_device_and_alloc_pd(ibv_res, IB_DEVICE_NAME);
        }

        REDLOG("[REG_MEM_SIZE]%ld\n", reg_mem_max_size);
        m_reg_buffer(ibv_res, reg_mem, reg_mem_max_size);
        m_create_cq_and_qp(ibv_res, MAX_QP_LEN, IBV_QPT_RC);

        ibv_res->port = tcp_port;

        m_sync(ibv_res, server_name, reg_mem);

        m_modify_qp_to_rts_and_rtr(ibv_res);
    }

    forceinline void establish_conn_async(char *reg_mem_, long long reg_mem_max_size_, int tcp_port, char *server_name) {


        reg_mem = reg_mem_;
        reg_mem_max_size = reg_mem_max_size_;
        
        if (ibv_res->ctx == nullptr) {

			printf("Open Device and Alloc\n");
            m_open_device_and_alloc_pd(ibv_res, IB_DEVICE_NAME);
        }

        REDLOG("[REG_MEM_SIZE]%ld\n", reg_mem_max_size);
//		int fuck_size = 100000000;
//		int fuck_size = 16384 * 1000 * 128;
//		posix_memalign(reg_mem, 8096, fuck_size);
//		reg_mem = (char *)memalign(8096, fuck_size);
//        m_reg_buffer(ibv_res, reg_mem, fuck_size);

        m_reg_buffer(ibv_res, reg_mem, reg_mem_max_size);
        m_create_cq_and_qp(ibv_res, MAX_QP_LEN, IBV_QPT_RC);

        ibv_res->port = tcp_port;

        m_sync_memc(ibv_res, server_name, reg_mem);

        m_modify_qp_to_rts_and_rtr(ibv_res);
    }


    forceinline uint64_t wrap_post_cas(char *buffer, uint64_t compare, uint64_t swap, long offset, int qp_index) {
        m_post_cas_offset(ibv_res, buffer, compare, swap, offset, qp_index);
        m_poll_send_cq(ibv_res, qp_index);
        return *((volatile uint64_t *)buffer);
    }

    forceinline uint64_t wrap_post_faa(char *buffer, uint64_t add, long offset, int qp_index) {
        m_post_faa_offset(ibv_res, buffer, add, offset, qp_index);
        m_poll_send_cq(ibv_res, qp_index);
        return *((uint64_t *)buffer);
    }

    forceinline void wrap_post_write_offset(char *buffer, size_t size, long offset, int qp_index) {

#ifdef USE_CLOCK_MEASURE
        uint64_t begin = rdtsc();
#endif
        if (size > max_inline_size) {
            wrap_post_write_offset_without_inline(buffer, size, offset, qp_index);
            return;
        }
        if (sig_counter % W_POLL_DIS == 0) {
            m_post_write_offset_sig_inline(ibv_res, buffer, size, offset, qp_index);
            m_poll_send_cq(ibv_res, qp_index);
        } else {
            m_post_write_offset_inline(ibv_res, buffer, size, offset, qp_index);
        }
        sig_counter ++;
#ifdef STAT
        counter[0] ++;
#endif

#ifdef USE_CLOCK_MEASURE
        clock_total += (rdtsc() - begin);
#endif
    }

    forceinline void wrap_post_read_offset(char *buffer, size_t size, long offset, int qp_index) {

#ifdef USE_CLOCK_MEASURE
        uint64_t begin = rdtsc();
#endif
        if (sig_counter % R_POLL_DIS == 0) {
            m_post_read_offset_sig(ibv_res, buffer, size, offset, qp_index);
            m_poll_send_cq(ibv_res, qp_index);
        } else {
            m_post_read_offset(ibv_res, buffer, size, offset, qp_index);
        }

        sig_counter ++;
#ifdef STAT
        counter[1] ++;
#endif

#ifdef USE_CLOCK_MEASURE
        clock_total += (rdtsc() - begin);
#endif
    } 

    forceinline void wrap_post_write_offset_without_inline(char *buffer, size_t size, long offset, int qp_index) {

#ifdef USE_CLOCK_MEASURE
        uint64_t begin = rdtsc();
#endif        
        if (sig_counter % W_POLL_DIS == 0) {
            m_post_write_offset_sig(ibv_res, buffer, size, offset, qp_index);
            m_poll_send_cq(ibv_res, qp_index);
        } else {
            m_post_write_offset(ibv_res, buffer, size, offset, qp_index);
        }
        sig_counter ++;

#ifdef STAT
        counter[2] ++;      
#endif

#ifdef USE_CLOCK_MEASURE
        clock_total += (rdtsc() - begin);
#endif
    }

    forceinline void wrap_post_write_offset_force(char *buffer, size_t size, long offset, int qp_index) {

#ifdef USE_CLOCK_MEASURE
        uint64_t begin = rdtsc();
#endif
        m_post_write_offset_sig(ibv_res, buffer, size, offset, qp_index);
        m_poll_send_cq(ibv_res, qp_index);

#ifdef STAT
        counter[3] ++;      
#endif

#ifdef USE_CLOCK_MEASURE
        clock_total += (rdtsc() - begin);
#endif
    }

    forceinline void wrap_post_batch_write_offset_force(char **buffer_list, size_t* size_list, int length, long offset, int qp_index) {

#ifdef USE_CLOCK_MEASURE
        uint64_t begin = rdtsc();
#endif
        m_post_write_sgl(ibv_res, buffer_list, size_list, length, offset, 0);
        m_poll_send_cq(ibv_res, qp_index);

#ifdef STAT
        counter[3] ++;      
#endif

#ifdef USE_CLOCK_MEASURE
        clock_total += (rdtsc() - begin);
#endif
    }

    forceinline void wrap_post_batch_write_offset(char **buffer_list, size_t* size_list, int length, long offset, int qp_index) {

#ifdef USE_CLOCK_MEASURE
        uint64_t begin = rdtsc();
#endif
        if (sig_counter % W_POLL_DIS == 0) {
            m_post_write_sgl(ibv_res, buffer_list, size_list, length, offset, 0);
            m_poll_send_cq(ibv_res, qp_index);
        } else {
            m_post_write_sgl_unsig(ibv_res, buffer_list, size_list, length, offset, 0);
        }
        sig_counter ++;
#ifdef STAT
        counter[0] ++;
#endif

#ifdef USE_CLOCK_MEASURE
        clock_total += (rdtsc() - begin);
#endif
    }

#if (defined USE_RDMA_READ_OPT)
    forceinline void wrap_post_read_offset_force(char *buffer, size_t size, long offset, int qp_index) {

        int *flag = (int *)(buffer + size - 5);
        *flag = 0xdeadbeaf;

        if (sig_counter % R_POLL_DIS == 0) {
            m_post_read_offset_sig(ibv_res, buffer, size, offset, qp_index);
            m_poll_send_cq(ibv_res, qp_index);
        } else {
            m_post_read_offset(ibv_res, buffer, size, offset, qp_index);
        }

        while(*(volatile int *)flag == 0xdeadbeaf); 
        sig_counter ++;
        counter[4] ++;      
    }

#else
    forceinline void wrap_post_read_offset_force(char *buffer, size_t size, long offset, int qp_index) {

#ifdef USE_CLOCK_MEASURE
        uint64_t begin = rdtsc();
#endif
        m_post_read_offset_sig(ibv_res, buffer, size, offset, qp_index);
        m_poll_send_cq(ibv_res, qp_index);

#ifdef STAT
        counter[4] ++;      
#endif

#ifdef USE_CLOCK_MEASURE
        clock_total += (rdtsc() - begin);
#endif
    } 
#endif

    forceinline uint64_t get_clock_total() {
        return clock_total;
    }
};

#endif // WRAP_RDMA_HPP
