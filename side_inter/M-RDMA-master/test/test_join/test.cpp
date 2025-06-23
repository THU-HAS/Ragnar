#include <getopt.h>
#include <cstdio>
#include <cstdint>
#include <cstdlib>

#include "src/app/distributed_join.hpp"

using namespace join;

void test_all(int argc, char **argv) {

    int choose;
	int src_id = -1;
	int join_num = 1;
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
				join_num = atoi(optarg);
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

    auto sn = new join_node(src_id, join_num, batch_size);

	sn->join_run();

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