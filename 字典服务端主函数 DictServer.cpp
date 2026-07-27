// 引入智能指针头文件，用于自动管理对象内存
#include <memory>
#include "Dictionary.hpp"
#include "UdpServer.hpp"

// 辅助函数：提示用户如何正确使用命令行参数启动程序
void Usage(const std::string& name)
{
    // 提示：虽然这里的打印文案依然写着 "ip port"，但实际上根据下方逻辑，目前只需要传端口号
    std::cerr << "Usage: " << name << " ip port" << std::endl;
}

// ./UdpEchoServer 8080
// 我们不直接绑定固定IP
int main(int argc, char* argv[])
{
    // 参数校验：程序启动名算第1个参数，端口号算第2个，所以 argc 必须等于 2
    if (argc != 2)
    {
        Usage(argv[0]);
        exit(0);
    }

    // 初始化日志系统：启用控制台日志输出策略，这样程序运行时的 LOG 信息才会打印在屏幕上
    ENABLE_CONSOLE_LOG_STRATEGY();

    // std::string server_ip = argv[1];
    // 获取传入的端口参数，并将字符串 (argv[1]) 转换为 16 位无符号整型
    uint16_t server_port = std::stoi(argv[1]);

    // 1. 创建一个在线字典服务
    // 【业务层】：实例化字典对象。使用 std::unique_ptr 智能指针管理，确保程序结束时资源自动释放
    std::unique_ptr<Dictionary> dict = std::make_unique<Dictionary>();

    // 2. 创建一个网络服务器
    // 【网络层与桥接】：实例化 UdpServer。
    // 核心亮点：通过 Lambda 表达式 (匿名函数) 实现了网络与业务的完美解耦。
    // [&dict]：以引用方式捕获外部的字典对象指针。
    // (const std::string &word)->std::string：定义了输入一个字符串，返回一个字符串的处理逻辑。
    // 运行机制：当底层的 UdpServer 收到网络数据时，它会拿着收到的字符串来调用这段 Lambda 代码，
    // 从而触发 dict->TransTrate(word) 进行翻译，再由 UdpServer 将翻译结果发回给客户端。
    std::unique_ptr<UdpServer> usvr = std::make_unique<UdpServer>([&dict](const std::string& word)->std::string {
        return dict->TransTrate(word);
        }, server_port);

    // 3. 初始化和启动服务器
    // 执行底层 socket 创建和 INADDR_ANY 的 bind 绑定操作
    usvr->Init();
    // 启动服务器死循环，开始不间断地接待客户端的网络请求
    usvr->Start();
}
