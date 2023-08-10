#ifndef CONFIG_H
#define CONFIG_H

#include <unistd.h>
#include <stdlib.h>
#include <string>

class Config
{
public:
    Config();
    ~Config(){};

    void parseArg(int argc, char*argv[]);
    void print();

    //端口号
    int port;

    //触发组合模式
    int trigMode;

    //超时时间
    int timeoutMs;

    //优雅关闭链接
    bool OptLinger;

    //数据库登录名、密码、数据库名
    char* sqlUser;
    char* sqlPwd;
    char* dbName;

    //数据库连接池数量
    int connPoolNum;

    //线程池内的线程数量
    int threadNum;

    //是否关闭日志
    bool openLog;

    //日志等级
    int logLevel;

    //消息队列长度
    int logQueSize;
};

#endif