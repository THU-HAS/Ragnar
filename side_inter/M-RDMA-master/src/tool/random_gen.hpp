#ifndef RANDOM_GEN_H
#define RANDOM_GEN_H

#include <algorithm>
#include <ctime>
#include <random>
#include <vector>
#include <fstream>
#include <iostream>
#include <cassert>

#include "utils.hpp"

class RandomGen {
public:
	std::random_device req_rd;
	int *random_req_data;
	std::vector<int> shuffle_req_data;
	int shuffle_req_data_len;
	int shu_counter;
	int ran_counter;
	int max_len;
	bool use_skew;
	RandomGen(int max_len_): max_len(max_len_), ran_counter(0) {
		char *res = getenv("USE_SKEW");
		if (res != nullptr) {
			use_skew = (atoi(res) == 0 ? false : true);
		}
		if (use_skew) {
			REDLOG("[USE SKEW DATA]\n");
			skew();
		} else {
			REDLOG("[USE UNIFORM DATA]\n");
			uniform();
		}
	}
	RandomGen(int max_len_, int max_level, double P): max_len(max_len_), ran_counter(0), shu_counter(0) {

	}
	~RandomGen() {

	}

	void gen_shuffle_req_data(int max_gen_num) {
		shuffle_req_data.resize(max_gen_num);
		// std::mt19937 req_dis(req_rd());
		shuffle_req_data_len = max_gen_num;
		shu_counter = 0;
		for (int i = 1 ; i <= max_gen_num; i ++ ) {
			shuffle_req_data[i - 1] = i;
		}
		std::random_shuffle(shuffle_req_data.begin(), shuffle_req_data.end());
	}
	uint64_t get_shuffle_req_num() {
		assert(shu_counter < shuffle_req_data_len);
		return static_cast<uint64_t>(shuffle_req_data[shu_counter ++]);
	}

    uint64_t get_shuffle_req_num_inf() {
		return static_cast<uint64_t>(shuffle_req_data[shu_counter ++ % max_len]);
	}

	uint64_t get_ran_op() {
		assert(shu_counter + 1 < shuffle_req_data_len);
		return static_cast<uint64_t>(shuffle_req_data[shu_counter + 1]);
	}

	void skew() {
		random_req_data = new int[max_len];
		std::ifstream myfile;
	    myfile.open("./tool/YCSB/src/data/0.dat");
	    long temp;
		for (int i = 0; i < max_len; i ++ ) {
			myfile >> temp;
			random_req_data[i] = (int)(temp % (1 << 30));	
		}	
	}
	void uniform() {

		random_req_data = new int[max_len];
		for (int i = 0; i < max_len; i ++ ) {
			random_req_data[i] = req_rd() % (1 << 30); 
		}	
	}

	forceinline int get_num() {
		ran_counter ++;
		ran_counter %= max_len;
		return random_req_data[ran_counter ];
	}
	forceinline int get_num(int mod) {
		ran_counter ++;
		ran_counter %= max_len;
		return random_req_data[ran_counter ] % mod;
	}
};

#endif