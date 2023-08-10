#include "config/config.h"
#include "server/webserver.h"

int main(int argc, char* argv[]) {
    /* 守护进程 后台运行 */
    //daemon(1, 0); 

    Config config;
    config.parseArg(argc, argv);

    WebServer server(
        config.port, config.trigMode, config.timeoutMs, config.OptLinger,   /* 端口 ET模式 timeoutMs 优雅退出  */
        3306, config.sqlUser, config.sqlPwd, config.dbName,             /* Mysql配置 */
        config.connPoolNum, config.threadNum, config.openLog,
        config.logLevel, config.logQueSize);             /* 连接池数量 线程池数量 日志开关 日志等级 日志异步队列容量 */
    server.Start();
} 
  