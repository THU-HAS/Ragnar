#include <stdio.h>
#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <atomic>

#include "src/tool/utils.hpp"

extern "C"{

#include "src/libmrdma/libmrdma.h"
#include "src/libmrdma/dynamic_conn_ud.h"
}

using namespace std;

// #define USE_MULTI_CTX
#define USE_UD_CONN
// #define USE_SRQ
#define USE_ONE_SIDE
// #define USE_PRINT

// #define THREAD_NAME(role) role##_batch_thread
#define THREAD_NAME(role) role##_thread

typedef char *charptr;

const long ud_buffer_size = 1L << 10;
const long buffer_size = 1L << 10;
const long data_buffer_size = 1L << 14;
const int poll_cyc = 1010;
const int queue_depth = 1000;
const int test_number = 10000000;
const int ITER_NUM = 2000000;
const int ITER_DIS_NUM = 100000;
const int BATCH_SIZE = 32;
const int PUSH_SIZE = 128;
const int UNSIG_SIZE = 1;
const int CONN_ITER = 100;
const int TCASE_ITER = 5;
const int PINGPONG_DIS = 1;
const int POLL_MULTI_RECV_CQ_NUM = 8;
const int DOORBELL_SIZE = 64;
random_device req_rd;
vector<thread> thread_list;
struct ibv_wc wcs[queue_depth]; 

atomic<int> local_seq_num;

#define push_multi_rq(num) { \
	for (int i = 0; i < num; i ++) { \
		m_post_recv(&ibv_res, buffer, send_size); \
	} \
}

#define push_multi_rq_sgl(num) { \
	for (int i = 0; i < num; i ++) { \
		m_post_recv_sgl(&ibv_res, buffer, offsets, buffer_size_list, sgl_n); \
	} \
}


void client_thread(m_ibv_res *ibv_res, char *buffer, int send_size, int qp_index) {

	int qp_sum = ibv_res->qp_sum;

	for (int i = 0; i < ITER_NUM + 100; i ++ ) {
		printf("%d\n", i);
#ifdef USE_PRINT
		if (i % 10000 == 0) {
			printf("%d, %d\n", qp_index, i);
		}
#endif

		// if (i % POLL_MULTI_RECV_CQ_NUM == 0 && i > 0) {
		// 	m_poll_recv_cq_multi(ibv_res, POLL_MULTI_RECV_CQ_NUM, qp_index);
		// }
		m_poll_recv_cq(ibv_res, qp_index);

		m_post_recv(ibv_res, buffer, send_size + M_UD_PADDING, qp_index);

#ifdef USE_ONE_SIDE
		if (i % PINGPONG_DIS == 0) {
			
			if (i % UNSIG_SIZE == 0) {

				m_post_ud_send_sig_inline(ibv_res, buffer, send_size, qp_index);

				m_poll_send_cq(ibv_res, qp_index);
			} else {
				m_post_ud_send_inline(ibv_res, buffer, send_size, qp_index);
			}
		}
#endif
		//if (i % BATCH_SIZE == 0) m_poll_send_cq(&ibv_res);
	}
	// sleep(5);

	printf("[BUFFER]%s\n", buffer + M_UD_PADDING);
}

void server_thread(m_ibv_res *ibv_res, char *buffer, int send_size, int qp_index) {
	memset(buffer, '-', buffer_size);
	//				buffer[32] = 'c';
	//				buffer[31] = 'd';
	int qp_sum = ibv_res->qp_sum;

	sleep(1);
	auto start = chrono::system_clock::now();
	for (int i = 0; i < ITER_NUM; i ++ ) {
		printf("%d\n", i);
#ifdef USE_PRINT
		if (i % 10000 == 0) {
			printf("%d, %d\n", qp_index, i);
		}
#endif
		if (i % UNSIG_SIZE == 0) {

			m_post_ud_send_sig_inline(ibv_res, buffer, send_size, qp_index);

			m_poll_send_cq(ibv_res, qp_index);
		} else {
			m_post_ud_send_inline(ibv_res, buffer, send_size, qp_index);
		}
#ifdef USE_ONE_SIDE
		if (i % PINGPONG_DIS == 0) {

			m_poll_recv_cq(ibv_res, qp_index);
			// if (i % POLL_MULTI_RECV_CQ_NUM == 0) {
			// 	m_poll_recv_cq_multi(ibv_res, POLL_MULTI_RECV_CQ_NUM, qp_index);
			// }

			m_post_recv(ibv_res, buffer, send_size + M_UD_PADDING, qp_index);
		}
#endif
		local_seq_num ++;
		//if (i % BATCH_SIZE == 0) m_poll_send_cq(&ibv_res);
	}


	auto end = chrono::system_clock::now();
	std::chrono::duration<double> diff = end-start;
	printf("[TIME]%lf\n", diff.count());
	printf("[TPUT]%lf\n", ITER_NUM / diff.count() / 1000000.0);

	sleep(1);
}

#define SIMPLE_POLL

void client_batch_thread(m_ibv_res *ibv_res, char *buffer, int send_size, int qp_index) {

	char **buffers = new charptr[DOORBELL_SIZE];
	size_t *send_sizes = new size_t[DOORBELL_SIZE];
	size_t *recv_sizes = new size_t[DOORBELL_SIZE];

	for (int i = 0; i < DOORBELL_SIZE; i ++ ) {
		buffers[i] = buffer;
		send_sizes[i] = send_size;
		recv_sizes[i] = send_size + M_UD_PADDING;
	}

	int qp_sum = ibv_res->qp_sum;

	int iter = 0;
	int pre_id = 0, now_id;

	while (true) {

#ifdef USE_PRINT
		if (iter % 1 == 0) {
			printf("%d, %d\n", qp_index, iter);
		}
#endif
		printf("%d, %d\n", qp_index, iter);

#ifdef SIMPLE_POLL
		for (int i = 0; i < DOORBELL_SIZE; i ++ ) {
			now_id = m_poll_recv_cq_get_id(ibv_res, qp_index);
			if (now_id % 10000 == 0) {

				printf("[%d]%d\n", qp_index, now_id);
			}
			if (pre_id + 1 != now_id && pre_id != 0) {
				printf("[%d][DIFF]%d %d\n", qp_index, pre_id, now_id);
			}
			pre_id = now_id;
			if (now_id == ITER_NUM - 1) {
				printf("[OK~]\n");
			}
		}
#else
		int ret_num = m_poll_recv_cq_and_return(ibv_res, DOORBELL_SIZE, qp_index, wcs);
		iter += ret_num;
#endif
		if (buffer[M_UD_PADDING] == '+') {

			printf("[%d][Recv_Pre]%d\n", qp_index, iter);

			int ret_num = m_poll_recv_cq_and_return(ibv_res, 10000, qp_index, wcs);			
			iter += ret_num;

			printf("[%d][Recv_Now]%d\n", qp_index, iter);
			for (int i = 0; i < 10; i ++){
				printf("[%d][WC_ID%d]%d\n", qp_index, i, m_poll_recv_cq_get_id(ibv_res, qp_index));
			}

			printf("[%d][Bye~]\n", qp_index);	
			return;
		}

#ifdef USE_SRQ
		m_post_multi_share_recv(ibv_res, buffer, send_size + M_UD_PADDING, DOORBELL_SIZE);
#else
	#ifdef SIMPLE_POLL
		m_post_batch_recv(ibv_res, buffers, recv_sizes, DOORBELL_SIZE, qp_index);
	#else
		m_post_batch_recv(ibv_res, buffers, recv_sizes, ret_num, qp_index);
	#endif
#endif

#ifdef USE_ONE_SIDE
		if (iter % PINGPONG_DIS == 0) {

			m_post_ud_batch_send_inline(ibv_res, buffers, send_sizes, DOORBELL_SIZE, BATCH_SIZE, qp_index);
		}
#endif
	}

	printf("[BUFFER]%s\n", buffer + M_UD_PADDING);
}

void client_batch_share_thread(m_ibv_res *ibv_res, char *buffer, int send_size, int qp_index) {

	char **buffers = new charptr[DOORBELL_SIZE];
	size_t *send_sizes = new size_t[DOORBELL_SIZE];
	size_t *recv_sizes = new size_t[DOORBELL_SIZE];

	for (int i = 0; i < DOORBELL_SIZE; i ++ ) {
		buffers[i] = buffer;
		send_sizes[i] = send_size;
		recv_sizes[i] = send_size + M_UD_PADDING;
	}

	int qp_sum = ibv_res->qp_sum;

	int iter = 0;
	int pre_id = 0, now_id;

	while (true) {

		// printf("%d, %d\n", qp_index, iter);

		for (int i = 0; i < DOORBELL_SIZE; i ++ ) {

#ifdef USE_SRQ
			m_poll_all_recv_cq_rr(ibv_res, buffer, send_size + M_UD_PADDING);
#endif

		}

		if (buffer[M_UD_PADDING] == '+') {

			printf("[%d][Bye~]\n", qp_index);	
			return;
		}

		// m_post_multi_share_recv(ibv_res, buffer, send_size + M_UD_PADDING, DOORBELL_SIZE);
	}

	printf("[BUFFER]%s\n", buffer + M_UD_PADDING);
}

void server_batch_thread(m_ibv_res *ibv_res, char *buffer, int send_size, int qp_index) {
	memset(buffer, '-', send_size);
	
	char **buffers = new charptr[DOORBELL_SIZE];
	size_t *send_sizes = new size_t[DOORBELL_SIZE];
	size_t *recv_sizes = new size_t[DOORBELL_SIZE];

	for (int i = 0; i < DOORBELL_SIZE; i ++ ) {
		buffers[i] = buffer;
		send_sizes[i] = send_size;
		recv_sizes[i] = send_size + M_UD_PADDING;
	}

	int qp_sum = ibv_res->qp_sum;

	sleep(1);
	auto start = chrono::system_clock::now();
	for (int i = 0; i < ITER_NUM / DOORBELL_SIZE; i ++ ) {

#ifdef USE_PRINT
		if (i % 1 == 0) {
			printf("%d, %d\n", qp_index, i);
		}
#endif

		m_post_ud_batch_send_inline(ibv_res, buffers, send_sizes, DOORBELL_SIZE, BATCH_SIZE, qp_index);

#ifdef USE_ONE_SIDE
		if (i % PINGPONG_DIS == 0) {
			int ret_num = m_poll_recv_cq_and_return(ibv_res, DOORBELL_SIZE, qp_index, wcs);

			m_post_batch_recv(ibv_res, buffers, recv_sizes, ret_num, qp_index);

			// m_poll_recv_cq_multi(ibv_res, DOORBELL_SIZE, qp_index);

			// m_post_batch_recv(ibv_res, buffers, recv_sizes, DOORBELL_SIZE, qp_index);
		}
#endif
	}


	auto end = chrono::system_clock::now();
	std::chrono::duration<double> diff = end-start;
	printf("[TIME]%lf\n", diff.count());
	printf("[TPUT]%lf\n", ITER_NUM / diff.count() / 1000000.0);

// End communication
	memset(buffer, '+', send_size);

	m_post_ud_send_sig_inline(ibv_res, buffer, send_size, qp_index);
	m_poll_send_cq(ibv_res, qp_index);

	sleep(2);
}
/*
	Test Main
*/

void test(int argc, char **argv) {
	struct m_ibv_res ibv_res;
	FILL(ibv_res);

	char *buffer = new char[buffer_size];
	//int m_buffer_size = 128 * 64;
	const char *server;
	if (argc == 3) {
		ibv_res.is_server = 1;	
	} else if (argc == 4) {
		ibv_res.is_server = 0;
		server = argv[3];
		printf("Connect to %s\n", server);
	} else {
		printf("[ERROR] illegal input\n");
	}
	int send_size = atoi(argv[1]);
	int qp_sum = atoi(argv[2]);

	thread_list = vector<thread>(qp_sum);

	m_init_parameter(&ibv_res, 1, 7089, 0xdeadbeaf, M_UD, qp_sum);

	m_open_device_and_alloc_pd(&ibv_res);

	// m_get_is_odp(&ibv_res);	

	m_reg_buffer(&ibv_res, buffer, buffer_size);

	m_create_cq_and_qp(&ibv_res, poll_cyc, IBV_QPT_UD);

	m_sync(&ibv_res, server, buffer);

#ifdef USE_SRQ
	m_post_multi_share_recv(&ibv_res, buffer, send_size + M_UD_PADDING, 100);
#else
	for (int i = 0; i < qp_sum; i ++ ) {
		for (int j = 0; j < 100; j ++ ) {
			m_post_recv(&ibv_res, buffer, send_size + M_UD_PADDING, i);
		}
		// m_post_multi_recv(&ibv_res, buffer, send_size + M_UD_PADDING, 100, i);
	}
#endif

	m_modify_qp_to_rts_and_rtr(&ibv_res);

	if (ibv_res.is_server) {

		// server_batch_thread(&ibv_res, buffer, send_size, 0);
		for (int i = 0; i < qp_sum; i ++ ) {

			char *tmp_buffer = (buffer + i * send_size);

			thread_list[i] = thread(THREAD_NAME(server), &ibv_res, tmp_buffer, send_size, i);
		}

		for (int i = 0; i < qp_sum; i ++ ) {
			thread_list[i].join();
		}

	} else {
		// client_batch_thread(&ibv_res, buffer, send_size, 0);

#ifndef USE_SRQ
		for (int i = 0; i < qp_sum; i ++ ) {

			char *tmp_buffer = (char *)(buffer + i * (send_size + M_UD_PADDING));

			thread_list[i] = thread(THREAD_NAME(client), &ibv_res, tmp_buffer, send_size, i);
		}

		for (int i = 0; i < qp_sum; i ++ ) {
			thread_list[i].join();
		}
#else
		client_batch_share_thread(&ibv_res, buffer, send_size, 0);
#endif
	}

}

int main(int argc, char **argv) {

	printf("Test Begin\n");

	test(argc, argv);

	return 0;
}
