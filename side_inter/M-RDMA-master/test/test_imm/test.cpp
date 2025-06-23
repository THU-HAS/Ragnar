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
const int UNSIG_SIZE = 32;
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
        
        //sleep(20);

        if (ibv_req_notify_cq(ibv_res.recv_cq[0], 0)) {

                fprintf(stderr, "FUCK\n");
                //return -1;
        }

        

		//m_post_recv(&ibv_res, buffer, send_size, 0);

		//m_poll_recv_cq_lazy(&ibv_res, 1000, 0);
		// sleep(5);


		printf("[BUFFER]%s\n", buffer);
		//m_poll_send_cq(&ibv_res);
	} else {


        //sleep(20);
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

		m_post_write_offset_sig_imm(&ibv_res, buffer, send_size, 0, 4ULL, 0);

	}

}


int main(int argc, char **argv) {

	printf("Test Begin\n");

    test(argc, argv);

    exit(0);

	return 0;
}
