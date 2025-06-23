#if !defined(SPIN_LOCK_H)
#define SPIN_LOCK_H

#include "libmrdma.h"

#define LOCK 1ULL
#define UNLOCK 0ULL

#define SLEEP_TIME 100

static forceinline void m_cpu_relax () {
	// asm("pause");
    m_nano_sleep(SLEEP_TIME);
}

static forceinline uint64_t 
m_deter_post_cas(struct m_ibv_res *ibv_res, char *buffer, uint64_t compare, uint64_t swap, long offset, int qp_index) {
    m_post_cas_offset(ibv_res, buffer, compare, swap, offset, qp_index);
    m_poll_send_cq(ibv_res, qp_index);
    return *((volatile uint64_t *)buffer);
}

static forceinline uint64_t 
m_deter_post_faa(struct m_ibv_res *ibv_res, char *buffer, uint64_t add, long offset, int qp_index) {
    m_post_faa_offset(ibv_res, buffer, add, offset, qp_index);
    m_poll_send_cq(ibv_res, qp_index);
    return *((volatile uint64_t *)buffer);
}

static forceinline void
m_init_lock(struct m_ibv_res *ibv_res, char *lock_val, int qp_index) {
    //  TODO
}

static forceinline void
m_lock(struct m_ibv_res *ibv_res, char *lock_val, int qp_index) {
	while (m_deter_post_cas(ibv_res, lock_val, UNLOCK, LOCK, 0, 0)) {

        // printf("Val: %lld", *((volatile long long *)lock_val));
    };
}

static forceinline void
m_unlock(struct m_ibv_res *ibv_res, char *lock_val, int qp_index) { 
    volatile uint64_t *lock_ptr = (volatile uint64_t *)lock_val;
    *lock_ptr = UNLOCK;
    m_post_write_offset_sig_inline(ibv_res, lock_val, sizeof(uint64_t), 0, 0);
    m_poll_send_cq(ibv_res, 0);
}

static forceinline void
m_lock_back_off(struct m_ibv_res *ibv_res, char *lock_val, int qp_index) {
	while (1) {

        size_t wait = 1;
		if (!m_deter_post_cas(ibv_res, lock_val, UNLOCK, LOCK, 0, 0)) {
			return;
		}
		
		for (int i = 0; i < wait; i ++ ) {
			m_cpu_relax();
		}

        volatile uint64_t *lock_ptr = (volatile uint64_t *)lock_val;
        m_post_read_offset_sig(ibv_res, lock_val, sizeof(uint64_t), 0, 0);
        m_poll_send_cq(ibv_res, 0);
		while (*lock_val) {
			wait *= 2;
			for (int i = 0; i < wait; i ++ ) {
				m_cpu_relax();
			}
            m_post_read_offset_sig(ibv_res, lock_val, sizeof(uint64_t), 0, 0);
            m_poll_send_cq(ibv_res, 0);
		}
        // printf("Val: %lld", *((volatile long long *)lock_val));
    }
}

#endif // SPIN_LOCK_H
