#ifndef PT_COMMON_HPP
#define PT_COMMON_HPP
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>
#include <endian.h>
#include <byteswap.h>
#include <getopt.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <infiniband/verbs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <vector>
#include <iterator>
#include <csignal>

#include <sys/mman.h>
#include <fcntl.h>
#include <cmath>
#include <sched.h>

#define USE_HUGE_PAGES true
#define MAX_SGE_NUM 20
#define MAX_REQ_NUM 100
#define SIGNALED 1
#define CORE_AFFI 15

extern volatile sig_atomic_t m_stop;

void m_handle_signint(int sig);

static inline uint64_t rdtsc()
{
    uint32_t low, high;
    __asm__ volatile("rdtsc" : "=a"(low), "=d"(high));
    return (uint64_t)high << 32 | low;
}

enum OPCODE
{
    WRITE,
    READ,
    WRITE_IMM,
    FAA,
    CAS,
};

typedef struct __attribute__((packed)) _mr_metadata_t
{
    uint64_t addr;
    uint32_t length;
    uint32_t rkey;

} mr_metadata_t;

typedef struct __attribute__((packed)) _qp_metadata_t
{
    uint32_t qpn;
    uint16_t lid;
    uint8_t gid[16];
} qp_metadata_t;

typedef struct _sge_t
{
    int idx;
    int length;
    int addr_bias;
} sge_t;

typedef struct _request_t
{
    enum OPCODE opcode;
    int local_qp_idx;
    int remote_qp_idx;
    std::vector<sge_t> local_sges;
    sge_t remote_sge;
} request_t;

typedef struct _config_t
{
    std::string dev_name;
    uint ib_port;
    uint mr_size;
    uint cq_size;
    uint qp_recv_size;
    uint qp_send_size;
    uint mr_num;
    uint qp_num;
    uint req_num;
    uint req_num_bit0;
    uint req_num_bit1;
    uint extime;
    std::vector<request_t> reqs;
    std::vector<request_t> reqs_bit0;
    std::vector<request_t> reqs_bit1;
} config_t;

class agent
{
    protected:
        struct ibv_port_attr port_attr;
        struct ibv_context *ib_ctx = nullptr;
        struct ibv_comp_channel *channel = nullptr;
        struct ibv_pd *pd = nullptr;
        struct ibv_cq *cq = nullptr;
        uint ib_port;
        uint mr_num;
        uint qp_num;
        uint req_num;
        uint req_num_bit0;
        uint req_num_bit1;
        uint mr_size;
        uint qp_recv_size;
        uint qp_send_size;
        uint extime;
        std::vector<char *> bufs;
        std::vector<ibv_mr *> mrs;
        std::vector<ibv_qp *> qps;
        std::vector<request_t> reqs;
        std::vector<request_t> reqs_bit0;
        std::vector<request_t> reqs_bit1;
        std::vector<mr_metadata_t> remote_mr_metadata;
        std::vector<qp_metadata_t> remote_qp_metadata;
        
    public:
        int init(config_t *config);
        int connect();
        int destroy();
};

class client: public agent
{
    public:
        int sync(const char* addr, int port);
        int send(std::string savefilename);
};

class server: public agent
{
    public:
        int sync(int port);
        int recv();
};

#endif