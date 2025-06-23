#include "common.hpp"
#include <iostream>
#include "parse_config.hpp"

volatile sig_atomic_t m_stop = 0;

std::string replace_filename(const std::string& original_path, const std::string& new_filename) {
    size_t last_slash_pos = original_path.find_last_of("/\\");
    if (last_slash_pos == std::string::npos) {
        return new_filename + "_" + original_path;
    }
    std::string directory = original_path.substr(0, last_slash_pos + 1);
    std::string filename = original_path.substr(last_slash_pos + 1);
    
    return directory + new_filename + "_" + filename;
}

void run_server(int port, config_t &config)
{
    server m_server;

    m_server.init(&config);
    m_server.sync(port);
    m_server.connect();
    m_server.recv();
    m_server.destroy();
}
void run_client(const char* addr, int port, config_t &config, std::string savefilename)
{
    client m_client;
            
    m_client.init(&config);
    m_client.sync(addr, port);
    m_client.connect();
    m_client.send(savefilename);
    m_client.destroy();
}
void helpinfo(char* argv[])
{
    std::cerr << "Usage: " << argv[0] << " server <socket_port> [config_file]" << std::endl;
    std::cerr << "Usage: " << argv[0] << " client <server_addr> <socket_port> [config_file]" << std::endl;
    std::cerr << "### config_file is 'config' as default" << std::endl;
    std::cerr << "# CONFIG FORMAT\n### ibv_dev_name ib_port mr_size cq_size qp_recv_size qp_send_size mr_num qp_num" << std::endl;
    std::cerr << "### optype local_qpn remote_qpn local_sge_num sge1 sge2 ... sgeN remote_sge" << std::endl;
    std::cerr << "# SGE FORMAT\n### idx_length_addrbias" << std::endl;
}

int main(int argc, char* argv[]) {

    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(CORE_AFFI, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);

    if (argc < 3 || argc > 5) {
        helpinfo(argv);
        return 1;
    }
    bool is_server;

    std::string input(argv[1]);
    if(input == "server")
        is_server = true;
    else if(input == "client")
        is_server = false;
    else
    {
        std::cerr << "Unknown command: " << input << std::endl;
        return 1;
    }
    int port;
    std::string addr;
    std::string config_file;

    if (is_server)
    {
        if(argc != 3 && argc != 4)
        {
            helpinfo(argv);
            return 1;
        }
        port = std::atoi(argv[2]);
        if (argc == 4)
            config_file = argv[3];
        else
            config_file ="config";
    }
    else
    {
        if(argc != 4 && argc != 5)
        {
            helpinfo(argv);
            return 1;
        }
        addr = argv[2];
        port = std::atoi(argv[3]);
        if (argc == 5)
            config_file = argv[4];
        else
            config_file ="config";
    }


    std::ifstream file(config_file);

    if (!file.is_open()) {
        throw std::runtime_error("Cannot open config " + config_file);
    }

    config_t config = readConfigFromFile(file);
    std::vector<request_t> reqs = readRequestsFromFile(file);
    
    config.req_num = (uint)reqs.size();
    config.reqs = reqs;


    if (is_server)
    {
        run_server(port, config);
    }
    else
    {
        run_client(addr.c_str(), port, config, replace_filename(config_file, "result"));
    }


    return 0;
}