#include <getopt.h>

#include "src/app/hashtable.hpp"

using namespace hashtable;

void test_all(int argc, char **argv) {

    int choose;
	int key = -1;
	int front_num = 1;
	char option;
	char *server_name;
	int counter_size = 1;

	while((choose = getopt(argc, argv, "fbht:k:s:c:")) != -1) {
		switch(choose) {
			case 'c':
				counter_size = atoi(optarg);
				break;
			case 'f':
				option = 'f';
				break;
			case 'b':
				option = 'b';
				break;
			case 's':
				server_name = optarg;
				break;
			case 'k':
				key = atoi(optarg);
				break;
			case 't':
				front_num = atoi(optarg);
				break;
			case 'h':
				printf("usage: -f/b -s [server] -k [key] -t [front num]\n");
				break;
			default:
				printf("usage: -f/b -s [server] -k [key] -t [front num]\n");
				exit(1);
		}
	}

	if (option == 'f') {

		HashTableFront *front_end;

        front_end = new HashTableFront(key, front_num, counter_size, server_name);


	} else if (option == 'b') {

		m_memc_server_start();

        HashTableBack *back_end = new HashTableBack(key, front_num);

		m_memc_server_stop();

	} else {
		assert(-1);
	}
}

int main(int argc, char **argv)
{
    /* code */
	test_all(argc, argv);

    return 0;
}