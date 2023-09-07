#ifndef SQL_ACTION_H
#define SQL_ACTION_H

#include <mysql/mysql.h>
#include <string>
#include <string.h>
#include <sstream>
#include <unordered_map>
#include <functional>

#include "../pool/sqlconnRAII.h"
#include "../log/log.h"

namespace sqlAction{
    std::string getRecord(std::string& table, std::unordered_map<std::string, std::string>& queryCond);

    std::string getRecordList(std::string& table, std::unordered_map<std::string, std::string>& queryCond);

    std::string getTodayUserList(std::string& table, std::unordered_map<std::string, std::string>& queryCond);

    std::string addRecord(std::string& table, std::unordered_map<std::string, std::string>& queryCond);

    std::string updateRecord(std::string& table, std::unordered_map<std::string, std::string>& queryCond);

    std::string login(std::string table, std::string username, std::string password);

    bool getQueryResult(std::string& result, std::string& table, const std::string& queryAction, std::unordered_map<std::string, std::string>& queryCond);

    extern const std::unordered_map<std::string, std::string> FILED_MAP;
}

#endif