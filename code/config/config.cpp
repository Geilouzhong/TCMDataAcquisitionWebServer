#include "config.h"

Config::Config(){
    //端口号,默认9008
    port = 9008;

    //触发组合模式,默认listenfd LT + connfd LT
    trigMode = 0;

    //超时时间
    timeoutMs = 60000;

    //优雅关闭链接，默认不使用
    OptLinger = false;

    //数据库登录名、密码、数据库名
    sqlUser = (char*)"root";
    sqlPwd = (char*)"051741";
    dbName = (char*)"webserverdb";

    //数据库连接池数量,默认8
    connPoolNum = 8;

    //线程池内的线程数量,默认8
    threadNum = 8;

    //日志开关,默认不关闭
    openLog = true;

    //消息队列长度，0为同步日志
    logQueSize = 1024;
}

void Config::parseArg(int argc, char*argv[]){
    int opt;
    const char *str = "p:m:T:o:u:w:d:s:t:l:L:q:";
    while ((opt = getopt(argc, argv, str)) != -1)
    {
        switch (opt)
        {
        case 'p':
        {
            port = atoi(optarg);
            break;
        }
        case 'm':
        {
            trigMode = atoi(optarg);
            break;
        }
        case 'T':
        {
            timeoutMs = atoi(optarg);
            break;
        }
        case 'o':
        {
            OptLinger = atoi(optarg);
            break;
        }
        case 'u':
        {
            sqlUser = optarg;
            break;
        }
        case 'w':
        {
            sqlPwd = optarg;
            break;
        }
        case 'd':
        {
            dbName = optarg;
            break;
        }
        case 's':
        {
            connPoolNum = atoi(optarg);
            break;
        }
        case 't':
        {
            threadNum = atoi(optarg);
            break;
        }
        case 'l':
        {
            openLog = atoi(optarg);
            break;
        }
        case 'L':
        {
            logLevel = atoi(optarg);
            break;
        }
        case 'q':
        {
            logQueSize = atoi(optarg);
            break;
        }
        default:
            break;
        }
    }
}

void Config::print() {
    printf("port: %d\n", port);
    printf("trigMode: %d\n", trigMode);
    printf("timeoutMs: %d\n", timeoutMs);
    printf("Linger: %d\n", OptLinger);
    printf("sqlUser: %s\n", sqlUser);
    printf("sqlPwd: %s\n", sqlPwd);
    printf("dbName: %s\n", dbName);
    printf("connPoolNum: %d\n", connPoolNum);
    printf("threadNum: %d\n", threadNum);
    printf("openLog: %d\n", openLog);
    printf("logLevel: %d\n", logLevel);
    printf("logQueSize: %d\n", logQueSize);
}