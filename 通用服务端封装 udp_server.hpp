#pragma once
// udp_server.hpp
#pragma once
#include "udp_socket.hpp"
#include <functional>

// 业务处理回调函数类型
typedef std::function<void(const std::string&, std::string* resp)> Handler;

class UdpServer {
public:
    UdpServer() {
        assert(sock_.Socket());
    }

    ~UdpServer() {
        sock_.Close();
    }

    // 启动服务
    bool Start(const std::string& ip, uint16_t port, Handler handler) {
        // 绑定端口
        if (!sock_.Bind(ip, port)) {
            return false;
        }
        printf("UdpServer start success, listen on %s:%d\n", ip.c_str(), port);
        // 事件循环
        for (;;) {
            // 接收请求
            std::string req;
            std::string remote_ip;
            uint16_t remote_port = 0;
            if (!sock_.RecvFrom(&req, &remote_ip, &remote_port)) {
                continue;
            }
            // 业务处理
            std::string resp;
            handler(req, &resp);
            // 返回响应
            sock_.SendTo(resp, remote_ip, remote_port);
            printf("[%s:%d] req: %s, resp: %s\n", remote_ip.c_str(), remote_port,
                req.c_str(), resp.c_str());
        }
        sock_.Close();
        return true;
    }

private:
    UdpSocket sock_;
};
