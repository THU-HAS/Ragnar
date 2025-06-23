#ifndef PT_SYNC_HPP
#define PT_SYNC_HPP

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define MAGIC_WORD "SYNC"

int sync_server(int port, char* data_send, char* data_recv, int length) {
    int server_fd, sock;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 1) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    if ((sock = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    read(sock, buffer, sizeof(buffer));
    if(strcmp(buffer, MAGIC_WORD) == 0)
    {
        memset(buffer, 0, sizeof(buffer));
        std::cout << "Synchronization successful." << std::endl;
        send(sock, MAGIC_WORD, strlen(MAGIC_WORD), 0);
    }
    else
    {
        std::cout << "Synchronization failed." << std::endl;
        exit(EXIT_FAILURE);
    }
    read(sock, buffer, sizeof(buffer));
    send(sock, data_send, length, 0);
    memcpy(data_recv, buffer, length);

    close(sock);
    close(server_fd);

    return 0;
}

int sync_client(const char* addr, int port, char*data_send, char* data_recv, int length)
{
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "\n Socket creation error \n";
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if(inet_pton(AF_INET, addr, &serv_addr.sin_addr) <= 0) {
        std::cout << "\nInvalid address/ Address not supported \n";
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "\nConnection Failed \n";
        return -1;
    }

    send(sock, MAGIC_WORD, strlen(MAGIC_WORD), 0);
    read(sock, buffer, sizeof(buffer));
    if(strcmp(buffer, MAGIC_WORD) == 0)
    {
        std::cout << "Synchronization successful." << std::endl;
        memset(buffer, 0, sizeof(buffer));
    }
    else
    {
        std::cout << "Synchronization failed." << std::endl;
        exit(EXIT_FAILURE);
    }
    send(sock, data_send, length, 0);
    read(sock, buffer, sizeof(buffer));

    memcpy(data_recv, buffer, length);
    close(sock);

    return 0;
}
#endif