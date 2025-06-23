#ifndef UTILS_HPP
#define UTILS_HPP

#include <ctime>
#include <climits>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>

static uint64_t
rdtsc()
{
        uint32_t lo, hi;
        __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
        return (((uint64_t)hi << 32) | lo);
}


#define forceinline		inline __attribute__((always_inline))
#define packed          __attribute__((packed))
#define CACHE_LINE_SIZE 64

#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */

void REDLOG(const char *format, ...)
{   
    va_list args;

    char buf1[1000], buf2[1000];
    memset(buf1, 0, 1000);
    memset(buf2, 0, 1000);

    va_start(args, format);
    vsnprintf(buf1, 1000, format, args);

    snprintf(buf2, 1000, "\033[31m%s\033[0m", buf1);
    printf("%s", buf2);
    //write(2, buf2, 1000);
    va_end( args );
}

void GREENLOG(const char *format, ...)
{   
    va_list args;

    char buf1[1000], buf2[1000];
    memset(buf1, 0, 1000);
    memset(buf2, 0, 1000);

    va_start(args, format);
    vsnprintf(buf1, 1000, format, args);

    snprintf(buf2, 1000, "\033[32m%s\033[0m", buf1);
    printf("%s", buf2);
    //write(2, buf2, 1000);
    va_end( args );
}

#define LOG(...)        fprintf(stdout, __VA_ARGS__)

#endif // UTILS_HPP
