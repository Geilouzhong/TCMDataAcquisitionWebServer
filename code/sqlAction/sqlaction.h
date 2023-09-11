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
    std::string getRecordOrder(std::string& table, std::unordered_map<std::string, std::string>& queryCond);

    std::string getRecordListOrder(std::string& table, std::unordered_map<std::string, std::string>& queryCond);

    std::string getTodayUserListOrder(std::string& table, std::unordered_map<std::string, std::string>& queryCond);

    std::string getDoctorNameOrder(std::string& table, std::unordered_map<std::string, std::string>& queryCond);

    std::string getQueryNumOrder(std::string& table, std::unordered_map<std::string, std::string>& queryCond);

    std::string getTodayQueryNumOrder(std::string& table, std::unordered_map<std::string, std::string>& queryCond);

    std::string addRecordOrder(std::string& table, std::unordered_map<std::string, std::string>& queryCond);

    std::string updateRecordOrder(std::string& table, std::unordered_map<std::string, std::string>& queryCond);

    std::string updateDoctorPasswordOrder(std::string& table, std::unordered_map<std::string, std::string>& queryCond);

    std::string login(std::string table, std::string username, std::string password);

    std::string getDoctorIdByUsername(std::string username);

    std::string toJsonArrayString(std::vector<std::string>& jsonList); 

    bool getQueryResult(std::string& result, std::string& table, const std::string& queryAction,
         std::unordered_map<std::string, std::string>& queryCond);

    bool insertRecord(std::string& result, std::string& table, const std::string& queryAction,
         std::unordered_map<std::string, std::string>& queryCond);

    bool updateRecord(std::string& result, std::string& table, const std::string& queryAction,
         std::unordered_map<std::string, std::string>& queryCond);

    extern const std::unordered_map<std::string, std::string> FILED_MAP;
}

#endif