#ifndef PARSE_CONFIG_HPP
#define PARSE_CONFIG_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <stdexcept>
#include "common.hpp"

OPCODE stringToOpcode(const std::string& str)
{
        OPCODE opcode;
    if (str == "WRITE") {
        opcode = WRITE;
    } else if (str == "READ") {
        opcode = READ;
    } else if (str == "WRITE_IMM") {
        opcode = WRITE_IMM;
    } else if (str == "FAA") {
        opcode = FAA;
    } else if (str == "CAS") {
        opcode = CAS;
    } else {
        throw std::runtime_error("Invalid OPCODE: " + str);
    }
    return opcode;
}

sge_t parseSge(const std::string& str)
{
    sge_t sge;
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string token;

    while (std::getline(iss, token, '_')) {
        tokens.push_back(token);
    }

    if (tokens.size() != 3) {
        throw std::runtime_error("sge config error");
    }

    sge.idx = std::stoi(tokens[0]);
    sge.length = std::stoi(tokens[1]);
    sge.addr_bias = std::stoi(tokens[2]);

    return sge;
}

config_t readConfigFromFile(std::ifstream& file)
{
    config_t config;
    std::string line;
    std::getline(file, line);
    std::istringstream iss(line);
    iss >> config.dev_name >> config.ib_port >> config.mr_size >> config.cq_size >> config.qp_recv_size >> config.qp_send_size >> config.mr_num >> config.qp_num;
    return config;
}

std::vector<request_t> readRequestsFromFile(std::ifstream& file)
{
    
    std::vector<request_t> requests;
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        request_t req;
        std::string opcodeStr, localSgesSizeStr;
        if (!(iss >> opcodeStr >> req.local_qp_idx >> req.remote_qp_idx >> localSgesSizeStr)) {
            throw std::runtime_error("Format error");
        }

        req.opcode = stringToOpcode(opcodeStr);
        size_t localSgesCount = std::stoi(localSgesSizeStr);
        req.local_sges.reserve(localSgesCount);

        for (size_t i = 0; i < localSgesCount; ++i) {
            std::string localSgeStr;
            
            if (!(iss >> localSgeStr)) {
                throw std::runtime_error("Number of local_sge error");
            }
            req.local_sges.push_back(parseSge(localSgeStr));
        }

        std::string remoteSgeStr;
        if (!(iss >> remoteSgeStr)) {
            throw std::runtime_error("Lack of remote_sge error");
        }
        req.remote_sge = parseSge(remoteSgeStr);

        requests.push_back(req);
    }

    file.close();
    return requests;
}

std::ostream& operator<<(std::ostream& os, OPCODE opcode)
{
    switch (opcode) {
        case WRITE: os << "WRITE"; break;
        case READ: os << "READ"; break;
        case WRITE_IMM: os << "WRITE_IMM"; break;
        case FAA: os << "FAA"; break;
        case CAS: os << "CAS"; break;
        default: os << "UNKNOWN"; break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const sge_t& sge)
{
    os << sge.idx << "_" << sge.length << "_" << sge.addr_bias;
    return os;
}

void printRequest(const request_t& req)
{
    std::cout << "Opcode: " << req.opcode
              << "\nLocal QP Index: " << req.local_qp_idx
              << "\nRemote QP Index: " << req.remote_qp_idx
              << "\nLocal SGEs: [";
    for (size_t i = 0; i < req.local_sges.size(); ++i) {
        std::cout << req.local_sges[i];
        if (i < req.local_sges.size() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << "]\nRemote SGE: " << req.remote_sge << std::endl;
}
#endif