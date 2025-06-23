#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <chrono>
#include <sys/types.h>

using namespace std;

static uint64_t rdtsc() {

    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return (((uint64_t)hi << 32) | lo);
}

const int mem_size = (1<<30);

int main(int argc, char **argv) {


    srand48(time(NULL) * getpid());

    int item_size = atoi(argv[1]);
    int iter = mem_size / item_size;

    bool is_random_dst = static_cast<bool>(atoi(argv[2]));
    bool is_random_src = static_cast<bool>(atoi(argv[3]));

    char *mem_src = new char[mem_size];
    char *mem_dst = new char[mem_size];

    uint64_t m_b = rdtsc();

    auto t1 = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iter; i ++ ) {


        int offset_dst, offset_src; 

        if (is_random_dst) {


            offset_dst = rand() % ( mem_size - item_size );
        } else {


            offset_dst = i * item_size % ( mem_size - item_size );
        }

        if (is_random_src) {


            offset_src = rand() % ( mem_size - item_size );
        } else {


            offset_src = i * item_size % ( mem_size - item_size ); 
        }

        memcpy(mem_dst + offset_dst, mem_src + offset_src, item_size);
    }

    auto t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> diff = t2 - t1;

    uint64_t m_e = rdtsc();

    //printf("%lld\n", m_e - m_b);

    printf("%lf\n", (double)mem_size / item_size / 1000000 / diff.count());

    return 0;
}
