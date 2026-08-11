// 客户端我们就不封装了,也不使用日志了
#include <cstdlib>
#include <cstring>         // 提供 memset 等内存操作函数
#include <iostream>
#include <string>
// --- 网络通信核心头文件 ---
#include <sys/socket.h>    // 提供 socket、sendto、recvfrom 等系统调用
#include <netinet/in.h>    // 提供 sockaddr_in 结构体及网络宏定义
#include <arpa/inet.h>     // 提供网络字节序与IP格式转换函数 (htons, inet_addr)
#include <sys/types.h>

// 辅助函数：当用户命令行参数输入不对时，提示正确的启动格式
void Usage(const std::string& name)
{
    std::cerr << "Usage: " << name << " server_ip server_port" << std::endl;
}

// ./UdpEchoClient 1900.0.0.1 8080 (注：1900 是无效 IP 段，本地测试通常用 127.0.0.1)
int main(int argc, char* argv[])
{
    // 客户端需要3个参数：程序名本身、目标服务器的IP、目标服务器的端口
    if (argc != 3)
    {
        Usage(argv[0]);
        exit(1);
    }

    // 得到我们的服务端IP和Port
    std::string server_ip = argv[1];
    // std::stoi: 将传入的字符串形式的端口号转换为 16位无符号整数
    uint16_t server_port = std::stoi(argv[2]);

    // 1. 创建 sockefd (获取网卡/网络协议栈的访问凭证)
    // AF_INET: IPv4协议族; SOCK_DGRAM: 无连接的数据报服务(UDP)
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        std::cerr << "create client socketfd error" << std::endl;
        exit(1);
    }

    // 2. 构建目标服务器socket信息 (提前写好信封上的“收件人地址”)
    // 自己一定需要自己的IP和端口号。
    // 但是，client不能自己显示的bind port，一般客户端都是由OS自己选择IP和Port，
    // 尤其是Port，client的port要让OS随机选择
    // 客户端port，是多少不重要，唯一才重要
    // 服务器port，是多少很重要，唯一是基础
    // client不能自己显示的bind port, 但是必须bind，由OS自己完成，Port随机
    struct sockaddr_in server;
    socklen_t len = sizeof(server);
    // 我们服务端用了bzero,这里就用用memset (严谨：清零内存，防止残留脏数据干扰内核)
    memset(&server, 0, len);
    server.sin_family = AF_INET;
    server.sin_port = htons(server_port); // 主机字节序转网络大端字节序
    server.sin_addr.s_addr = inet_addr(server_ip.c_str()); // 点分十进制字符串转网络4字节整数IP

    // 3. 发送数据和读取数据
    std::string inbuffer;
    while (true)
    {
        std::cout << "Please Enter# ";
        // 获取用户输入 (注意：cin 遇到空格会截断，如果是发带有空格的英文句子，通常改用 getline)
        std::cin >> inbuffer;

        // 1. 发送数据
        // 【核心细节】：对于客户端，正是在这里【第一次】调用 sendto 发送数据时，
        // 操作系统底层会察觉到这个 sockfd 还没有绑定端口，从而自动为它分配一个随机的空闲端口进行隐式 bind！
        ssize_t n = sendto(sockfd, inbuffer.c_str(), inbuffer.size(), 0, (struct sockaddr*)&server, len);
        // 压制编译器针对变量 n 未使用的警告
        (void)n;

        // 2. 接收数据
        // temp 用于接收给你回信的那个节点 (即服务器) 的网络地址信息
        struct sockaddr_in temp;
        socklen_t tempLen = sizeof(temp);
        char buffer[1024];

        // 阻塞等待服务器的处理结果 (例如翻译后的中文)
        // 注意 sizeof(buffer) - 1 是为了给最后的手动 '\0' 预留出安全的空间
        ssize_t m = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&temp, &tempLen);
        if (m > 0)
            // 安全处理：网络发来的全是纯字节，我们必须手动加上 C 风格字符串的结尾标识，防止打印时越界乱码
            buffer[m] = 0;

        // 打印最终结果
        std::cout << buffer << std::endl;
    }
}
