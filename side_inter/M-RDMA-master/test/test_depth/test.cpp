#include <stdio.h>
#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <atomic>
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

#define USE_DEPTH
// #define USE_POLL_TH

// #define USE_SENDER_POLL


const int poll_cyc = 1010;
const int ITER_NUM = 6000000;
const int max_depth = 64;
const int print_iter = 10000;

random_device req_rd;
int send_size = 32;
int send_iter = 1000000;
int port_num = 7089;
int depth = 1; // Deafult Depth
char *buffer;
struct m_ibv_res ibv_res;
struct m_ibv_res ibv_res_s[8];
struct ibv_wc wc_res[max_depth];
int qp_num = 1; // Default 1 for front-end

void run_test() {
	uint64_t tmp_val;

	// if (port_num == 7089) {
	// 	m_lock(&ibv_res, buffer, 0);
	// }

#if (defined USE_DEPTH)
    int no_sync_num = 0;

	for (int i = 0; i < send_iter; i ++ ) {
        
        // printf("<%d>\n", no_sync_num);
        
        m_post_write_offset_sig_inline(&ibv_res, buffer, sizeof(uint64_t), 0, 0);
        no_sync_num ++;

        // memset(wc_res, 0, sizeof(ibv_wc) * depth);
        
        int ack_num = m_poll_send_cq_and_quit(&ibv_res, no_sync_num, 0);
        no_sync_num -= ack_num;


        while(no_sync_num >= depth) {
            m_poll_send_cq(&ibv_res, 0);
            no_sync_num --;
        }
	}

	fprintf(stdout, "[COUNTER]%lld\n", (long long)tmp_val);	
#elif (defined USE_POLL_TH)

    // atomic<int> no_sync_num = 0;
    atomic<int> i(0);
    atomic<int> j(0);

    thread send_th = thread([&]() {
        while(i < send_iter) {
    
            m_post_write_offset_sig_inline(&ibv_res, buffer, sizeof(uint64_t), 0, 0);
            // printf("[Send]%d\n", static_cast<int>(i));
            i ++;

            if (i % print_iter == 0) {
                printf("[Send]%d, [Poll]%d\n", static_cast<int>(i), static_cast<int>(j));
            }

#ifdef USE_SENDER_POLL
            while (i - j > depth) {
                m_poll_send_cq(&ibv_res, 0);
                // printf("[Poll]%d\n", static_cast<int>(j));
                j ++;
            }
#else
            while (i - j > depth) {
                this_thread::yield();
            }
#endif
            // memset(wc_res, 0, sizeof(ibv_wc) * depth);
        }
    });

    thread poll_th = thread([&]() {
        while(j < send_iter) {
    
            m_poll_send_cq(&ibv_res, 0);
            // printf("[Poll]%d\n", static_cast<int>(j));
            j ++;
            // memset(wc_res, 0, sizeof(ibv_wc) * depth);
        }
    });

    send_th.join();

    poll_th.join();
        
#else

#endif
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

	while((choose = getopt(argc, argv, "b:ht:s:i:e:d:")) != -1) {
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
            case 'd':
                depth = atoi(optarg);
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
		
		sleep(400);
//		m_post_recv(&ibv_res, buffer, send_size, 0);

//		m_poll_recv_cq_lazy(&ibv_res, 100000, 0);
//		printf("[BUFFER]%s\n", buffer);

	} else {
		*(volatile uint64_t *)buffer = UNLOCK;

		fprintf(stdout, "<==== Client Begin ====>\n");

		sleep(1);

		auto start = chrono::system_clock::now();

		fprintf(stdout, "[Mode] %c\n", option);
	
		run_test();

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
