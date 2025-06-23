#include <stdio.h>
#include <iostream>
#include <random>
#include <chrono>
#include <time.h> 
#include <sys/shm.h>

#include "src/tool/utils.hpp"

#define USE_LESS_MAX_SGE

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
//	#define USE_SEQ
	#define USE_RAND
#endif

//#define USE_HUGEPAGE

const int poll_cyc = 1010;
const int queue_depth = 1000;
const int test_number = 10000000;
random_device req_rd;

void test_seq_random(int argc, char **argv) {
	struct m_ibv_res ibv_res;
	FILL(ibv_res);
	srand( (unsigned)time( NULL ) );
	const long long buffer_size_all = 1LL << 25;
//	const int buffer_size_all = 1 << 30;
//	const int buffer_size_all = 1 << 25;
#ifdef USE_HUGEPAGE
    int sid = shmget(0x666, buffer_size_all, SHM_HUGETLB | 0666);
    char *buffer = (char *) shmat(sid, 0, 0);
#else
	char *buffer = new char[buffer_size_all];
#endif
	//int m_buffer_size = 128 * 64;
	const char *server;
	int send_size;
	long long send_iter;
	int max_iter;
	int unsig_size = 32;
	ibv_res.is_server = 1;
	char option;
	int src_is_rand = 0;
	int dst_is_rand = 0;
	int choose = '0';

	while((choose = getopt(argc, argv, "abrwhi:s:d:p:")) != -1) {
		switch(choose) {
			case 'r':
				option = 'r';
				break;
			case 'w':
				option = 'w';
				break;
			case 'i':
				send_iter = atoll(optarg);
				break;
			case 's':
				send_size = atoi(optarg);
				max_iter = buffer_size_all / send_size - 10;
				break;
			case 'd':
				server = optarg;
				ibv_res.is_server = 0;
				printf("[IS SERVER]\n");
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

	assert(option != '0');

	m_init_parameter(&ibv_res, 1, 7089, 0xdeadbeaf, M_RC, 1);

	m_open_device_and_alloc_pd(&ibv_res);

	m_get_is_odp(&ibv_res);	

	m_reg_buffer_lite(&ibv_res, buffer, buffer_size_all);

	m_create_cq_and_qp(&ibv_res, poll_cyc, IBV_QPT_RC);

	m_sync(&ibv_res, server, buffer);

	m_modify_qp_to_rts_and_rtr(&ibv_res);

	if (ibv_res.is_server) {
		
		sleep(400);
//		m_post_recv(&ibv_res, buffer, send_size, 0);

//		m_poll_recv_cq_lazy(&ibv_res, 100000, 0);
//		printf("[BUFFER]%s\n", buffer);

	} else {
		memset(buffer, '-', buffer_size_all);
		sleep(1);
		auto start = chrono::system_clock::now();
		for (long long i = 0; i < send_iter; i ++ ) {
			
//#if (defined USE_SEQ)
//#elif (defined USE_RAND)
//#endif
			long long src_offset;
			long long dst_offset;

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

int main(int argc, char **argv) {

	printf("Test Begin\n");

	test_seq_random(argc, argv);

	return 0;
}
