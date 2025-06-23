/*
 * Copyright @ stmatengss
 * email -> stmatengss@163.com
 * */
#ifndef BARRIER_H

#define BARRIER_H

#include "libmrdma.h"
#include "spin_lock.h"

void m_client_sync(struct m_ibv_res *ibv_res, char *addr, int qp_num, int peak_num) {
    int begin_counter = m_deter_post_faa(ibv_res, addr, 1ULL, sizeof(uint64_t), qp_num);

    do {
        begin_counter = m_deter_post_faa(ibv_res, addr, 0ULL, sizeof(uint64_t), qp_num);
    } while(begin_counter != peak_num);

}

#endif