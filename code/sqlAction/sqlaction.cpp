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
    {"getRecord", sqlAction::getRecordOrder},
    {"getRecordList", sqlAction::getRecordListOrder},
    {"getTodayUserList", sqlAction::getTodayUserListOrder},
    {"getDoctorName", sqlAction::getDoctorNameOrder},
    {"addRecord", sqlAction::addRecordOrder},
    {"updateRecord", sqlAction::updateRecordOrder},
    {"updateDoctorPassword", sqlAction::updateDoctorPasswordOrder}
};

std::string filed_conversion(const std::string str) {
    if (sqlAction::FILED_MAP.count(str) > 0) {
        return sqlAction::FILED_MAP.find(str)->second;
    }
    return str;
}

string getList(string& table, unordered_map<string, string>& queryCond, const char* str) {
    int pageSize = 1;
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

    orderStream << " ORDER BY reception_time DESC LIMIT " << to_string(pageSize) << " OFFSET " << offset;
    return orderStream.str();
}

string sqlAction::getRecordOrder(string& table, unordered_map<string, string>& queryCond) {
    stringstream orderStream;
    orderStream << "SELECT JSON_OBJECT('就诊ID',diagnostic_records.id, '接诊时间', reception_time, "
        "'患者ID', patient_id, '患者姓名', users.name, '手机号', users.username, "
        "'患者自述', patient_statement, '医生主诉', doctor_chief_complaint, '医生姓名', doctors.name, "
        "'面诊', face_diagnose, '舌诊', tongue_diagnose, '脉诊', pulse_diagnose, "
        "'诊断结果', medical_result, '治疗方案', treatment, '备注', remark) FROM diagnostic_records "
        << "JOIN users ON users.id = patient_id "
        "JOIN doctors ON doctors.id = doctor_id "
        "WHERE ";
    return getList(table, queryCond, orderStream.str().c_str());
}

string sqlAction::getRecordListOrder(string& table, unordered_map<string, string>& queryCond) {

    string order = "SELECT JSON_OBJECT('就诊ID',diagnostic_records.id, '接诊时间', reception_time, "
        "'患者ID', patient_id, '患者姓名', users.name, '诊断时间', diagnostic_time) AS json_array_result "
        "FROM diagnostic_records INNER JOIN users ON users.id = patient_id WHERE ";
    return getList(table, queryCond, order.c_str());
}

string sqlAction::getTodayUserListOrder(string& table, unordered_map<string, string>& queryCond) {
    
    string order = "SELECT JSON_OBJECT('就诊ID',diagnostic_records.id, '接诊时间', reception_time, "
        "'患者ID', patient_id, '手机号', users.username, '患者姓名', users.name, '诊断时间', diagnostic_time) "
        "AS json_array_result FROM diagnostic_records INNER JOIN users ON users.id = patient_id WHERE DATE(reception_time)=CURDATE() AND ";
    return getList(table, queryCond, order.c_str());
    
}

string sqlAction::getDoctorNameOrder(string& table, unordered_map<string, string>& queryCond) {
    return "SELECT JSON_OBJECT('医生姓名', name) AS json_array_result FROM doctors WHERE id=" + queryCond.find("doctorIdInRecord")->second; // 待优化
}

string sqlAction::addRecordOrder(string& table, unordered_map<string, string>& queryCond) {
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

string sqlAction::updateRecordOrder(string& table, unordered_map<string, string>& queryCond) {
    string order = "UPDATE diagnostic_records SET ";
    string id = queryCond["receptionId"];
    queryCond.erase("receptionId");
    queryCond.erase("doctorIdInRecord");    // 待优化
    for (const auto& kv : queryCond) {
        order += filed_conversion(kv.first) + "='" + kv.second + "',";
    }
    order += "diagnostic_time=NOW()";
    order += " WHERE id = " + id;
    return order;
}

string sqlAction::updateDoctorPasswordOrder(string& table, unordered_map<string, string>& queryCond) {
    return "UPDATE doctors SET password=" + queryCond.find("doctorPassword")->second + " WHERE id = "
        + queryCond.find("doctorIdInRecord")->second;   // 待优化
}

string sqlAction::login(string table, string username, string password) {
    string order = "SELECT username, password FROM " + table + " WHERE username='%s' LIMIT 1";
    
}

string sqlAction::getDoctorIdByUsername(string username) {
    MYSQL* sql;
    MYSQL_RES *res;
    MYSQL_ROW row;
    SqlConnRAII(&sql,  SqlConnPool::Instance());
    assert(sql);

    string order = "SELECT id FROM doctors WHERE username = " + username + " LIMIT 1";
    LOG_DEBUG("Order: %s", order.c_str())
    if (mysql_query(sql, order.c_str())) {
        LOG_DEBUG("docotr ID: mysql_query() failed: %s\n", mysql_error(sql))
        mysql_free_result(res);
        return "";
    }
    res = mysql_store_result(sql);
    if (res == nullptr) {
        LOG_DEBUG("result set is empty")
        mysql_free_result(res);
        return "";
    }else {
        unsigned long num_rows = mysql_num_rows(res);
        if (num_rows == 0) {
            LOG_DEBUG("result set is empty")
            mysql_free_result(res);
            return "";
        }
    }
    row = mysql_fetch_row(res);
    mysql_free_result(res);
    return row[0];
}

string getOrder(string& table, const string& queryAction, unordered_map<string, string>& queryCond) {
    string order = "";
    // 获取对应指令
    auto getOrderFuncPtr = GET_QUERY_ORDER.find(queryAction);
    if (getOrderFuncPtr != GET_QUERY_ORDER.end()) {
        order = getOrderFuncPtr->second(table, queryCond);
    }
    return order;
}

std::string sqlAction::toJsonArrayString(std::vector<std::string>& jsonList) {
    std::string jsonArray = "[";
    for (size_t i = 0; i < jsonList.size(); i++) {
        if (i > 0) {
            jsonArray += ",";
        }
        jsonArray += jsonList[i];
    }
    jsonArray += "]";
    return jsonArray;
}

bool patientVerify(string userId) {
    string order = "SELECT COUNT(*) FROM users WHERE id = " + userId;
    LOG_DEBUG("%s", order.c_str())
    MYSQL* sql;
    MYSQL_RES *res;
    MYSQL_ROW row;
    SqlConnRAII(&sql,  SqlConnPool::Instance());
    assert(sql);

    if (mysql_query(sql, order.c_str())) {
        LOG_DEBUG("mysql_query() failed %s\n", mysql_error(sql))
        return false;
    }
    res = mysql_use_result(sql);

    if ((row = mysql_fetch_row(res)) != NULL) {
        int count = atoi(row[0]);
        if (count < 1) {
            LOG_DEBUG("用户不存在")
            mysql_free_result(res);
            return false;
        }
    }

    mysql_free_result(res);
    return true;
}

bool sqlAction::insertRecord(string& result, string& table, const string& queryAction,
    unordered_map<string, string>& queryCond){
    string order = getOrder(table, queryAction, queryCond);
    LOG_DEBUG("Order: %s", order.c_str())
    MYSQL* sql;
    SqlConnRAII(&sql,  SqlConnPool::Instance());
    assert(sql);

    if (queryCond.count("patientId") > 0) {
        if (!patientVerify(queryCond["patientId"])) {
            return false;
        }
    }
    else {
        return false;    
    }

    if (mysql_query(sql, order.c_str()) != 0) {
        LOG_DEBUG("Insert failed: %s\n", mysql_error(sql))
        return false;
    }
    return true;
}

bool sqlAction::updateRecord(string& result, string& table, const string& queryAction,
    unordered_map<string, string>& queryCond) {

    string order = getOrder(table, queryAction, queryCond);
    LOG_DEBUG("Order: %s", order.c_str())
    MYSQL* sql;
    SqlConnRAII(&sql,  SqlConnPool::Instance());
    assert(sql);

    if (mysql_query(sql, order.c_str()) != 0) {
        LOG_DEBUG("Update failed: %s\n", mysql_error(sql))
        return false;
    }
    return true;
}

bool sqlAction::getQueryResult(string& result, string& table, const string& queryAction,
    unordered_map<string, string>& queryCond) {
    MYSQL* sql;
    MYSQL_RES *res;
    MYSQL_ROW row;
    SqlConnRAII(&sql,  SqlConnPool::Instance());
    assert(sql);

    string order = getOrder(table, queryAction, queryCond);
    LOG_DEBUG("Order: %s", order.c_str())
    
    if (mysql_query(sql, order.c_str())) {
        LOG_DEBUG("mysql_query() failed: %s\n", mysql_error(sql))
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
    
    if (queryAction.substr(queryAction.length() - 4) == "List") {
        vector<string> jsonList;
        while ((row = mysql_fetch_row(res))) {
            if (row[0] == NULL) {
                mysql_free_result(res);
                return false;
            }
            string jsonStr = row[0];
            jsonList.push_back(jsonStr);
        }

        result = toJsonArrayString(jsonList);
    }
    else {
        row = mysql_fetch_row(res);
        result = row[0];
    }
    mysql_free_result(res);

    return true;
}