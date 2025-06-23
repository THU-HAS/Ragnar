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
const int ITER_NUM = 20000;
const int ITER_DIS_NUM = 100000;
const int BATCH_SIZE = 32;
const int PUSH_SIZE = 128;
const int UNSIG_SIZE = 1;
const int PINGPONG_DIS = 1;
const int DOORBELL_SIZE = 64;
random_device req_rd;
vector<thread> thread_list;

atomic<int> local_seq_num;

void client_thread(m_ibv_res *ibv_res, char *buffer, int send_size, int qp_index) {

	int qp_sum = ibv_res->qp_sum;

	// printf("[%d]Ready!\n", qp_index);

	for (int i = 0; i < ITER_NUM + 100; i ++ ) {
		// printf("%d: %d\n", qp_index, i);

#ifdef USE_PRINT
		if (i % 10000 == 0) {
			printf("%d, %d\n", qp_index, i);
		}
#endif

		m_poll_recv_cq(ibv_res, qp_index);

		m_post_recv(ibv_res, buffer, send_size + M_UD_PADDING, qp_index);

#ifdef USE_ONE_SIDE
		if (i % PINGPONG_DIS == 0) {
			
			if (i % UNSIG_SIZE == 0) {

				m_post_send_sig(ibv_res, buffer, send_size, qp_index);

				m_poll_send_cq(ibv_res, qp_index);
			} else {
				m_post_send(ibv_res, buffer, send_size, 0, qp_index);
			}
		}
#endif
	}
	// sleep(5);

	printf("[BUFFER]%s\n", buffer + M_UD_PADDING);
}

void server_thread(m_ibv_res *ibv_res, char *buffer, int send_size, int qp_index) {
	memset(buffer, '-', buffer_size);
	//				buffer[32] = 'c';
	//				buffer[31] = 'd';
	int qp_sum = ibv_res->qp_sum;

	// printf("[%d]Ready!\n", qp_index);

	sleep(1);
	auto start = chrono::system_clock::now();
	for (int i = 0; i < ITER_NUM; i ++ ) {
		// printf("%d: %d\n", qp_index, i);
#ifdef USE_PRINT
		if (i % 10000 == 0) {
			printf("%d, %d\n", qp_index, i);
		}
#endif
		if (i % UNSIG_SIZE == 0) {

			m_post_send_sig(ibv_res, buffer, send_size, qp_index);

			m_poll_send_cq(ibv_res, qp_index);
		} else {
			m_post_send(ibv_res, buffer, send_size, 0, qp_index);
		}
#ifdef USE_ONE_SIDE
		if (i % PINGPONG_DIS == 0) {

			m_poll_recv_cq(ibv_res, qp_index);


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

void test(int argc, char **argv) {
	struct m_ibv_res ibv_res[10];
	FILL(ibv_res);

	char *buffer[10];
	//int m_buffer_size = 128 * 64;
	const char *server;
	if (argc == 3) {
		for (int i = 0; i < 10; i ++ ) 
			ibv_res[i].is_server = 1;	
	} else if (argc == 4) {
		for (int i = 0; i < 10; i ++ ) 
			ibv_res[i].is_server = 0;
		server = argv[3];
		printf("Connect to %s\n", server);
	} else {
		printf("[ERROR] illegal input\n");
	}
	int send_size = atoi(argv[1]);
	int qp_sum = atoi(argv[2]);

	thread_list = vector<thread>(qp_sum);

	for (int i = 0; i < qp_sum / 2; i ++ ) {
		m_init_parameter(&ibv_res[i], 1, 7089 + i, 0xdeadbeaf, M_RC, 2);

		m_open_device_and_alloc_pd(&ibv_res[i]);

		// m_get_is_odp(&ibv_res);	
		buffer[i] = new char[buffer_size];

		m_reg_buffer(&ibv_res[i], buffer[i], buffer_size);

		m_create_cq_and_qp(&ibv_res[i], poll_cyc, IBV_QPT_RC);

		m_sync(&ibv_res[i], server, buffer[i]);

		for (int j = 0; j < 100; j ++ ) {
			m_post_recv(&ibv_res[i], buffer[i], send_size + M_UD_PADDING, i % 2);
		}
			// m_post_multi_recv(&ibv_res, buffer, send_size + M_UD_PADDING, 100, i);

		m_modify_qp_to_rts_and_rtr(&ibv_res[i]);
	}

	if (ibv_res[0].is_server) {

		// server_batch_thread(&ibv_res, buffer, send_size, 0);
		for (int i = 0; i < qp_sum; i ++ ) {

			char *tmp_buffer = (buffer[i / 2] + 2 * (i % 2) * send_size);

			thread_list[i] = thread(THREAD_NAME(server), &ibv_res[i / 2], tmp_buffer, send_size, i % 2);

		}

		for (int i = 0; i < qp_sum; i ++ ) {
			thread_list[i].join();
		}

	} else {
		// client_batch_thread(&ibv_res, buffer, send_size, 0);
		for (int i = 0; i < qp_sum; i ++ ) {

			char *tmp_buffer = (char *)(buffer[i / 2] + 2 * (i % 2) * (send_size + M_UD_PADDING));

			thread_list[i] = thread(THREAD_NAME(client), &ibv_res[i / 2], tmp_buffer, send_size, i % 2);

			sleep(1);
		}

		for (int i = 0; i < qp_sum; i ++ ) {
			thread_list[i].join();
		}
	}

}

int main(int argc, char **argv) {

	printf("Test Begin\n");

	test(argc, argv);

	return 0;
}
