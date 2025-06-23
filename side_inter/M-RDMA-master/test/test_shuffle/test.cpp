#include <getopt.h>
#include <cstdio>
#include <cstdint>
#include <cstdlib>

#include "src/app/distributed_shuffle.hpp"

using namespace shuffle;

void test_all(int argc, char **argv) {

    int choose;
	int src_id = -1;
	int shuffle_num = 1;
	int batch_size = 1;
	char option;
	char *server_name;

	while((choose = getopt(argc, argv, "b:s:n:h")) != -1) {
		switch(choose) {
			case 'b':
				batch_size = atoi(optarg);
				break;
			case 's':
				src_id = atoi(optarg);
				break;
			case 'n':
				shuffle_num = atoi(optarg);
				break;
			case 'h':
				printf("usage: -f/b -s [server] -k [src_id] -t [front num]\n");
				break;
			default:
				printf("usage: -f/b -s [server] -k [src_id] -t [front num]\n");
				exit(1);
		}
	}

	if (src_id == 0) {
		m_memc_server_start();
	}

    auto sn = new shuffle_node(src_id, shuffle_num, batch_size);

	sn->shuffle_run();

	if (src_id == 0) {
		m_memc_server_stop();
	}
}

int main(int argc, char **argv)
{
    /* code */
	test_all(argc, argv);

    return 0;
}