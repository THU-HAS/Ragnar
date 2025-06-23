#include <stdio.h>
#include <iostream>
#include <random>
#include <chrono>
#include <time.h> 

#include "src/tool/utils.hpp"

extern "C"{

#include "src/libmrdma/libmrdma.h"
#include "src/libmrdma/dynamic_conn_ud.h"
}

using namespace std;

// #define USE_MULTI_CTX
// #define USE_UD_CONN
//#define USE_TEST_REG
#define USE_TEST_SEQ_RANDOM
#ifdef USE_TEST_SEQ_RANDOM
	#define USE_SEQ
//	#define USE_RAND
#endif

const long ud_buffer_size = 1L << 10;
const long buffer_size = 1L << 10;
const long data_buffer_size = 1L << 14;
const int poll_cyc = 1010;
const int queue_depth = 1000;
const int test_number = 10000000;
const int ITER_NUM = 6000000;
const int ITER_DIS_NUM = 100000;
const int BATCH_SIZE = 64;
const int PUSH_SIZE = 128;
const int UNSIG_SIZE = 64;
const int CONN_ITER = 100;
const int TCASE_ITER = 5;
random_device req_rd;

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

/*
	USE_RAN, SEND_SIZE, BUFFER_BLOCK_SIZE, DEST_IP	
*/

void test_reg_mem(int argc, char **argv) {
	struct m_ibv_res ibv_res;
	FILL(ibv_res);

	int reg_mem_size = atoi(argv[2]);
	int reg_mem_iter = atoi(argv[1]);
	const char *server;

	char *buffer = new char[reg_mem_size * reg_mem_iter];
	//int m_buffer_size = 128 * 64;
	

	m_init_parameter(&ibv_res, 1, 7089, 0xdeadbeaf, M_RC, 1);

	m_open_device_and_alloc_pd(&ibv_res);


	// for (int s = 0; s < 10; s ++ ) {

	auto start = chrono::system_clock::now();	
	for (int i = 0; i < reg_mem_iter; i ++ ) {
		m_reg_external_buffer(&ibv_res, buffer + i * reg_mem_size, reg_mem_size);
	}
	auto end = chrono::system_clock::now();
	std::chrono::duration<double> diff = end-start;
	printf("[TIME]%lf\n", diff.count());

	// 	reg_mem_size *= 2;
	// }
}

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

	m_init_parameter(&ibv_res, 1, 7089, 0xdeadbeaf, M_RC, qp_sum);

	m_open_device_and_alloc_pd(&ibv_res);

	m_get_is_odp(&ibv_res);	

	m_reg_buffer(&ibv_res, buffer, buffer_size);

	m_create_cq_and_qp(&ibv_res, poll_cyc, IBV_QPT_RC);

	m_sync(&ibv_res, server, buffer);

	m_modify_qp_to_rts_and_rtr(&ibv_res);

	if (ibv_res.is_server) {
        
        sleep(20);

		m_post_recv(&ibv_res, buffer, send_size, 0);

		m_poll_recv_cq_lazy(&ibv_res, 1000, 0);
		// sleep(5);

		printf("[BUFFER]%s\n", buffer);
		//m_poll_send_cq(&ibv_res);
	} else {


        sleep(20);
		memset(buffer, '-', buffer_size);
		//				buffer[32] = 'c';
		//				buffer[31] = 'd';
		sleep(1);
		auto start = chrono::system_clock::now();
		for (int i = 0; i < ITER_NUM / qp_sum; i ++ ) {
			for (int j = 0; j < qp_sum; j ++ ) {
				// printf("%d %d\n", i, j);
				if (i % UNSIG_SIZE == 0) {

					m_post_write_offset_sig(&ibv_res, buffer, send_size, 0, j);

					m_poll_send_cq(&ibv_res, j);
				} else {
					m_post_write(&ibv_res, buffer, send_size, j);
				}
			}
			//if (i % BATCH_SIZE == 0) m_poll_send_cq(&ibv_res);
		}


		auto end = chrono::system_clock::now();
		std::chrono::duration<double> diff = end-start;
		printf("[TIME]%lf\n", diff.count());

		m_post_write_offset_sig_imm(&ibv_res, buffer, send_size, 0, 0ULL, 0);

	}

}

void test_seq_random(int argc, char **argv) {
	struct m_ibv_res ibv_res;
	FILL(ibv_res);
	srand( (unsigned)time( NULL ) );

//	const int buffer_size_all = 1 << 25;
	const int buffer_size_all = 1 << 30;

	char *buffer = new char[buffer_size_all];
	//int m_buffer_size = 128 * 64;
	const char *server;
	int send_size;
	int send_iter;
	int max_iter;
	int unsig_size = 1;
	ibv_res.is_server = 1;
	char option;
	int src_is_rand = 0;
	int dst_is_rand = 0;
	int choose;

	while((choose = getopt(argc, argv, "abrwhi:s:d:p:")) != -1) {
		switch(choose) {
			case 'r':
				option = 'r';
				break;
			case 'w':
				option = 'w';
				break;
			case 'i':
				send_iter = atoi(optarg);
				break;
			case 's':
				send_size = atoi(optarg);
				max_iter = buffer_size_all / send_size - 10;
				break;
			case 'd':
				server = optarg;
				ibv_res.is_server = 0;
				break;
			case 'p':
				unsig_size = atoi(optarg);
				break;
			case 'b':
				dst_is_rand = 1;
				break;
			case 'a':
				src_is_rand = 1;
				break;
			case 'h':
				printf("usage: -w/r -i [iterations] -s [send size] -d [destination] -p [post list]\n");
				return;
			default:
				printf("usage: -a (is_rand) -w/r -i [iterations] -s [send size] -d [destination] -p [post list]\n");
				exit(1);
		}
	}

	m_init_parameter(&ibv_res, 1, 7089, 0xdeadbeaf, M_RC, 1);

	m_open_device_and_alloc_pd(&ibv_res);

	m_get_is_odp(&ibv_res);	

	m_reg_buffer(&ibv_res, buffer, buffer_size_all);

	m_create_cq_and_qp(&ibv_res, poll_cyc, IBV_QPT_RC);

	m_sync(&ibv_res, server, buffer);

	m_modify_qp_to_rts_and_rtr(&ibv_res);


	if (ibv_res.is_server) {
		
        printf("Begin Server\n");
		sleep(20);
//		m_post_recv(&ibv_res, buffer, send_size, 0);

//		m_poll_recv_cq_lazy(&ibv_res, 100000, 0);
//		printf("[BUFFER]%s\n", buffer);

	} else {

        printf("Begin Client\n");
        sleep(10);

		memset(buffer, '-', buffer_size);
		sleep(1);
		auto start = chrono::system_clock::now();
		for (int i = 0; i < send_iter; i ++ ) {
			
//#if (defined USE_SEQ)
//#elif (defined USE_RAND)
//#endif
			int src_offset;
			int dst_offset;

			if (src_is_rand) {

				src_offset = rand() % max_iter; 
			} else {

				rand();
				src_offset = i % max_iter;
			}

			if (dst_is_rand) {

				dst_offset = rand() % max_iter; 
			} else {

				rand();
				dst_offset = i % max_iter;
			}

			if (i % unsig_size == 0) {
				if (option == 'w')
					m_post_write_offset_sig(&ibv_res, buffer + src_offset * send_size, send_size, dst_offset * send_size, 0);
				if (option == 'r')
					m_post_read_offset_sig(&ibv_res, buffer + src_offset * send_size, send_size, dst_offset * send_size, 0);
				m_poll_send_cq(&ibv_res, 0);
			} else {
				if (option == 'w')
					m_post_write_offset(&ibv_res, buffer + src_offset * send_size, send_size, dst_offset * send_size, 0);
				if (option == 'r')
					m_post_read_offset(&ibv_res, buffer + src_offset * send_size, send_size, dst_offset * send_size, 0);
			}
			//if (i % BATCH_SIZE == 0) m_poll_send_cq(&ibv_res);
		}


		auto end = chrono::system_clock::now();
		std::chrono::duration<double> diff = end-start;
		printf("[TIME]%lf\n", diff.count());
		printf("[TPUT]%lf\n", send_iter / diff.count());

		m_post_write_offset_sig_imm(&ibv_res, buffer, send_size, 0, 0ULL, 0);

	}

}

void test_reg_duplicate(int argc, char **argv) {

	int ctx_sum = atoi(argv[1]);

	int buffer_size = 1 << 28;

	char *rdma_mem = new char[buffer_size];

	struct m_ibv_res *ibv_res;
	ibv_res = new m_ibv_res[ctx_sum];

	memset(&ibv_res[0], 0, sizeof(m_ibv_res) * ctx_sum);

	for (int i = 0; i < ctx_sum; i ++ ) {

		ibv_res[i].is_server = true;

		m_init_parameter(&ibv_res[i], 1, 7089 + i, 0xdeadbeaf, M_RC, 1);

		m_open_device_and_alloc_pd(&ibv_res[i]);
	}

    m_get_atomic_level(&ibv_res[0]);

	for (int i = 0; i < ctx_sum; i ++ ) {
		
		m_reg_buffer(&ibv_res[i], rdma_mem, buffer_size);
		// m_create_cq_and_qp(&ibv_res[i], poll_cyc, IBV_QPT_RC);
	}
}
/*
	USE_RAN, SEND_SIZE, BUFFER_BLOCK_SIZE, DEST_IP	
*/

void test_multi_ctx(int argc, char **argv) {
	struct m_ibv_res *ibv_res;
	bool is_server;
	FILL(ibv_res);

	char *buffer = new char[buffer_size];
	//int m_buffer_size = 128 * 64;
	const char *server = "";
	if (argc == 4) {
		is_server = 1;	
	} else if (argc == 5) {
		is_server = 0;
		server = argv[4];
		printf("Connect to %s\n", server);
	} else {
		printf("[ERROR] illegal input\n");
	}
	int send_size = atoi(argv[1]);
	int qp_sum = atoi(argv[2]);
	int ctx_sum = atoi(argv[3]);

	ibv_res = new m_ibv_res[ctx_sum];

	for (int i = 0; i < ctx_sum; i ++ ) {

		ibv_res[i].is_server = is_server;

		m_init_parameter(&ibv_res[i], 1, 7089 + i, 0xdeadbeaf, M_RC, qp_sum);

		m_open_device_and_alloc_pd(&ibv_res[i]);
	}
	
	auto start = chrono::system_clock::now();	

	for (int i = 0; i < ctx_sum; i ++ ) {
		
		m_reg_buffer(&ibv_res[i], buffer, buffer_size);

		m_create_cq_and_qp(&ibv_res[i], poll_cyc, IBV_QPT_RC);
	}

	auto end = chrono::system_clock::now();
	std::chrono::duration<double> diff = end-start;
	printf("[TIME]%lf\n", diff.count());


	for (int i = 0; i < ctx_sum; i ++ ) {

		m_sync(&ibv_res[i], server, buffer);
	}

	start = chrono::system_clock::now();	

	for (int i = 0; i < ctx_sum; i ++ ) {
		
		m_modify_qp_to_rts_and_rtr(&ibv_res[i]);
	}

	end = chrono::system_clock::now();
	diff = end-start;
	printf("[TIME]%lf\n", diff.count());

	if (is_server) {

		m_post_recv(&ibv_res[0], buffer, send_size, 0);

		m_poll_recv_cq_lazy(&ibv_res[0], 1000, 0);
		// sleep(5);

		printf("[BUFFER]%s\n", buffer);
		//m_poll_send_cq(&ibv_res);
	} else {
		memset(buffer, '-', buffer_size);
		//				buffer[32] = 'c';
		//				buffer[31] = 'd';
		sleep(1);
		auto start = chrono::system_clock::now();
		for (int i = 0; i < ITER_NUM / qp_sum / ctx_sum + 1; i ++ ) {
			for (int j = 0; j < qp_sum; j ++ ) {
				// printf("%d %d\n", i, j);
				for (int k = 0; k < ctx_sum; k ++ ) {
					if (i % UNSIG_SIZE == 0) {

						m_post_write_offset_sig(&ibv_res[k], buffer, send_size, 0, j);

						m_poll_send_cq(&ibv_res[k], j);
					} else {
						m_post_write(&ibv_res[k], buffer, send_size, j);
					}
				}
			}
			//if (i % BATCH_SIZE == 0) m_poll_send_cq(&ibv_res);
		}


		auto end = chrono::system_clock::now();
		std::chrono::duration<double> diff = end-start;
		printf("[TIME]%lf\n", diff.count());

		m_post_write_offset_sig_imm(&ibv_res[0], buffer, send_size, 0, 0ULL, 0);

	}
}

void test_conn_ud(int argc, char **argv) {
	struct m_ibv_res ibv_res_ud;
	struct m_ibv_res ibv_res_rc;
	FILL(ibv_res_ud);
	FILL(ibv_res_rc);

	char *rc_buffer = new char[data_buffer_size];
	char *ud_buffer = new char[ud_buffer_size];
	//int m_buffer_size = 128 * 64;
	const char *server;
	if (argc == 3) {
		ibv_res_ud.is_server = 1;	
		ibv_res_rc.is_server = 1;	
	} else if (argc == 4) {
		ibv_res_ud.is_server = 0;
		ibv_res_rc.is_server = 0;
		server = argv[3];
		printf("Connect to %s\n", server);
	} else {
		printf("[ERROR] illegal input\n");
	}
	int send_size = atoi(argv[1]);
	int qp_sum = atoi(argv[2]);

// Init RC & UD
	m_init_parameter(&ibv_res_ud, 1, 7089, 0xdeadbeaf, M_UD, 1);
	m_init_parameter(&ibv_res_rc, 1, 7089, 0xdeadbeaf, M_RC, qp_sum);

	m_open_device_and_alloc_pd(&ibv_res_ud);
	m_open_device_and_alloc_pd(&ibv_res_rc);

	m_reg_buffer(&ibv_res_ud, ud_buffer, ud_buffer_size);
	m_reg_buffer(&ibv_res_rc, rc_buffer, data_buffer_size);

// Create UD connection
	m_create_cq_and_qp(&ibv_res_ud, poll_cyc, IBV_QPT_UD);

	m_sync(&ibv_res_ud, server, ud_buffer);

	m_reload_multi_recv(&ibv_res_ud, ud_buffer, CONN_ITER + 2, 0);

	m_modify_qp_to_rts_and_rtr(&ibv_res_ud);

	// Test UD connection
	if (ibv_res_ud.is_server) {

		printf("[Server]\n");

		m_post_recv(&ibv_res_ud, ud_buffer, send_size + M_UD_PADDING, 0);

		printf("[Poll CQ]\n");
		
		m_poll_recv_cq(&ibv_res_ud, 0);
		// sleep(5);

		printf("[BUFFER]%s\n", ud_buffer + M_UD_PADDING);
		//m_poll_send_cq(&ibv_res);
	} else {
		sleep(1);

		printf("[Client]\n");

		memset(ud_buffer, '-', buffer_size);

		m_post_ud_send_sig(&ibv_res_ud, ud_buffer, send_size, 0);

		m_poll_send_cq(&ibv_res_ud, 0);

	}

// Create RC connection via RC
	printf("[Begin Test Conn]\n");
	uint64_t begin = rdtsc();
	for (int tcase = 0; tcase <= TCASE_ITER; tcase ++) {
		
		printf("[T_ITER]%d\n", tcase);		

		for (int t = 0; t < CONN_ITER; t ++ ) {
			
			for (int i = 0; i < qp_sum; i ++ ) {
				m_create_cq_and_qp_by_id(&ibv_res_rc, poll_cyc, IBV_QPT_RC, i);

				printf("[ITER]%d\n", t);
				m_sync_via_ud_by_id(&ibv_res_rc, rc_buffer, i, &ibv_res_ud, ud_buffer, 0);

				m_modify_qp_to_rts_and_rtr_by_id(&ibv_res_rc, i);

				m_destroy_qp_by_id(&ibv_res_rc, i);
			}
		}	

		m_reload_multi_recv(&ibv_res_ud, ud_buffer, CONN_ITER, 0);	
	}
	printf("[Switch] %lu cycles\n", rdtsc() - begin);

	for (int i = 0; i < qp_sum; i ++ ) {
		m_create_cq_and_qp_by_id(&ibv_res_rc, poll_cyc, IBV_QPT_RC, i);

		m_sync_via_ud_by_id(&ibv_res_rc, rc_buffer, i, &ibv_res_ud, ud_buffer, 0);

		m_modify_qp_to_rts_and_rtr_by_id(&ibv_res_rc, i);

	}

	printf("[Begin RC Test]\n");

// Test RC connection
	if (ibv_res_rc.is_server) {

		m_post_recv(&ibv_res_rc, rc_buffer, send_size, 0);

		m_poll_recv_cq_lazy(&ibv_res_rc, 1000, 0);
		// sleep(5);

		printf("[BUFFER]%s\n", rc_buffer);
		//m_poll_send_cq(&ibv_res);
	} else {
		memset(rc_buffer, '+', buffer_size);
		//				buffer[32] = 'c';
		//				buffer[31] = 'd';
		sleep(1);
		auto start = chrono::system_clock::now();
		for (int i = 0; i < ITER_NUM / qp_sum; i ++ ) {
			for (int j = 0; j < qp_sum; j ++ ) {
				printf("%d %d\n", i, j);
				if (i % UNSIG_SIZE == 0) {

					m_post_write_offset_sig(&ibv_res_rc, rc_buffer, send_size, 0, j);

					m_poll_send_cq(&ibv_res_rc, j);
				} else {
					m_post_write(&ibv_res_rc, rc_buffer, send_size, j);
				}
			}
			//if (i % BATCH_SIZE == 0) m_poll_send_cq(&ibv_res);
		}


		auto end = chrono::system_clock::now();
		std::chrono::duration<double> diff = end-start;
		printf("[TIME]%lf\n", diff.count());

		m_post_write_offset_sig_imm(&ibv_res_rc, rc_buffer, send_size, 0, 0ULL, 0);

	}

}


int main(int argc, char **argv) {

	printf("Test Begin\n");

    test(argc, argv);

    exit(0);

#if (defined USE_MULTI_CTX) 
	test_multi_ctx(argc, argv);
#elif (defined USE_UD_CONN)
	test_conn_ud(argc, argv);
#elif (defined USE_TEST_REG)
	test_reg_mem(argc, argv);
#elif (defined USE_TEST_SEQ_RANDOM)
	test_seq_random(argc, argv);
#else
	test_reg_duplicate(argc, argv);
#endif

	return 0;
}
