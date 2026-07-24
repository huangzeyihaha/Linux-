#ifndef __DICTIONARY__HPP
#define __DICTIONARY__HPP

// --- 标准库与第三方头文件 ---
#include <fstream>         // 提供文件输入输出流 (std::ifstream)，用于读取字典文件
#include <iostream>
#include <string>
#include <unordered_map>   // 提供哈希表数据结构，用于实现内存中的极速键值对查找 (O(1) 复杂度)
#include "logger.hpp"      // 引入自定义的日志模块
using namespace LogModule;

// --- 全局配置常量 ---
const std::string gdefaultfilename = "./Dict.txt"; // 默认加载的字典配置文件路径
const std::string gsep = ": ";                     // 字典文件中 Key 和 Value 之间的分隔符 (冒号加空格)

class Dictionary
{
private:
    // 私有方法：负责在对象初始化时，将磁盘文件中的字典数据加载到内存中
    void LoadConfig()
    {
        // 尝试以只读模式打开指定的字典文件
        std::ifstream in(_dictfilename);
        if (!in.is_open())
        {
            // 如果文件不存在或权限不足，对于字典服务来说是致命错误，直接退出进程
            LOG(LogLevel::FATAL) << "open fail";
            exit(1);
        }
        LOG(LogLevel::INFO) << "open success";

        std::string line;
        // 按行读取文件内容，只要没读到文件末尾 (EOF)，就一直循环读取
        while (std::getline(in, line))
        {
            // apple: 苹果 - I eat an apple every day. / 我每天吃一个苹果。

            // 核心切割逻辑：寻找分隔符 ": " 的位置
            auto pos = line.find(gsep);
            if (pos == std::string::npos) // 没有找到
            {
                // 容错处理：如果这一行格式不对（缺少分隔符），只打警告日志，跳过它继续加载下一行
                LOG(LogLevel::WARNING) << "load fail";
                continue;
            }

            // 提取 Key (英文单词)：
            // pos 的值恰好等于前半部分字符串的长度，所以 substr(0, pos) 完美截取了单词
            std::string key = line.substr(0, pos);

            // 提取 Value (中文翻译及例句)：
            // 从 (分隔符的起始下标 + 分隔符本身的长度) 开始截取，一直截取到这行的末尾
            std::string value = line.substr(pos + gsep.size());

            // 将切割好的键值对存入内存中的哈希表
            _dictmp.insert({ key, value });
        }
        // 释放文件句柄资源
        in.close();
    }

public:
    // 构造函数：默认使用全局的配置路径
    // 巧妙的设计：对象一被创建，就立刻自动调用 LoadConfig() 完成文件的加载和解析
    Dictionary(const std::string dictfilename = gdefaultfilename)
        : _dictfilename(dictfilename)
    {
        LoadConfig();
    }

    // 公共接口：提供在线翻译服务 (将收到的网络单词转换为对应的中文)
    std::string TransTrate(const std::string& word)
    {
        // 在哈希表中进行极速查找
        auto it = _dictmp.find(word);
        if (it == _dictmp.end())
        {
            // 如果查到了哈希表的末尾还没找到，说明字典里没有这个词
            return "未知";
        }
        // 找到了，返回对应的翻译内容 (迭代器的 second 就是 Value)
        return it->second;
    }

    // 析构函数：由于使用了 STL 容器 (string, unordered_map)，它们会自动管理内存，所以这里为空即可
    ~Dictionary()
    {

    }

private:
    std::string _dictfilename; // 存放当前对象使用的字典文件路径
    std::unordered_map<std::string, std::string> _dictmp; // 核心数据结构：承载字典内容的内存哈希表
};
#endif
