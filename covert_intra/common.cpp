
#include <chrono>
#include "common.hpp"
#include "sync.hpp"
#include <iomanip>
#include <fstream>
#define WR_ID_QP_BIT 4
#define WR_ID_QUEUE_BIT 12
#define WR_ID_QP_MASK       0x000000000000000f
#define WR_ID_QUEUE_MASK    0x000000000000fff0
#define WR_ID_TIME_MASK     0xffffffffffff0000

#define OUTPUT_FILE 1

void m_handle_signint(int sig)
{
    std::cout << std::endl << "Keyboard Interrupt" << std::endl;
    m_stop = 1;
}
int agent::init(config_t* config)
{
    int rc = 0;

    struct ibv_device **dev_list = NULL;
    struct ibv_device *ib_dev = NULL;
    int i;
    int num_devices;
    dev_list = ibv_get_device_list(&num_devices);

    if(!dev_list)
    {
        fprintf(stderr, "failed to get IB devices list\n");
        rc = 1;
        ibv_free_device_list(dev_list);
        dev_list = NULL;
        goto Init_exit;
    }

    if(!num_devices)
    {
        fprintf(stderr, "found %d device(s)\n", num_devices);
        rc = 1;
        goto Init_exit;
    }
    
    for(i = 0; i < num_devices; i ++)
    {
        if(config->dev_name.empty())
        {
            config->dev_name = strdup(ibv_get_device_name(dev_list[i]));
        }
        /* find the specific device */
        if(ibv_get_device_name(dev_list[i]) == config->dev_name)
        {
            ib_dev = dev_list[i];
            break;
        }
    }


    if(!ib_dev)
    {
        fprintf(stderr, "IB device %s wasn't found\n", config->dev_name.c_str());
        rc = 1;
        goto Init_exit;
    }

    ib_ctx = ibv_open_device(ib_dev);

    if(!ib_ctx)
    {
        fprintf(stderr, "failed to open device %s\n", config->dev_name.c_str());
        rc = 1;
        goto Init_exit;
    }
    ibv_free_device_list(dev_list);
    dev_list = NULL;
    ib_dev = NULL;

    if(ibv_query_port(ib_ctx, config->ib_port, &port_attr))
    {
        fprintf(stderr, "ibv_query_port on port %u failed\n", config->ib_port);
        rc = 1;
        goto Init_exit;
    }
    // pd
    pd = ibv_alloc_pd(ib_ctx);
    if(!pd)
    {
        fprintf(stderr, "ibv_alloc_pd failed\n");
        rc = 1;
        goto Init_exit;
    }
    // comp_channel
    channel = ibv_create_comp_channel(ib_ctx);
    if (!channel) {
        fprintf(stderr, "failed to create completion channel\n");
        rc = 1;
        goto Init_exit;
    }
    // cq
    cq = ibv_create_cq(ib_ctx, config->cq_size, NULL, NULL, 0);
    if(!cq)
    {
        fprintf(stderr, "failed to create CQ with %u entries\n", config->cq_size);
        rc = 1;
        goto Init_exit;
    }
    // mr
    char* buf;
    ibv_mr *mr;
    for (int i = 0; i < config->mr_num; i++)
    {
        buf = nullptr;
        mr = nullptr;
#if USE_HUGE_PAGES == true
        buf =  (char *)mmap(NULL, (uint)ceil(config->mr_size / 2097152.0) * 2097152,
                           PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB,
                           -1, 0);
#else
        buf = (char *)malloc(config->mr_size);
#endif
        bufs.push_back(buf);
#if USE_HUGE_PAGES == true
        if (buf == MAP_FAILED)
#else
        if (!buf)
#endif
        {
            fprintf(stderr, "failed to malloc %u bytes to memory buffer %d\n", config->mr_size, i);
            rc = 1;
            goto Init_exit;
        }
        memset(buf, 0 , config->mr_size);
        mr = ibv_reg_mr(pd, buf, config->mr_size, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
        if(!mr)
        {
            fprintf(stderr, "ibv_reg_mr %d failed with mr_flags=0x%x\n", i, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
            rc = 1;
            goto Init_exit;
        }
        mrs.push_back(mr);
    }
    // qp
    ibv_qp *qp;
    struct ibv_qp_init_attr qp_init_attr;
    struct ibv_qp_attr attr;
    for (int i = 0; i < config->qp_num; i++)
    {
        qp = nullptr;
        memset(&qp_init_attr, 0, sizeof qp_init_attr);
        qp_init_attr.qp_type = IBV_QPT_RC;
        qp_init_attr.sq_sig_all = 1;
        qp_init_attr.send_cq = cq;
        qp_init_attr.recv_cq = cq;
        qp_init_attr.cap.max_send_wr = config->qp_send_size;
        qp_init_attr.cap.max_recv_wr = config->qp_recv_size;
        qp_init_attr.cap.max_send_sge = MAX_SGE_NUM;
        qp_init_attr.cap.max_recv_sge = MAX_SGE_NUM;

        qp = ibv_create_qp(pd, &qp_init_attr);
        if(!qp)
        {
            fprintf(stderr, "failed to create QP %d\n", i);
            rc = 1;
            goto Init_exit;
        }
        qps.push_back(qp);

        memset(&attr, 0, sizeof(attr));
        attr.qp_state = IBV_QPS_INIT;
        attr.port_num = config->ib_port;
        attr.pkey_index = 0;
        attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
    
        if(ibv_modify_qp(qp, &attr, IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS))
        {
            fprintf(stderr, "failed to modify QP state to INIT\n");
            rc = 1;
            goto Init_exit;
        }
    
    }

    reqs.assign(config->reqs.begin(), config->reqs.end());
    reqs_bit0.assign(config->reqs_bit0.begin(), config->reqs_bit0.end());
    reqs_bit1.assign(config->reqs_bit1.begin(), config->reqs_bit1.end());
    std::cout << reqs.size() << " " << reqs_bit0.size() << " " << reqs_bit1.size() << std::endl;

    ib_port = config->ib_port;
    mr_num = config->mr_num;
    qp_num = config->qp_num;
    req_num = config->req_num;
    req_num_bit0 = config->req_num_bit0;
    req_num_bit1 = config->req_num_bit1;
    mr_size = config->mr_size;
    qp_recv_size = config->qp_recv_size;
    qp_send_size = config->qp_send_size;
    extime = config->extime;
Init_exit:

    if(dev_list)
    {
        ibv_free_device_list(dev_list);
        dev_list = nullptr;
    }
    if (rc == 1)
    {
        this->destroy();
    }
    return rc;
}

int agent::destroy()
{
    for (std::vector<ibv_mr *>::iterator it = mrs.begin(); it != mrs.end(); ++it)
    {
        if(*it)
            ibv_dereg_mr(*it);
    }
    for (std::vector<ibv_qp *>::iterator it = qps.begin(); it != qps.end(); ++it)
    {
        if(*it)
            ibv_destroy_qp(*it);
    }
    for (std::vector<char *>::iterator it = bufs.begin(); it != bufs.end(); ++it)
    {
        if(*it)
#if USE_HUGE_PAGES == true
            if(munmap(*it, (uint)ceil(mr_size / 2097152.0) * 2097152) == -1)
            {
                std::cerr << "munmap failed" << std::endl;
            };
#else
            free(*it);
#endif
    }
    if(channel) ibv_destroy_comp_channel(channel);
    if(cq) ibv_destroy_cq(cq);
    if(pd) ibv_dealloc_pd(pd);
    if(ib_ctx) ibv_close_device(ib_ctx);
    return 0;
}

int agent::connect()
{
    struct ibv_qp_attr attr;
    int rc = 0;
    for (int i = 0; i < qp_num; i++)
    {
        memset(&attr, 0, sizeof(attr));
        attr.qp_state = IBV_QPS_RTR;
        attr.path_mtu = IBV_MTU_256;
        attr.dest_qp_num = this->remote_qp_metadata[i].qpn;
        attr.rq_psn = 0;
        attr.max_dest_rd_atomic = 1;
        attr.min_rnr_timer = 0x12;
        attr.ah_attr.is_global = 0;
        attr.ah_attr.dlid = this->remote_qp_metadata[i].lid;
        attr.ah_attr.sl = 0;
        attr.ah_attr.src_path_bits = 0;
        attr.ah_attr.port_num = this->ib_port;
        attr.ah_attr.is_global = 1;
        attr.ah_attr.port_num = 1;
        memcpy(&attr.ah_attr.grh.dgid, this->remote_qp_metadata[i].gid, 16);
        attr.ah_attr.grh.flow_label = 0;
        attr.ah_attr.grh.hop_limit = 1;
        attr.ah_attr.grh.sgid_index = 0;
        attr.ah_attr.grh.traffic_class = 0;
        if(ibv_modify_qp(qps[i], &attr, IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN |
                IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER))
        {
            fprintf(stderr, "failed to modify QP %d state to RTR\n", i);
            rc = 1;
            goto Connect_exit;
        }
        memset(&attr, 0, sizeof(attr));
        attr.qp_state = IBV_QPS_RTS;
        attr.timeout = 0x12;
        attr.retry_cnt = 6;
        attr.rnr_retry = 0;
        attr.sq_psn = 0;
        attr.max_rd_atomic = 1;
        if(ibv_modify_qp(qps[i], &attr, IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT |
                IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC))
        {
            fprintf(stderr, "failed to modify QP %d state to RTS\n", i);
            rc = 1;
            goto Connect_exit;
        }
    }
    Connect_exit:

        return rc;
    }

int server::sync(int port)
{
    uint mr_recv_size = sizeof(mr_metadata_t) * mr_num;
    uint qp_recv_size = sizeof(qp_metadata_t) * qp_num;
    mr_metadata_t *mr_data_send = (mr_metadata_t *)malloc(mr_recv_size + 1);
    mr_metadata_t *mr_data_recv = (mr_metadata_t *)malloc(mr_recv_size + 1);
    qp_metadata_t *qp_data_send = (qp_metadata_t *)malloc(qp_recv_size + 1);
    qp_metadata_t *qp_data_recv = (qp_metadata_t *)malloc(qp_recv_size + 1);
    int i;
    
    // mr
    for (i = 0; i < mr_num; i++)
    {
        mr_data_send[i].addr = (uint64_t)bufs[i];
        mr_data_send[i].length = (uint32_t) mr_size;
        mr_data_send[i].rkey = mrs[i]->rkey;
    }
    
    sync_server(port, (char *)mr_data_send, (char *)mr_data_recv, mr_recv_size);
    
    for (i = 0; i < mr_num; i++)
    {
        remote_mr_metadata.push_back(mr_data_recv[i]);
    }

    // debug
    std::cout << std::endl;
    std::cout << "local" << std::endl;
    for (i = 0; i < mr_num; i++)
    {
        std::cout << " mr " << i << ", addr = " << mr_data_send[i].addr << ", length = " << mr_data_send[i].length << ", rkey = " << mr_data_send[i].rkey << std::endl;
    }
    std::cout << std::endl;
    std::cout << "remote" << std::endl;
    for (i = 0; i < mr_num; i++)
    {
        std::cout << " mr " << i << ", addr = " << remote_mr_metadata[i].addr << ", length = " << remote_mr_metadata[i].length << ", rkey = " << remote_mr_metadata[i].rkey << std::endl;
    }

    // qp
    union ibv_gid my_gid;
    for (i = 0; i < qp_num; i++)
    {

        qp_data_send[i].qpn = qps[i]->qp_num;
        qp_data_send[i].lid = port_attr.lid;
        ibv_query_gid(ib_ctx, ib_port, 0, &my_gid);
        memcpy(qp_data_send[i].gid, &my_gid, 16);
    }
    
    sync_server(port, (char *)qp_data_send, (char *)qp_data_recv, qp_recv_size);
    
    for (i = 0; i < qp_num; i++)
    {
        remote_qp_metadata.push_back(qp_data_recv[i]);
    }

    // debug
    std::cout << std::endl;
    std::cout << "local" << std::endl;
    for (i = 0; i < qp_num; i++)
    {
        std::cout << " qp " << i << ", qpn = " << qp_data_send[i].qpn << ", lid = " << qp_data_send[i].lid << ", gid = ";
        std::cout << std::hex << std::uppercase << std::setfill('0');
        for (int j = 0; j < 16; j++)
            std::cout << std::setw(2) << (int)qp_data_send[i].gid[j] << " ";
        std::cout << std::endl;
        std::cout.unsetf(std::ios::hex | std::ios::uppercase | std::ios::showbase);
        std::cout << std::setfill(' ');
    }
    std::cout << std::endl;
    std::cout << "remote" << std::endl;
    for (i = 0; i < qp_num; i++)
    {
        std::cout << " qp " << i << ", qpn = " << remote_qp_metadata[i].qpn << ", lid = " << remote_qp_metadata[i].lid << ", gid = ";
        std::cout << std::hex << std::uppercase << std::setfill('0');
        for (int j = 0; j < 16; j++)
            std::cout << std::setw(2) << (int)remote_qp_metadata[i].gid[j] << " ";
        std::cout << std::endl;
        std::cout.unsetf(std::ios::hex | std::ios::uppercase | std::ios::showbase);
        std::cout << std::setfill(' ');    }
    free(mr_data_recv);
    free(mr_data_send);
    free(qp_data_send);
    free(qp_data_recv);
    return 0;

}

int client::sync(const char* addr, int port)
{
    uint mr_recv_size = sizeof(mr_metadata_t) * mr_num;
    uint qp_recv_size = sizeof(qp_metadata_t) * qp_num;
    mr_metadata_t *mr_data_send = (mr_metadata_t *)malloc(mr_recv_size + 1);
    mr_metadata_t *mr_data_recv = (mr_metadata_t *)malloc(mr_recv_size + 1);
    qp_metadata_t *qp_data_send = (qp_metadata_t *)malloc(qp_recv_size + 1);
    qp_metadata_t *qp_data_recv = (qp_metadata_t *)malloc(qp_recv_size + 1);
    int i;
    
    //mr
    for (i = 0; i < mr_num; i++)
    {
        mr_data_send[i].addr = (uint64_t)bufs[i];
        mr_data_send[i].length = mr_size;
        mr_data_send[i].rkey = mrs[i]->rkey;
    }
    sync_client(addr, port, (char *)mr_data_send, (char *)mr_data_recv, mr_recv_size);
    for (i = 0; i < mr_num; i++)
    {
        remote_mr_metadata.push_back(mr_data_recv[i]);
    }

    // debug
    std::cout << std::endl;
    std::cout << "local" << std::endl;
    for (i = 0; i < mr_num; i++)
    {
        std::cout << " mr " << i << ", addr = " << mr_data_send[i].addr << ", length = " << mr_data_send[i].length << ", rkey = " << mr_data_send[i].rkey << std::endl;
    }
    std::cout << std::endl;
    std::cout << "remote" << std::endl;
    for (i = 0; i < mr_num; i++)
    {
        std::cout << " mr " << i << ", addr = " << remote_mr_metadata[i].addr << ", length = " << remote_mr_metadata[i].length << ", rkey = " << remote_mr_metadata[i].rkey << std::endl;
    }

    usleep(100000);

    // qp
    union ibv_gid my_gid;
    for (i = 0; i < qp_num; i++)
    {

        qp_data_send[i].qpn = qps[i]->qp_num;
        qp_data_send[i].lid = port_attr.lid;
        ibv_query_gid(ib_ctx, ib_port, 0, &my_gid);
        memcpy(qp_data_send[i].gid, &my_gid, 16);
    }
    sync_client(addr, port, (char *)qp_data_send, (char *)qp_data_recv, qp_recv_size);

    for (i = 0; i < qp_num; i++)
    {
        remote_qp_metadata.push_back(qp_data_recv[i]);
    }

    // debug
    std::cout << std::endl;
    std::cout << "local" << std::endl;
    for (i = 0; i < qp_num; i++)
    {
        std::cout << " qp " << i << ", qpn = " << qp_data_send[i].qpn << ", lid = " << qp_data_send[i].lid << ", gid = ";
        std::cout << std::hex << std::uppercase << std::setfill('0');
        for (int j = 0; j < 16; j++)
            std::cout << std::setw(2) << (int)qp_data_send[i].gid[j] << " ";
        std::cout << std::endl;
        std::cout.unsetf(std::ios::hex | std::ios::uppercase | std::ios::showbase);
        std::cout << std::setfill(' ');
    }
    std::cout << std::endl;
    std::cout << "remote" << std::endl;
    for (i = 0; i < qp_num; i++)
    {
        std::cout << " qp " << i << ", qpn = " << remote_qp_metadata[i].qpn << ", lid = " << remote_qp_metadata[i].lid << ", gid = ";
        std::cout << std::hex << std::uppercase << std::setfill('0');
        for (int j = 0; j < 16; j++)
            std::cout << std::setw(2) << (int)remote_qp_metadata[i].gid[j] << " ";
        std::cout << std::endl;
        std::cout.unsetf(std::ios::hex | std::ios::uppercase | std::ios::showbase);
        std::cout << std::setfill(' ');    }
    free(mr_data_recv);
    free(mr_data_send);
    free(qp_data_send);
    free(qp_data_recv);
    return 0;
}

int client::send(std::string savefilename)
{
#if OUTPUT_FILE == 1
    // std::time_t now_c = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    // std::strftime(file_name, sizeof(file_name), "%Y-%m-%d-%H-%M-%S.txt", std::localtime(&now_c));

    std::ofstream save_file(savefilename);

#endif
    signal(SIGINT, m_handle_signint);
    int req_id, sge_id;
    struct ibv_send_wr wrs[MAX_REQ_NUM];
    struct ibv_sge sges[MAX_REQ_NUM][MAX_SGE_NUM];

    struct ibv_send_wr wrs_bit0[MAX_REQ_NUM];
    struct ibv_sge sges_bit0[MAX_REQ_NUM][MAX_SGE_NUM];

    struct ibv_send_wr wrs_bit1[MAX_REQ_NUM];
    struct ibv_sge sges_bit1[MAX_REQ_NUM][MAX_SGE_NUM];

    struct ibv_send_wr *bad_wr = NULL;
    struct ibv_wc* wc;
    int req_msg_length, poll_res;
    request_t m_req;
    sge_t m_sge;

    for (req_id = 0; req_id < req_num; req_id++)
    {
        // req_msg_length = 0;
        m_req = reqs[req_id];
        for (sge_id = 0; sge_id < m_req.local_sges.size(); sge_id++)
        {
            m_sge = m_req.local_sges[sge_id];
            memset(&sges[req_id][sge_id], 0, sizeof(ibv_sge));
            sges[req_id][sge_id].addr = (uint64_t) bufs[m_sge.idx] + m_sge.addr_bias;
            sges[req_id][sge_id].lkey = mrs[m_sge.idx]->lkey;
            sges[req_id][sge_id].length = m_sge.length;
            // std::cout << "sge " << req_id << "," << sge_id << " : " << sges[req_id][sge_id].addr << ", " << sges[req_id][sge_id].lkey << ", " << sges[req_id][sge_id].length << std::endl;
            // req_msg_length += m_sge.length;
        }
        memset(&wrs[req_id], 0, sizeof(ibv_send_wr));
        wrs[req_id].num_sge = m_req.local_sges.size();
        wrs[req_id].sg_list = sges[req_id];
        wrs[req_id].wr_id = m_req.local_qp_idx & WR_ID_QP_MASK;
#if SIGNALED
        wrs[req_id].send_flags = IBV_SEND_SIGNALED;
        wrs[req_id].next = nullptr;
#else
        wrs[req_id].send_flags = (req_id == req_num - 1) ? IBV_SEND_SIGNALED : 0;
        wrs[req_id].next = (req_id == req_num - 1) ? nullptr : &wrs[req_id + 1];
#endif
        
        switch (m_req.opcode)
        {
        case OPCODE::WRITE:
            wrs[req_id].opcode = IBV_WR_RDMA_WRITE;
            wrs[req_id].wr.rdma.remote_addr = (uint64_t)remote_mr_metadata[m_req.remote_sge.idx].addr + m_req.remote_sge.addr_bias;
            wrs[req_id].wr.rdma.rkey = remote_mr_metadata[m_req.remote_sge.idx].rkey;
            break;
        case OPCODE::READ:
            wrs[req_id].opcode = IBV_WR_RDMA_READ;
            wrs[req_id].wr.rdma.remote_addr = (uint64_t)remote_mr_metadata[m_req.remote_sge.idx].addr + m_req.remote_sge.addr_bias;
            wrs[req_id].wr.rdma.rkey = remote_mr_metadata[m_req.remote_sge.idx].rkey;
            break;
        case OPCODE::WRITE_IMM:
            wrs[req_id].opcode = IBV_WR_RDMA_WRITE_WITH_IMM;
            wrs[req_id].imm_data = 0x12345678;
            break;
        case OPCODE::FAA:
            wrs[req_id].opcode = IBV_WR_ATOMIC_FETCH_AND_ADD;
            wrs[req_id].wr.atomic.remote_addr = (uint64_t)remote_mr_metadata[m_req.remote_sge.idx].addr + m_req.remote_sge.addr_bias;
            wrs[req_id].wr.atomic.rkey = remote_mr_metadata[m_req.remote_sge.idx].rkey;
            wrs[req_id].wr.atomic.compare_add = 0x00000001;
            break;
        case OPCODE::CAS:
            wrs[req_id].opcode = IBV_WR_ATOMIC_CMP_AND_SWP;
            wrs[req_id].wr.atomic.remote_addr = (uint64_t)remote_mr_metadata[m_req.remote_sge.idx].addr + m_req.remote_sge.addr_bias;
            wrs[req_id].wr.atomic.rkey = remote_mr_metadata[m_req.remote_sge.idx].rkey;
            wrs[req_id].wr.atomic.compare_add = 0x00000000;
            wrs[req_id].wr.atomic.swap = 0xffffffff;
    
            break;
        default:
            fprintf(stderr, "req opcode error\n");
            return 1;
            break;
        }
        // std::cout << wrs[req_id].opcode << ", " << wrs[req_id].num_sge << ", " << wrs[req_id].send_flags << ", " << wrs[req_id].wr.rdma.remote_addr << ", " << wrs[req_id].wr.rdma.rkey << std::endl;
    }


    for (req_id = 0; req_id < req_num_bit0; req_id++)
    {
        // req_msg_length = 0;
        m_req = reqs_bit0[req_id];
        for (sge_id = 0; sge_id < m_req.local_sges.size(); sge_id++)
        {
            m_sge = m_req.local_sges[sge_id];
            memset(&sges_bit0[req_id][sge_id], 0, sizeof(ibv_sge));
            sges_bit0[req_id][sge_id].addr = (uint64_t) bufs[m_sge.idx] + m_sge.addr_bias;
            sges_bit0[req_id][sge_id].lkey = mrs[m_sge.idx]->lkey;
            sges_bit0[req_id][sge_id].length = m_sge.length;
            // std::cout << "sge " << req_id << "," << sge_id << " : " << sges_bit0[req_id][sge_id].addr << ", " << sges_bit0[req_id][sge_id].lkey << ", " << sges_bit0[req_id][sge_id].length << std::endl;
            // req_msg_length += m_sge.length;
        }
        memset(&wrs_bit0[req_id], 0, sizeof(ibv_send_wr));
        wrs_bit0[req_id].num_sge = m_req.local_sges.size();
        wrs_bit0[req_id].sg_list = sges_bit0[req_id];
        wrs_bit0[req_id].wr_id = m_req.local_qp_idx & WR_ID_QP_MASK;
#if SIGNALED
        wrs_bit0[req_id].send_flags = IBV_SEND_SIGNALED;
        wrs_bit0[req_id].next = nullptr;
#else
        wrs_bit0[req_id].send_flags = (req_id == req_num_bit0 - 1) ? IBV_SEND_SIGNALED : 0;
        wrs_bit0[req_id].next = (req_id == req_num_bit0 - 1) ? nullptr : &wrs_bit0[req_id + 1];
#endif
        
        switch (m_req.opcode)
        {
        case OPCODE::WRITE:
            wrs_bit0[req_id].opcode = IBV_WR_RDMA_WRITE;
            wrs_bit0[req_id].wr.rdma.remote_addr = (uint64_t)remote_mr_metadata[m_req.remote_sge.idx].addr + m_req.remote_sge.addr_bias;
            wrs_bit0[req_id].wr.rdma.rkey = remote_mr_metadata[m_req.remote_sge.idx].rkey;
            break;
        case OPCODE::READ:
            wrs_bit0[req_id].opcode = IBV_WR_RDMA_READ;
            wrs_bit0[req_id].wr.rdma.remote_addr = (uint64_t)remote_mr_metadata[m_req.remote_sge.idx].addr + m_req.remote_sge.addr_bias;
            wrs_bit0[req_id].wr.rdma.rkey = remote_mr_metadata[m_req.remote_sge.idx].rkey;
            break;
        case OPCODE::WRITE_IMM:
            wrs_bit0[req_id].opcode = IBV_WR_RDMA_WRITE_WITH_IMM;
            wrs_bit0[req_id].imm_data = 0x12345678;
            break;
        case OPCODE::FAA:
            wrs_bit0[req_id].opcode = IBV_WR_ATOMIC_FETCH_AND_ADD;
            wrs_bit0[req_id].wr.atomic.remote_addr = (uint64_t)remote_mr_metadata[m_req.remote_sge.idx].addr + m_req.remote_sge.addr_bias;
            wrs_bit0[req_id].wr.atomic.rkey = remote_mr_metadata[m_req.remote_sge.idx].rkey;
            wrs_bit0[req_id].wr.atomic.compare_add = 0x00000001;
            break;
        case OPCODE::CAS:
            wrs_bit0[req_id].opcode = IBV_WR_ATOMIC_CMP_AND_SWP;
            wrs_bit0[req_id].wr.atomic.remote_addr = (uint64_t)remote_mr_metadata[m_req.remote_sge.idx].addr + m_req.remote_sge.addr_bias;
            wrs_bit0[req_id].wr.atomic.rkey = remote_mr_metadata[m_req.remote_sge.idx].rkey;
            wrs_bit0[req_id].wr.atomic.compare_add = 0x00000000;
            wrs_bit0[req_id].wr.atomic.swap = 0xffffffff;
    
            break;
        default:
            fprintf(stderr, "req opcode error\n");
            return 1;
            break;
        }
        // std::cout << wrs[req_id].opcode << ", " << wrs[req_id].num_sge << ", " << wrs[req_id].send_flags << ", " << wrs[req_id].wr.rdma.remote_addr << ", " << wrs[req_id].wr.rdma.rkey << std::endl;
    }

    for (req_id = 0; req_id < req_num_bit0; req_id++)
    {
        // req_msg_length = 0;
        m_req = reqs_bit1[req_id];
        for (sge_id = 0; sge_id < m_req.local_sges.size(); sge_id++)
        {
            m_sge = m_req.local_sges[sge_id];
            memset(&sges_bit1[req_id][sge_id], 0, sizeof(ibv_sge));
            sges_bit1[req_id][sge_id].addr = (uint64_t) bufs[m_sge.idx] + m_sge.addr_bias;
            sges_bit1[req_id][sge_id].lkey = mrs[m_sge.idx]->lkey;
            sges_bit1[req_id][sge_id].length = m_sge.length;
            // std::cout << "sge " << req_id << "," << sge_id << " : " << sges_bit1[req_id][sge_id].addr << ", " << sges_bit1[req_id][sge_id].lkey << ", " << sges_bit1[req_id][sge_id].length << std::endl;
            // req_msg_length += m_sge.length;
        }
        memset(&wrs_bit1[req_id], 0, sizeof(ibv_send_wr));
        wrs_bit1[req_id].num_sge = m_req.local_sges.size();
        wrs_bit1[req_id].sg_list = sges_bit1[req_id];
        wrs_bit1[req_id].wr_id = m_req.local_qp_idx & WR_ID_QP_MASK;
#if SIGNALED
        wrs_bit1[req_id].send_flags = IBV_SEND_SIGNALED;
        wrs_bit1[req_id].next = nullptr;
#else
        wrs_bit1[req_id].send_flags = (req_id == req_num_bit0 - 1) ? IBV_SEND_SIGNALED : 0;
        wrs_bit1[req_id].next = (req_id == req_num_bit0 - 1) ? nullptr : &wrs_bit1[req_id + 1];
#endif
        
        switch (m_req.opcode)
        {
        case OPCODE::WRITE:
            wrs_bit1[req_id].opcode = IBV_WR_RDMA_WRITE;
            wrs_bit1[req_id].wr.rdma.remote_addr = (uint64_t)remote_mr_metadata[m_req.remote_sge.idx].addr + m_req.remote_sge.addr_bias;
            wrs_bit1[req_id].wr.rdma.rkey = remote_mr_metadata[m_req.remote_sge.idx].rkey;
            break;
        case OPCODE::READ:
            wrs_bit1[req_id].opcode = IBV_WR_RDMA_READ;
            wrs_bit1[req_id].wr.rdma.remote_addr = (uint64_t)remote_mr_metadata[m_req.remote_sge.idx].addr + m_req.remote_sge.addr_bias;
            wrs_bit1[req_id].wr.rdma.rkey = remote_mr_metadata[m_req.remote_sge.idx].rkey;
            break;
        case OPCODE::WRITE_IMM:
            wrs_bit1[req_id].opcode = IBV_WR_RDMA_WRITE_WITH_IMM;
            wrs_bit1[req_id].imm_data = 0x12345678;
            break;
        case OPCODE::FAA:
            wrs_bit1[req_id].opcode = IBV_WR_ATOMIC_FETCH_AND_ADD;
            wrs_bit1[req_id].wr.atomic.remote_addr = (uint64_t)remote_mr_metadata[m_req.remote_sge.idx].addr + m_req.remote_sge.addr_bias;
            wrs_bit1[req_id].wr.atomic.rkey = remote_mr_metadata[m_req.remote_sge.idx].rkey;
            wrs_bit1[req_id].wr.atomic.compare_add = 0x00000001;
            break;
        case OPCODE::CAS:
            wrs_bit1[req_id].opcode = IBV_WR_ATOMIC_CMP_AND_SWP;
            wrs_bit1[req_id].wr.atomic.remote_addr = (uint64_t)remote_mr_metadata[m_req.remote_sge.idx].addr + m_req.remote_sge.addr_bias;
            wrs_bit1[req_id].wr.atomic.rkey = remote_mr_metadata[m_req.remote_sge.idx].rkey;
            wrs_bit1[req_id].wr.atomic.compare_add = 0x00000000;
            wrs_bit1[req_id].wr.atomic.swap = 0xffffffff;
    
            break;
        default:
            fprintf(stderr, "req opcode error\n");
            return 1;
            break;
        }
        // std::cout << wrs[req_id].opcode << ", " << wrs[req_id].num_sge << ", " << wrs[req_id].send_flags << ", " << wrs[req_id].wr.rdma.remote_addr << ", " << wrs[req_id].wr.rdma.rkey << std::endl;
    }
    std::cout << wrs[0].opcode << ", " << wrs[0].num_sge << ", " << wrs[0].send_flags << ", " << wrs[0].wr.rdma.remote_addr << ", " << wrs[0].wr.rdma.rkey << ", " << wrs[0].sg_list[0].addr << std::endl;
    std::cout << wrs_bit0[0].opcode << ", " << wrs_bit0[0].num_sge << ", " << wrs_bit0[0].send_flags << ", " << wrs_bit0[0].wr.rdma.remote_addr << ", " << wrs_bit0[0].wr.rdma.rkey << ", " << wrs_bit0[0].sg_list[0].addr << std::endl;
    std::cout << wrs_bit1[0].opcode << ", " << wrs_bit1[0].num_sge << ", " << wrs_bit1[0].send_flags << ", " << wrs_bit1[0].wr.rdma.remote_addr << ", " << wrs_bit1[0].wr.rdma.rkey << ", " << wrs_bit1[0].sg_list[0].addr << std::endl;
    
    uint64_t req_send_cnt = 0;
    uint64_t req_comp_cnt = 0;
    wc = (ibv_wc *)malloc(sizeof(ibv_wc) * qp_send_size);
    int *req_in_queue = (int *)malloc(sizeof(int) * qp_num);
    memset(req_in_queue, 0, sizeof(int) * qp_num);
    int i;
    uint64_t time_last_ns = 0, time_this_ns = 0, poll_time, time_start = 0;
    int step = 3;
    printf("begin\n");
    ibv_send_wr *wr_dy;
    std::vector<request_t>* req_dy;
    uint req_num_dy;
    uint this_mod = 0, last_mod = 2;
    time_start = rdtsc();
    while (!m_stop)
    {
        for (req_id = 0; req_id < req_num; req_id++)
        {
            do
            {
                poll_res = 0;
                if (req_in_queue[reqs[req_id].local_qp_idx] > 0)
                    poll_res = ibv_poll_cq(cq, qp_send_size, wc);
                if (poll_res > 0)
                {
                    poll_time = rdtsc();
                }
                for (i = 0; i < poll_res; i++)
                {
                    // if(wc[i].status != IBV_WC_SUCCESS)
                    //     std::cout << wc[i].status << " " << (wc[i].wr_id & WR_ID_QP_MASK) << std::endl;
                    req_in_queue[wc[i].wr_id & WR_ID_QP_MASK] -= 1;
                    if(wc[i].status)
                        std::cout << "wce status error" << wc[i].status << std::endl;

#if OUTPUT_FILE == 1
                    if(req_comp_cnt % step == 0)
                        save_file << poll_time - time_start << " " << (wc[i].wr_id & WR_ID_QP_MASK) << " " << (poll_time - time_start - (wc[i].wr_id >> (WR_ID_QP_BIT + WR_ID_QUEUE_BIT))) << " " << ((wc[i].wr_id & WR_ID_QUEUE_MASK) >> WR_ID_QP_BIT) << std::endl;
#endif
                    req_comp_cnt += 1;
                }
                if (poll_res < 0)
                {
                    fprintf(stderr, "poll cq error\n");
                    return 1;
                }

                if(m_stop)
                    break;
            } while ((req_in_queue[reqs[req_id].local_qp_idx] >= qp_send_size));
            
            
            if(m_stop)
                break;

            wrs[req_id].wr_id = ((rdtsc() - time_start) << (WR_ID_QP_BIT + WR_ID_QUEUE_BIT)) | ((req_in_queue[reqs[req_id].local_qp_idx] << WR_ID_QP_BIT) & WR_ID_QUEUE_MASK) | (reqs[req_id].local_qp_idx & WR_ID_QP_MASK);
            if (ibv_post_send(qps[reqs[req_id].local_qp_idx], &wrs[req_id], &bad_wr))
            {
                fprintf(stderr, "post send error\n");
                return 1;
            }
            req_in_queue[reqs[req_id].local_qp_idx]++;
            
            req_send_cnt++;
            // printf("req_in_queue = %d\n", req_in_queue);
            // usleep(10);
// #if SIGNALED
            
//             if(req_send_cnt % step == 0)
//             {
//                 time_this_ns = rdtsc() - time_start;

// #if OUTPUT_FILE == 1
                

// #else
//                 if(time_last_ns != 0)
//                 {
//                     std::cout << req_send_cnt << " " << req_comp_cnt << " : ";
//                     std::cout << (time_this_ns - time_last_ns) << "ns, " << step/(double)(time_this_ns - time_last_ns) * 1000 << "Mrps" << std::endl;
//                 }
// #endif

//                 time_last_ns = time_this_ns;
                
//             }
        
// #endif
        }
        this_mod = (uint64_t)((rdtsc() - time_start) / extime) % 2;
        if(this_mod != last_mod)
        {
            // std::cout << "exchange to mod " << this_mod << std::endl;
            if (this_mod)
            {
                wr_dy = wrs_bit1;
                req_dy = &reqs_bit1;
                req_num_dy = req_num_bit1;
            }
            else
            {
                wr_dy = wrs_bit0;
                req_dy = &reqs_bit0;
                req_num_dy = req_num_bit0;
            }
            last_mod = this_mod;
        }

        for (req_id = 0; req_id < req_num_dy; req_id++)
        {
            do
            {
                poll_res = 0;
                if (req_in_queue[(*req_dy)[req_id].local_qp_idx] > 0)
                    poll_res = ibv_poll_cq(cq, qp_send_size, wc);

                if (poll_res > 0)
                {
                    poll_time = rdtsc();

                }
                for (i = 0; i < poll_res; i++)
                {
                    if(wc[i].status)
                        std::cout << wc[i].status << " " << (wc[i].wr_id & WR_ID_QP_MASK) << std::endl;
                    req_in_queue[wc[i].wr_id & WR_ID_QP_MASK] -= 1;
#if OUTPUT_FILE == 1
                    if(req_comp_cnt % step == 0)
                        save_file << poll_time - time_start << " " << (wc[i].wr_id & WR_ID_QP_MASK) << " " << (poll_time - time_start - (wc[i].wr_id >> (WR_ID_QP_BIT + WR_ID_QUEUE_BIT))) << " " << ((wc[i].wr_id & WR_ID_QUEUE_MASK) >> WR_ID_QP_BIT) << std::endl;
#endif              
                    req_comp_cnt += 1;
                }
                if (poll_res < 0)
                {
                    fprintf(stderr, "poll cq error\n");
                    return 1;
                }

                if(m_stop)
                    break;
            } while ((req_in_queue[(*req_dy)[req_id].local_qp_idx] >= qp_send_size));
            
            
            if(m_stop)
                break;
            wr_dy[req_id].wr_id = ((rdtsc() - time_start) << (WR_ID_QP_BIT + WR_ID_QUEUE_BIT)) | ((req_in_queue[(*req_dy)[req_id].local_qp_idx] << WR_ID_QP_BIT) & WR_ID_QUEUE_MASK) | ((*req_dy)[req_id].local_qp_idx & WR_ID_QP_MASK);
            
            if (ibv_post_send(qps[(*req_dy)[req_id].local_qp_idx], &wr_dy[req_id], &bad_wr))
            {
                fprintf(stderr, "post send error\n");
                return 1;
            }
            req_in_queue[(*req_dy)[req_id].local_qp_idx]++;
            
            req_send_cnt++;
            // printf("req_in_queue = %d\n", req_in_queue);
            // usleep(10);
// #if SIGNALED
            
//             if(req_send_cnt % step == 0)
//             {
//                 time_this_ns = rdtsc() - time_start;

// #if OUTPUT_FILE == 1
                
// #else
//                 if(time_last_ns != 0)
//                 {
//                     std::cout << req_send_cnt << " " << req_comp_cnt << " : ";
//                     std::cout << (time_this_ns - time_last_ns) << "ns, " << step/(double)(time_this_ns - time_last_ns) * 1000 << "Mrps" << std::endl;
//                 }
// #endif

//                 time_last_ns = time_this_ns;
                
//             }
        
// #endif
        }


#if !SIGNALED
        if(ibv_poll_cq(cq, qp_send_size, wc) < 0)
        {
            fprintf(stderr, "poll cq error\n");
            return 1;
        }
#endif
    }
    free(wc);
    free(req_in_queue);

#if OUTPUT_FILE == 1
    save_file.close();
#endif

    return 0;
}

int server::recv()
{
    signal(SIGINT, m_handle_signint);
    while(!m_stop)
    {
        usleep(100000);
    }
    return 0;
}