#ifndef DYNAMIC_CONN_UD_H
#define DYNAMIC_CONN_UD_H

#include "libmrdma.h"

static forceinline void
m_reload_multi_recv(struct m_ibv_res *ibv_res, char *ud_buffer, int num, int qp_index) {
	
	m_post_multi_recv(ibv_res, ud_buffer + M_CONN_LEN, M_CONN_PADDING, num, qp_index);
}

static forceinline void
m_init_param(struct m_ibv_res *ibv_res, char* rc_buffer_addr, int qp_index, char *ud_conn_addr) {

	ibv_res->lparam[qp_index] = (struct m_param *)ud_conn_addr;
	ibv_res->lparam[qp_index]->psn = lrand48() & 0xffffff;
	ibv_res->lparam[qp_index]->qpn = ibv_res->qp[qp_index]->qp_num;

	ibv_res->lparam[qp_index]->lid = m_get_lid(ibv_res);

	ud_conn_addr += sizeof(struct m_param);
	ibv_res->lpriv_data[qp_index] = (struct m_priv_data *)ud_conn_addr;
	ibv_res->lpriv_data[qp_index]->buffer_addr = (uint64_t)rc_buffer_addr;
	ibv_res->lpriv_data[qp_index]->buffer_rkey = ibv_res->mr->rkey;
	ibv_res->lpriv_data[qp_index]->buffer_length = ibv_res->mr->length;

	ud_conn_addr += sizeof(struct m_priv_data) + M_UD_PADDING;
	ibv_res->rparam[qp_index] = (struct m_param *)ud_conn_addr;

//	ud_conn_addr += sizeof(struct m_param) + M_UD_PADDING;
	ud_conn_addr += sizeof(struct m_param);
	ibv_res->rpriv_data[qp_index] = (struct m_priv_data *)ud_conn_addr;

}

static forceinline void
m_exchange_via_ud_by_id(struct m_ibv_res *ibv_res_ud, int qp_index_ud, 
	              struct m_param *lparam, struct m_priv_data *lpriv_data, 
	              struct m_param *rparam, struct m_priv_data *rpriv_data) {

//	m_post_recv(ibv_res_ud, (char *)rpriv_data - M_UD_PADDING, sizeof(struct m_priv_data) + M_UD_PADDING, qp_index_ud);
	if (ibv_res_ud->is_server == 1) {
	
		m_post_recv(ibv_res_ud, (char *)rparam - M_UD_PADDING, sizeof(struct m_param) + sizeof(struct m_priv_data) + M_UD_PADDING, qp_index_ud);

		m_poll_recv_cq(ibv_res_ud, qp_index_ud);

		m_post_ud_send_sig(ibv_res_ud, (char *)lparam, sizeof(struct m_param) + sizeof(struct m_priv_data), qp_index_ud);

		m_poll_send_cq(ibv_res_ud, qp_index_ud);

	} else {

		m_post_ud_send_sig(ibv_res_ud, (char *)lparam, sizeof(struct m_param) + sizeof(struct m_priv_data), qp_index_ud);

		m_poll_send_cq(ibv_res_ud, qp_index_ud);
		
		m_post_recv(ibv_res_ud, (char *)rparam - M_UD_PADDING, sizeof(struct m_param) + sizeof(struct m_priv_data) + M_UD_PADDING, qp_index_ud);

		m_poll_recv_cq(ibv_res_ud, qp_index_ud);
	}

//	m_post_ud_send_sig(ibv_res_ud, (char *)lpriv_data, sizeof(struct m_priv_data), qp_index_ud);
	
//	m_poll_recv_cq(ibv_res_ud, qp_index_ud);
	
}

static forceinline void
m_sync_via_ud_by_id(struct m_ibv_res *ibv_res, char* rc_buffer_addr, 
	int qp_index, struct m_ibv_res *ibv_res_ud, char *ud_conn_addr, int qp_index_ud) {

	m_init_param(ibv_res, rc_buffer_addr, qp_index, ud_conn_addr);

	// if (ibv_res_ud->is_server) {
	// 	m_nano_sleep(4000);
	// }

#ifndef DEBUG
	printf("Local LID = %d, QPN = %d, PSN = %d\n",
	       ibv_res->lparam[qp_index]->lid, ibv_res->lparam[qp_index]->qpn, ibv_res->lparam[qp_index]->psn);
	printf("Local Addr = %ld, RKey = %d, LEN = %zu\n",
	       ibv_res->lpriv_data[qp_index]->buffer_addr, ibv_res->lpriv_data[qp_index]->buffer_rkey,
	       ibv_res->lpriv_data[qp_index]->buffer_length);
#endif

	m_exchange_via_ud_by_id(ibv_res_ud, qp_index_ud, 
		ibv_res->lparam[qp_index], ibv_res->lpriv_data[qp_index],
		ibv_res->rparam[qp_index], ibv_res->rpriv_data[qp_index]);

#ifndef DEBUG
//#ifdef _DEBUG

	printf("Remote LID = %d, QPN = %d, PSN = %d\n",
	       ibv_res->rparam[qp_index]->lid, ibv_res->rparam[qp_index]->qpn, ibv_res->rparam[qp_index]->psn);

	printf("Remote Addr = %ld, RKey = %d, LEN = %zu\n",
	       ibv_res->rpriv_data[qp_index]->buffer_addr, ibv_res->rpriv_data[qp_index]->buffer_rkey,
	       ibv_res->rpriv_data[qp_index]->buffer_length);
#endif

}

#endif // DYNAMIC_CONN_UD_H
