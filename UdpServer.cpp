#ifndef __UDP__SERVER__HPP
#define __UDP__SERVER__HPP

// --- 标准系统与网络编程头文件 ---
#include <cstdint>
#include <string>
#include <strings.h>       // 提供 bzero
#include <sys/socket.h>    // 提供 socket、bind、recvfrom、sendto
#include <netinet/in.h>    // 提供 sockaddr_in 结构体及 INADDR_ANY
#include <arpa/inet.h>     // 提供 htons、inet_ntoa 等转换函数
#include <sys/types.h>
#include <functional>      // 提供 std::function，用于支持回调函数机制 (核心解耦利器)
#include "logger.hpp"

using namespace LogModule;

// 参数就是获得的数据，返回值就是处理完数据的结果
// 【核心解耦设计】：定义回调函数类型 callback_t。
// 服务器只负责收发字符串，具体字符串怎么处理（例如：翻译单词、计算算术题），由外部传入的这个函数决定。
using callback_t = std::function<std::string(const std::string&)>;

class UdpServer {
public:
    // 构造函数：现在多了一个参数 cb，用于接收上层业务传递进来的具体处理逻辑
    UdpServer(callback_t cb, uint16_t port)
        : _socketfd(-1)
        , _port(port)
        , _cb(cb) // 保存业务层传入的回调函数
    {
    }

    void Init()
    {
        // 1. 创建socket, 系统概念
        // AF_INET: IPv4协议族; SOCK_DGRAM: 无连接的数据报服务(UDP); 0: 默认协议
        _socketfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (_socketfd < 0)
        {
            LOG(LogLevel::FATAL) << "create socketfd error";
        }
        LOG(LogLevel::INFO) << "create socketfd success: " << _socketfd;

        // 2. bind (绑定网络信息到 socket)
        struct sockaddr_in local;
        socklen_t len = sizeof(local);
        bzero(&local, len); // 严谨操作：清空结构体内存，防止脏数据
        local.sin_family = AF_INET;
        local.sin_port = htons(_port); // 这里需要本机转网络 (保证端口大端传输)
        // 如果server 显示的bind了一个具体IP地址，那么它一般就只能收到发给这个IP地址的报文
        // local.sin_addr.s_addr = inet_addr(_ip.c_str()); // 点分十进制 -> 4字节IP,网络序列的
        // 绑定 INADDR_ANY (0)，监听本机所有网卡的请求，非常适合云服务器等多网卡环境
        local.sin_addr.s_addr = INADDR_ANY; // 任意IP地址

        // 执行系统调用 bind，强转为统一的 struct sockaddr* 指针
        int n = bind(_socketfd, (struct sockaddr*)&local, len);
        if (n < 0)
        {
            LOG(LogLevel::FATAL) << "bind socketfd error";
        }
        LOG(LogLevel::INFO) << "bind socketfd success: " << _socketfd;
    }

    void Start()
    {
        char inbuffer[1024];
        while (true)
        {
            // perr 保存客户端的网络地址信息 (发件人是谁)
            struct sockaddr_in perr;
            socklen_t len = sizeof(perr);
            // 1. 读取网络数据
            // 阻塞等待客户端发来数据，sizeof(inbuffer)-1 是为了给结尾留一个 '\0' 的位置
            ssize_t n = recvfrom(_socketfd, inbuffer, sizeof(inbuffer) - 1, 0, (struct sockaddr*)&perr, &len);
            if (n < 0)
            {
                LOG(LogLevel::WARNING) << "recvfrom error";
                break;
            }
            // 手动添加字符串结束符，将接收到的网络纯字节流转为安全的 C 风格字符串
            inbuffer[n] = 0;

            // 我们从peer里面拿到的肯定是网络序列,我们这里打印观察需要的是主机序列
            // inet_ntoa: 将 4 字节网络 IP 转成直观的点分十进制字符串
            std::string clientIp = inet_ntoa(perr.sin_addr);
            // ntohs: 将大端网络端口转回小端主机端口
            uint16_t clientPort = ntohs(perr.sin_port);
            LOG(LogLevel::INFO) << "get a message: " << inbuffer
                << ", client addr: " << clientIp << ":" << clientPort;

            // 处理数据
            // 【架构升级的精髓所在】：以前这里是写死的 "server say: " 字符串拼接
            // 现在网络层完全不用知道业务逻辑，直接呼叫上层传进来的 _cb 回调函数
            std::string result;
            if (_cb) // 安全检查：确保外部真的传了一个有效的函数进来
            {
                // 将网络接收到的请求 (inbuffer) 扔给业务层，获取处理结果 (result)
                result = _cb(inbuffer);
            }

            // 2. 发送网络数据
            // 这个len是个输入输出型参数
            // 把业务层返回的 result，通过 perr 记录的原路发回给客户端
            ssize_t m = sendto(_socketfd, result.c_str(), result.size(), 0, (struct sockaddr*)&perr, len);
            (void)m; // 压制编译器警告
        }
    }

    // 析构函数：释放系统资源
    ~UdpServer()
    {
        if (_socketfd >= 0)
        {
            close(_socketfd); // 关闭套接字
            LOG(LogLevel::INFO) << "socket closed, sockfd: " << _socketfd;
        }
    }
private:
    int _socketfd;       // 服务器 socket 文件描述符
    // std::string _ip;  // 可以不需要 (使用了 INADDR_ANY)
    uint16_t _port;      // 监听的端口

    // 保存外部传入的回调函数，作为网络层和业务层沟通的桥梁
    callback_t _cb;
};
#endif
