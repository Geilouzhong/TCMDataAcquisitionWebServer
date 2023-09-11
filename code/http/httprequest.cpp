#include "httprequest.h"
using namespace std;

const string HttpRequest::s_staticPrefix("/static/");
const unordered_set<string> HttpRequest::DEFAULT_HTML{
            "/static/home", "/static/register", "/static/login",
             "/static/error", "/static/video", "/static/picture", };

const unordered_map<string, int> HttpRequest::DEFAULT_HTML_TAG {
            {"/register.html", 0}, {"/login", 1},  };

std::string HttpRequest::s_URLDecode(const std::string& url) {
    std::string result;
    char ch;
    int i, j;
    for (i = 0; i < url.length(); i++) {
        if (int(url[i]) == 37) { // ASCII code for '%'
            sscanf(url.substr(i + 1, 2).c_str(), "%x", &j);
            ch = static_cast<char>(j);
            result += ch;
            i += 2;
        }
        else {
            result += url[i];
        }
    }
    return result;
}

void HttpRequest::Init() {
    method_ = path_ = version_ = body_ = queryTable_ = action_ = sessionId_ = "";
    state_ = REQUEST_LINE;
    isAccessStatic_ = true;
    isDelSession_ = false;
    header_.clear();
    post_.clear();
    queryCond_.clear();
    cookies_.clear();
}

bool HttpRequest::IsKeepAlive() const {
    if(header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

bool HttpRequest::IsAccessStatic() const {
    return isAccessStatic_;
}

string HttpRequest::GetSessionId() const {
    return sessionId_;
}

bool HttpRequest::parse(Buffer& buff) {
    const char CRLF[] = "\r\n";
    if(buff.ReadableBytes() <= 0) {
        return false;
    }
    while(buff.ReadableBytes() && state_ != FINISH) {
        const char* lineEnd = search(buff.Peek(), buff.BeginWriteConst(), CRLF, CRLF + 2);
        std::string line(buff.Peek(), lineEnd);
        switch(state_)
        {
        case REQUEST_LINE:
            if(!ParseRequestLine_(line)) {
                return false;
            }
            ParsePath_();
            break;    
        case HEADERS:
            ParseHeader_(line);
            if(buff.ReadableBytes() <= 2) {
                state_ = FINISH;
            }
            break;
        case BODY:
            ParseBody_(line);
            break;
        default:
            break;
        }
        if(lineEnd == buff.BeginWrite()) { break; }
        buff.RetrieveUntil(lineEnd + 2);
    }
    // 退出登录清除session
    if (isDelSession_ && isAccessStatic_) {
        SessionPool::Instance()->delSession(cookies_.find("sessionId")->second);
        isDelSession_ = false;
    }
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    // for (auto it = header_.begin(); it != header_.end(); ++it) {
    //     LOG_DEBUG("%s: %s", it->first.c_str(), it->second.c_str())
    // }
    return true;
}

void HttpRequest::ParsePath_() {
    path_ = s_URLDecode(path_);
    if(path_ == "/") {
        path_ = "/static/login.html"; 
        return ;
    }
    /* 静态资源 */
    if (path_.substr(0, s_staticPrefix.length()) == s_staticPrefix || path_ == "/login") {
        for(auto &item: DEFAULT_HTML) {
            if(item == path_) {
                path_ += ".html";
                break;
            }
        }
        isAccessStatic_ = true;
    }
    else if (path_.substr(path_.length() - 7) == "/logout") {
        isDelSession_ = true;
        path_ = "/static/login.html";
    }
    /* 数据库访问 */
    else {
        ParseUrlQuery_();
    }
}

void HttpRequest::ParseUrlQuery_() {
    /* 获取访问的表 */
    size_t found = path_.find_last_of('/');
    if (found != std::string::npos && found > 0) {
        queryTable_ = path_.substr(1, found - 1);
        isAccessStatic_ = false;
    }

    /* 获取行为 */
    regex pattern("/([^?]+)");
    smatch match;
    if (regex_search(path_, match, pattern)) {
        string tmp = match[1];
        found = tmp.find_last_of('/');
        if (found != std::string::npos && found > 0) {
            action_ = tmp.substr(found + 1);
        }
    } else {

    }

    /* 获取过滤内容 */
    pattern = ("([\\w-]+)=([\\w-]+)");
    sregex_iterator next(path_.begin(), path_.end(), pattern);
    sregex_iterator end;
    while (next != end) {
        smatch match = *next;
        queryCond_[match.str(1)] = match.str(2);
        ++next;
    }
    LOG_DEBUG("Table: %s", queryTable_.c_str())
    LOG_DEBUG("Action: %s", action_.c_str())
    // for (auto it = queryCond_.begin(); it != queryCond_.end(); it++) {
    //     LOG_DEBUG("%s: %s", it->first.c_str(), it->second.c_str())
    // }
}

bool HttpRequest::ParseRequestLine_(const string& line) {
    regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    smatch subMatch;
    if(regex_match(line, subMatch, patten)) {   
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        state_ = HEADERS;
        return true;
    }
    LOG_ERROR("RequestLine Error: %s", line.c_str());
    return false;
}

void HttpRequest::ParseHeader_(const string& line) {
    regex patten("^([^:]*): ?(.*)$");
    smatch subMatch;
    if(regex_match(line, subMatch, patten)) {
        header_[subMatch[1]] = subMatch[2];
        if (subMatch[1] == "Cookie") {
            // LOG_DEBUG("Cookie: %s", header_["Cookie"].c_str())
            ParseCookie_();
        }
    }
    else {
        state_ = BODY;
    }
}

void HttpRequest::ParseCookie_() {
    if (header_.find("Cookie") == header_.end()) return;
    string cookieString = header_["Cookie"];
    
    // 匹配 "name=value" 形式的 Cookie
    std::regex pattern(R"(([^=\s]+)=([^;\s]+))");

    std::sregex_iterator iter(cookieString.begin(), cookieString.end(), pattern);
    std::sregex_iterator end;
    
    while (iter != end) {
        std::smatch match = *iter;
        
        std::string name = match.str(1);
        std::string value = match.str(2);
        
        cookies_[name] = value;
        
        ++iter;
    }
}

void HttpRequest::ParseBody_(const string& line) {
    body_ = line;
    ParsePost_();
    state_ = FINISH;
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

int HttpRequest::ConverHex(char ch) {
    if(ch >= 'A' && ch <= 'F') return ch -'A' + 10;
    if(ch >= 'a' && ch <= 'f') return ch -'a' + 10;
    return ch;
}

void HttpRequest::ParsePost_() {
    if(method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
        ParseFromUrlencoded_();
        if(DEFAULT_HTML_TAG.count(path_)) {
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            LOG_DEBUG("Tag:%d", tag);
            if(tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);
                if(UserVerify(post_["username"], post_["password"], isLogin)) {

                    SessionPool* sessionPool = SessionPool::Instance();
                    // 验证session有效性，有效则更新session，无效则重新生成
                    if (cookies_.count("sessionId") == 1) {
                        if (sessionPool->sessionVerify(cookies_["sessionId"])) {
                            sessionPool->updateSession(cookies_["sessionId"]);
                        }
                        else {
                            sessionId_ = sessionPool->addSession(post_["username"]);
                            LOG_DEBUG("Generate session, ID: %s, username: %s", sessionId_.c_str(), post_["username"].c_str())
                        }
                    }
                    else {
                        sessionId_ = sessionPool->addSession(post_["username"]);
                        LOG_DEBUG("Generate session, ID: %s, username: %s", sessionId_.c_str(), post_["username"].c_str())
                    }

                    path_ = "/static/home.html";
                } 
                else {
                    path_ = "error.html";
                }
            }
        }
        else {
            for (const auto& pair : post_) {
                queryCond_.insert({pair.first, pair.second});
            }
        }
    }   
}

void HttpRequest::ParseFromUrlencoded_() {
    if(body_.size() == 0) { return; }

    string key, value;
    int num = 0;
    int n = body_.size();
    int i = 0, j = 0;

    for(; i < n; i++) {
        char ch = body_[i];
        switch (ch) {
        case '=':
            key = body_.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            body_[i] = ' ';
            break;
        case '%':
            num = ConverHex(body_[i + 1]) * 16 + ConverHex(body_[i + 2]);
            body_[i + 2] = num % 10 + '0';
            body_[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            value = body_.substr(j, i - j);
            j = i + 1;
            post_[key] = value;
            LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
            break;
        default:
            break;
        }
    }
    assert(j <= i);
    if(post_.count(key) == 0 && j < i) {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}

bool HttpRequest::UserVerify(const string &name, const string &pwd, bool isLogin) {
    if(name == "" || pwd == "") { return false; }
    LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
    MYSQL* sql;
    SqlConnRAII(&sql,  SqlConnPool::Instance());
    assert(sql);
    
    bool flag = false;
    unsigned int j = 0;
    char order[256] = { 0 };
    MYSQL_FIELD *fields = nullptr;
    MYSQL_RES *res = nullptr;
    
    if(!isLogin) { flag = true; }
    /* 查询用户及密码 */
    snprintf(order, 256, "SELECT username, password FROM doctors WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order);

    if(mysql_query(sql, order)) { 
        mysql_free_result(res);
        return false; 
    }
    res = mysql_store_result(sql);
    j = mysql_num_fields(res);
    fields = mysql_fetch_fields(res);

    while(MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        string password(row[1]);
        /* 注册行为 且 用户名未被使用*/
        if(isLogin) {
            if(pwd == password) { flag = true; }
            else {
                flag = false;
                LOG_DEBUG("pwd error!");
            }
        } 
        else { 
            flag = false; 
            LOG_DEBUG("user used!");
        }
    }
    mysql_free_result(res);

    /* 注册行为 且 用户名未被使用*/
    if(!isLogin && flag == true) {
        LOG_DEBUG("regirster!");
        bzero(order, 256);
        snprintf(order, 256,"INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG( "%s", order);
        if(mysql_query(sql, order)) { 
            LOG_DEBUG( "Insert error!");
            flag = false; 
        }
        flag = true;
    }
    SqlConnPool::Instance()->FreeConn(sql);
    LOG_DEBUG( "UserVerify success!!");
    return flag;
}

std::string HttpRequest::path() const{
    return path_;
}

std::string& HttpRequest::path(){
    return path_;
}
std::string HttpRequest::method() const {
    return method_;
}

std::string HttpRequest::version() const {
    return version_;
}

std::string HttpRequest::GetPost(const std::string& key) const {
    assert(key != "");
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}

std::string HttpRequest::GetPost(const char* key) const {
    assert(key != nullptr);
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}

std::string& HttpRequest::GetQueryTable() {
    return queryTable_;
}
std::string& HttpRequest::GetAction() {
    return action_;
}
std::unordered_map<std::string, std::string>& HttpRequest::GetQueryCond() {
    return queryCond_;
}

std::unordered_map<std::string, std::string>& HttpRequest::GetCookies() {
    return cookies_;
}