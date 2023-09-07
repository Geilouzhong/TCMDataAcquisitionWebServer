#include "sqlaction.h"

using namespace std;

const std::unordered_map<std::string, std::string> sqlAction::FILED_MAP = {
    {"userId", "users.id"}, 
    {"receptionId", "diagnostic_records.id"},
    {"username", "users.name"},
    {"receptionTime", "diagnostic_records.reception_time"},
    {"patientId", "diagnostic_records.patient_id"},
    {"doctorId", "doctors.id"},
    {"doctorIdInRecord", "doctor_id"},
    {"patientStatement", "diagnostic_records.patient_statement"},
    {"doctorChiefComplaint", "diagnostic_records.doctor_chief_complaint"},
    {"faceDiagnose", "diagnostic_records.face_diagnose"},
    {"tongueDiagnose", "diagnostic_records.tongue_diagnose"},
    {"pulseDiagnose", "diagnostic_records.pulse_diagnose"},
    {"medicalResult", "diagnostic_records.medical_result"},
    {"treatment", "diagnostic_records.treatment"},
    {"remark", "diagnostic_records.remark"}
};

const unordered_map<string, function<string(string&, unordered_map<string, string>&)>> GET_QUERY_ORDER = {
    {"getRecord", sqlAction::getRecord},
    {"getRecordList", sqlAction::getRecordList},
    {"getTodayUserList", sqlAction::getTodayUserList},
    {"addRecord", sqlAction::addRecord},
    {"updateRecord", sqlAction::updateRecord}
};

std::string filed_conversion(const std::string str) {
    if (sqlAction::FILED_MAP.count(str) > 0) {
        return sqlAction::FILED_MAP.find(str)->second;
    }
    return str;
}

string getList(string& table, unordered_map<string, string>& queryCond, const char* str) {
    int pageSize = 0;
    if (queryCond.count("pageSize") > 0) {
        pageSize = stoi(queryCond["pageSize"]);
        queryCond.erase("pageSize");
    }
    
    int pageIndex = 0;
    if (queryCond.count("pageIndex") > 0) {
        pageIndex = stoi(queryCond["pageIndex"]);
        queryCond.erase("pageIndex");
    }

    string offset = to_string(pageSize * pageIndex);

    stringstream orderStream;
    orderStream << str;
    for (auto it = queryCond.begin(); it != queryCond.end(); ++it) {
        orderStream << filed_conversion(it->first) << "=" << it->second;
        if (next(it) != queryCond.end()) {
            orderStream << " and ";
        }
    }

    orderStream << " ORDER BY reception_time LIMIT " << to_string(pageSize) << " OFFSET " << offset;
    return orderStream.str();
}

string sqlAction::getRecord(string& table, unordered_map<string, string>& queryCond) {
    stringstream orderStream;
    orderStream << "SELECT * FROM diagnostic_records WHERE id=" << queryCond.find("receptionId")->second << " LIMIT 1";
    return orderStream.str();
}

string sqlAction::getRecordList(string& table, unordered_map<string, string>& queryCond) {

    string order = "SELECT JSON_ARRAYAGG(JSON_OBJECT('receptionId',diagnostic_records.id, 'receptionTime', reception_time, "
        "'patientId', patient_id, 'patientName', users.name, 'diagnosticTime', diagnostic_time)) AS json_array_result "
        "FROM diagnostic_records INNER JOIN users ON users.id = patient_id ";
    if (queryCond.size() > 3) {
        order += "WHERE ";
    }
    return getList(table, queryCond, order.c_str());
}

string sqlAction::getTodayUserList(string& table, unordered_map<string, string>& queryCond) {
    
    string order = "SELECT JSON_ARRAYAGG(JSON_OBJECT('receptionId',diagnostic_records.id, 'receptionTime', reception_time, "
        "'patientId', patient_id, 'userName', users.username, 'patientName', users.name, 'diagnosticTime', diagnostic_time)) "
        "AS json_array_result FROM diagnostic_records INNER JOIN users ON users.id = patient_id WHERE DATE(reception_time)=CURDATE() AND ";
    if (queryCond.size() > 3) {
        order += "AND ";
    }
    return getList(table, queryCond, order.c_str());
    
}

string sqlAction::addRecord(string& table, unordered_map<string, string>& queryCond) {
    string order("INSERT INTO diagnostic_records (");
    string values("VALUES (");
    for (const auto& kv : queryCond) {
        order += filed_conversion(kv.first) + ",";
        values += "'" + kv.second + "',";
    }
    order.pop_back();
    order += ") ";
    values.pop_back();
    values += ");";
    order += values;

    return order;
}

string sqlAction::updateRecord(string& table, unordered_map<string, string>& queryCond) {
    string order = "UPDATE diagnostic_records SET ";
    string id = queryCond["receptionId"];
    for (const auto& kv : queryCond) {
        order += filed_conversion(kv.first) + "='" + kv.second + "',";
    }
    order.pop_back();
    order += " WHERE id = " + id;
    return order;
}

string sqlAction::login(string table, string username, string password) {
    string order = "SELECT username, password FROM " + table + " WHERE username='%s' LIMIT 1";
    
}

bool sqlAction::getQueryResult(string& result, string& table, const string& queryAction, unordered_map<string, string>& queryCond) {
    MYSQL* sql;
    MYSQL_RES *res;
    MYSQL_ROW row;
    SqlConnRAII(&sql,  SqlConnPool::Instance());
    assert(sql);

    string order = "";
    // 获取对应指令
    auto getOrderFuncPtr = GET_QUERY_ORDER.find(queryAction);
    if (getOrderFuncPtr != GET_QUERY_ORDER.end()) {
        order = getOrderFuncPtr->second(table, queryCond);
    }
    LOG_DEBUG("Order: %s", order.c_str())
    
    if (mysql_query(sql, order.c_str())) {
        LOG_DEBUG("mysql_query() failed")
        mysql_free_result(res);
        return false;
    }

    res = mysql_store_result(sql);
    if (res == nullptr) {
        LOG_DEBUG("result set is empty")
        mysql_free_result(res);
        return false;
    }
    else {
        unsigned long num_rows = mysql_num_rows(res);
        if (num_rows == 0) {
            LOG_DEBUG("result set is empty")
            mysql_free_result(res);
            return false;
        }
    }
    row = mysql_fetch_row(res);
    result = row[0];

    mysql_free_result(res);

    return true;
}