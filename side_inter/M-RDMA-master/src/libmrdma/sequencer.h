/*
 * Copyright @ stmatengss
 * email -> stmatengss@163.com
 * */
#ifndef SEQUENCER_H

#define SEQUENCER_H

#include "libmrdma.h"

static forceinline void
m_client_conn_server(struct m_ibv_res *ibv_res) {

}

static forceinline void
m_server_init(struct m_ibv_res *ibv_res) {

}

static forceinline void
m_server_reset_seq(char *seq_val, uint64_t reset_val) {
    volatile uint64_t *seq_ptr = (volatile uint64_t *)seq_val;
    *seq_ptr = reset_val;
    __sync_synchronize();
}

static forceinline void
m_client_reset_seq(struct m_ibv_res *ibv_res, int offset, char* reset_val_ptr) {
    m_post_atomic_write_offset_sig_inline(ibv_res, reset_val_ptr, sizeof(uint64_t), offset, 0); // This one will execute memory fence
}

static forceinline uint64_t
m_client_get_seq(struct m_ibv_res *ibv_res, int offset, char* val_ptr) {
    m_post_faa_offset(ibv_res, val_ptr, 1ULL, offset, 0);
    m_poll_send_cq(ibv_res, 0);
    return *((volatile uint64_t *)val_ptr);
}

#endif