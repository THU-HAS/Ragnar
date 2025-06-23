#include <stdio.h>
#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <time.h> 
#include <sys/ipc.h>
#include <sys/shm.h>

#include "src/tool/utils.hpp"

extern "C"{

#include "src/libmrdma/libmrdma.h"
#include "src/libmrdma/dynamic_conn_ud.h"
#include "src/libmrdma/spin_lock.h"
}

using namespace std;

// #define USE_RECV
// #define USE_MMAP
// #define USE_SHM
// #define USE_HUGPAGE
#define USE_MEMALIGN
// #define TEST_SEQ

#define TEST_LOCK
// #define USE_BACK_OFF

#ifdef USE_HUGPAGE
	#define HUGEPAGE_FLAG SHM_HUGETLB
#else
	#define HUGEPAGE_FLAG 0
#endif

#ifdef USE_SHM
	const int shm_key = 666;
#endif

const int poll_cyc = 1010;
const int ITER_NUM = 6000000;
random_device req_rd;
int send_size = 32;
int send_iter = 1000000;
int port_num = 7089;
char *buffer;
struct m_ibv_res ibv_res;
struct m_ibv_res ibv_res_s[16];
int qp_num = 1; // Default 1 for front-end

void run_test_lock() {
	uint64_t tmp_val;

	// if (port_num == 7089) {
	// 	m_lock(&ibv_res, buffer, 0);
	// }

	for (int i = 0; i < send_iter; i ++ ) {

#ifdef USE_BACK_OFF
		m_lock_back_off(&ibv_res, buffer, 0);
#else
		m_lock(&ibv_res, buffer, 0);
#endif

#ifdef USE_CHK
		// Critical Part
		m_post_read_offset_sig(&ibv_res, buffer + sizeof(uint64_t), sizeof(uint64_t), sizeof(uint64_t), 0);


		m_poll_send_cq(&ibv_res, 0);

		tmp_val = *(volatile uint64_t *)(buffer + sizeof(uint64_t));

		// printf("tmp_val: %lld\n", (long long)tmp_val);

		*(volatile uint64_t *)(buffer + sizeof(uint64_t)) = tmp_val + 1;

		m_post_write_offset_sig(&ibv_res, buffer + sizeof(uint64_t), sizeof(uint64_t), sizeof(uint64_t), 0);

		m_poll_send_cq(&ibv_res, 0);
#endif

		m_unlock(&ibv_res, buffer, 0);
	}

	fprintf(stdout, "[COUNTER]%lld\n", (long long)tmp_val);	
}

void run_test_sequencer() {

	uint64_t res;

	for (int i = 0; i < send_iter; i ++ ) {
		
		m_deter_post_faa(&ibv_res, buffer, 1ULL, 0, 0);
	}

	fprintf(stdout, "[COUNTER]%lld\n", (long long)res);	
}

void test(int argc, char **argv) {
	FILL(ibv_res);
	srand( (unsigned)time( NULL ) );

//	const int buffer_size_all = 1 << 25;

	buffer = new char[sizeof(uint64_t) * 2]; // One for lock, another for counter

	//int m_buffer_size = 128 * 64;
	const char *server;

	bool is_server = true;
	char option;
	int choose;

	while((choose = getopt(argc, argv, "b:ht:s:i:e:")) != -1) {
		switch(choose) {
			case 't':
				qp_num = atoi(optarg);
				break;
			case 'e':
				port_num = atoi(optarg);
				break;
			case 'i':
				send_iter = atoi(optarg);
				break;
			case 's':
				server = optarg;
				is_server = false;
				ibv_res.is_server = 0;
				break;
			case 'h':
				printf("usage: -t [thread number] -i [iter num] -e [port num] \n");
				return;
			default:
				printf("usage: -t [thread number] -i [iter num] -e [port num] \n");
				exit(1);
		}
	}

	if (is_server) {
		for (int i = 0; i < qp_num; i ++ ) {

			ibv_res_s[i].is_server = 1;

			m_init_parameter(&ibv_res_s[i], 1, port_num + i, 0xdeadbeaf, M_RC, 1);

			m_open_device_and_alloc_pd(&ibv_res_s[i]);			

			m_reg_buffer(&ibv_res_s[i], buffer, sizeof(uint64_t) * 2);

			m_create_cq_and_qp(&ibv_res_s[i], poll_cyc, IBV_QPT_RC);

			m_sync(&ibv_res_s[i], "", buffer);

			m_modify_qp_to_rts_and_rtr(&ibv_res_s[i]);

		}
	} else {

		m_init_parameter(&ibv_res, 1, port_num, 0xdeadbeaf, M_RC, 1);

		m_open_device_and_alloc_pd(&ibv_res);

		m_get_is_odp(&ibv_res);	

		m_reg_buffer(&ibv_res, buffer, sizeof(uint64_t) * 2);

		m_create_cq_and_qp(&ibv_res, poll_cyc, IBV_QPT_RC);

		m_sync(&ibv_res, server, buffer);

		m_modify_qp_to_rts_and_rtr(&ibv_res);
	}


	
	if (is_server) {
		
		fprintf(stdout, "<==== Server Begin ====>\n");

		// sleep(400);
		volatile uint64_t *sig_val = (volatile uint64_t *)(buffer + sizeof(uint64_t));
		while (*sig_val != 2 * qp_num) {
			printf("%d\n", (int)*sig_val);
			sig_val = (volatile uint64_t *)(buffer + sizeof(uint64_t));
			sleep(1);
		}

//		m_post_recv(&ibv_res, buffer, send_size, 0);

//		m_poll_recv_cq_lazy(&ibv_res, 100000, 0);
//		printf("[BUFFER]%s\n", buffer);

	} else {
		*(volatile uint64_t *)buffer = UNLOCK;

		fprintf(stdout, "<==== Client Begin ====>\n");

		sleep(1);
		
		int begin_counter = m_deter_post_faa(&ibv_res, buffer + sizeof(uint64_t), 1ULL, sizeof(uint64_t), 0);

		do {
			begin_counter = m_deter_post_faa(&ibv_res, buffer + sizeof(uint64_t), 0ULL, sizeof(uint64_t), 0);
		} while(begin_counter != qp_num);

		auto start = chrono::system_clock::now();

		fprintf(stdout, "[Mode] %c\n", option);

#if (defined TEST_LOCK)	
		run_test_lock();
#elif (defined TEST_SEQ)
		run_test_sequencer();
#else
		assert(0);
#endif

		m_deter_post_faa(&ibv_res, buffer + sizeof(uint64_t), 1ULL, sizeof(uint64_t), 0);

		auto end = chrono::system_clock::now();
		std::chrono::duration<double> diff = end-start;
		printf("[TIME]%lf\n", diff.count());
		printf("[TPUT]%lf\n", send_iter / diff.count());

		m_post_write_offset_sig_imm(&ibv_res, buffer, send_size, 0, 0ULL, 0);

	}

}

int main(int argc, char **argv) {

	printf("Test Begin\n");

	test(argc, argv);

	return 0;
}
