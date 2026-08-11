// udp_socket.hpp
#pragma once
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cassert>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;

class UdpSocket {
public:
    UdpSocket() : fd_(-1) {}

    // 创建套接字
    bool Socket() {
        fd_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd_ < 0) {
            perror("socket create failed");
            return false;
        }
        return true;
    }

    // 关闭套接字
    bool Close() {
        if (fd_ >= 0) {
            close(fd_);
            fd_ = -1;
        }
        return true;
    }

    // 绑定地址与端口
    bool Bind(const std::string& ip, uint16_t port) {
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = ip.empty() ? INADDR_ANY : inet_addr(ip.c_str());
        addr.sin_port = htons(port);
        int ret = bind(fd_, (sockaddr*)&addr, sizeof(addr));
        if (ret < 0) {
            perror("bind failed");
            return false;
        }
        return true;
    }

    // 接收数据，同时获取发送端IP和端口
    bool RecvFrom(std::string* buf, std::string* ip = NULL, uint16_t* port = NULL) {
        char tmp[1024 * 10] = { 0 };
        sockaddr_in peer;
        socklen_t len = sizeof(peer);
        ssize_t read_size = recvfrom(fd_, tmp, sizeof(tmp) - 1, 0, (sockaddr*)&peer, &len);
        if (read_size < 0) {
            perror("recvfrom failed");
            return false;
        }
        // 赋值输出参数
        buf->assign(tmp, read_size);
        if (ip != NULL) {
            *ip = inet_ntoa(peer.sin_addr);
        }
        if (port != NULL) {
            *port = ntohs(peer.sin_port);
        }
        return true;
    }

    // 发送数据到指定IP和端口
    bool SendTo(const std::string& buf, const std::string& ip, uint16_t port) {
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(ip.c_str());
        addr.sin_port = htons(port);
        ssize_t write_size = sendto(fd_, buf.data(), buf.size(), 0,
            (sockaddr*)&addr, sizeof(addr));
        if (write_size < 0) {
            perror("sendto failed");
            return false;
        }
        return true;
    }

private:
    int fd_;
};

