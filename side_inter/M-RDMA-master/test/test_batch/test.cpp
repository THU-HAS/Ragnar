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
}

using namespace std;

// #define USE_RECV
// #define USE_MMAP
// #define USE_SHM
// #define USE_HUGPAGE
#define USE_MEMALIGN

#ifdef USE_HUGPAGE
	#define HUGEPAGE_FLAG SHM_HUGETLB
#else
	#define HUGEPAGE_FLAG 0
#endif

#ifdef USE_SHM
	const int shm_key = 666;
#endif

const int buffer_size_all = 1 << 30;
const int prepared_offset = 1 << 22;
const int poll_cyc = 1010;
const int ITER_NUM = 6000000;
const int max_batch_size = 64;
random_device req_rd;
int batch_size = 32; // default
int unsig_size = 1; // Unused
int send_size = 32;
int send_iter = 1000000;
int port_num = 7089;
char *buffer;
struct m_ibv_res ibv_res;
char *buffers[max_batch_size];
int size_list[max_batch_size];
uint64_t imm_data_arr[max_batch_size];
int qp_num = 1;

void server_run_batch() {

	while (true) {

		uint64_t imm_data = m_poll_recv_cq_with_data(&ibv_res, 0);
	}
}

void client_run_postlist_batch(int index) {

	fprintf(stdout, "[Thread:%d]\n", index);

    for (int i = 0; i < send_iter; i ++ ) {
		
		// printf("F:%d\n", i);

        for (int i = 0; i < batch_size; i ++ ) {
            buffers[i] = buffer + prepared_offset + i * 2 * send_size;
            size_list[i] = send_size;
        }

#ifdef USE_RECV
        m_post_batch_write_with_imm(&ibv_res, buffers, imm_data_arr, (size_t *)size_list, batch_size, 0);
#else
        m_post_batch_write(&ibv_res, buffers, (size_t *)size_list, batch_size, index);
#endif
        m_poll_send_cq(&ibv_res, index);
    }
}

void client_run_sglist_batch(int index) {

    for (int i = 0; i < send_iter; i ++ ) {
    
        for (int i = 0; i < batch_size; i ++ ) {
            buffers[i] = buffer + prepared_offset + i * 2 * send_size;
            size_list[i] = send_size;
        }

#ifdef USE_RECV
		uint64_t imm_data = 0ULL;
        m_post_write_sgl_with_imm(&ibv_res, buffers, size_list, imm_data, batch_size, 0, 0);
#else
        m_post_write_sgl(&ibv_res, buffers, (size_t *)size_list, batch_size, 0, index);
#endif
        m_poll_send_cq(&ibv_res, index);
    }
}

void client_run_software_batch(int index) {

    for (int i = 0; i < send_iter; i ++ ) {

        for (int i = 0; i < batch_size; i ++ ) {
            memcpy(buffer + i * send_size, buffer + prepared_offset + i * 2 * send_size, send_size);
        }

        m_post_write_offset_sig(&ibv_res, buffer, send_size, 0, index);
        m_poll_send_cq(&ibv_res, index);
    }
}

void multi_thread_run(function<void(int)> run) {
	vector<thread> th(qp_num);
	for (int i = 0; i < qp_num; i ++ ) {
		th[i] = thread(run, i);
	}
	for (int i = 0; i < qp_num; i ++ ) {
		th[i].join();
	}
}

void test(int argc, char **argv) {
	FILL(ibv_res);
	srand( (unsigned)time( NULL ) );

//	const int buffer_size_all = 1 << 25;

#if (defined USE_MMAP)
	
#elif (defined USE_SHM)
	int id = shmget(shm_key, buffer_size_all, IPC_CREAT | IPC_EXCL | 0666 | HUGEPAGE_FLAG);
	buffer = (char *)shmat(id, NULL, 0);
	if (buffer == NULL) {
		fprintf(stderr, "[Get Share Memory Error!]\n");
	}
#elif (defined USE_MEMALIGN)
	posix_memalign((void **)&buffer, 4096, buffer_size_all);
#else
	buffer = new char[buffer_size_all];
#endif
	//int m_buffer_size = 128 * 64;
	const char *server;
	ibv_res.is_server = 1;
	char option;
	int choose;

	while((choose = getopt(argc, argv, "b:hi:s:d:p:m:e:t:")) != -1) {
		switch(choose) {
			case 't':
				qp_num = atoi(optarg);
				break;
			case 'e':
				port_num = atoi(optarg);
				break;
			case 'm':
				option = optarg[0];
				break;
			case 'i':
				send_iter = atoi(optarg);
				break;
			case 's':
				send_size = atoi(optarg);
				break;
			case 'd':
				server = optarg;
				ibv_res.is_server = 0;
				break;
			case 'p':
				unsig_size = atoi(optarg);
				break;
			case 'b':
				batch_size = atoi(optarg);
                assert(batch_size <= max_batch_size);
				break;
			case 'h':
				printf("usage: -m (a/b/c) -i [iterations] -s [send size] -d [destination] -p [post list] -b [batch size]\n");
				return;
			default:
				printf("usage: -m (a/b/c) -i [iterations] -s [send size] -d [destination] -p [post list] -b [batch size]\n");
				exit(1);
		}
	}

	m_init_parameter(&ibv_res, 1, port_num, 0xdeadbeaf, M_RC, port_num);

	m_open_device_and_alloc_pd(&ibv_res);

	m_get_is_odp(&ibv_res);	

	m_reg_buffer(&ibv_res, buffer, buffer_size_all);

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

		fprintf(stdout, "[Mode] %c\n", option);
	
        if (option == 'a') {
			function<void(int)> client_run_postlist_batch_ptr = client_run_postlist_batch;

			multi_thread_run(client_run_postlist_batch_ptr);
            // client_run_postlist_batch(0); 
        } else if (option == 'b') {
            function<void(int)> client_run_sglist_batch_ptr = client_run_sglist_batch;

			multi_thread_run(client_run_sglist_batch_ptr);
        } else if (option == 'c') {
			function<void(int)> client_run_software_batch_ptr = 
            client_run_software_batch;

			multi_thread_run(client_run_software_batch_ptr);
        } else {
            fprintf(stderr, "//Option Parameters Unsupport\n");
            exit(-1);
        }

		auto end = chrono::system_clock::now();
		std::chrono::duration<double> diff = end-start;
		printf("[TIME]%lf\n", diff.count());
		printf("[TPUT]%lf\n", send_iter * batch_size / diff.count());

		m_post_write_offset_sig_imm(&ibv_res, buffer, send_size, 0, 0ULL, 0);

	}

}

int main(int argc, char **argv) {

	printf("Test Begin\n");

	test(argc, argv);

	return 0;
}
