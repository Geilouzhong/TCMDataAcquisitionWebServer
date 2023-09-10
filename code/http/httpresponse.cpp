#include "httpresponse.h"

using namespace std;

const unordered_map<string, string> HttpResponse::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const unordered_map<int, string> HttpResponse::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 401, "Unauthorized"},
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

const unordered_map<int, string> HttpResponse::CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};

HttpResponse::HttpResponse() {
    code_ = -1;
    path_ = srcDir_ = "";
    isKeepAlive_ = false;
    mmFile_ = nullptr; 
    mmFileStat_ = { 0 };
};

void HttpResponse::Init(const string& srcDir, string& path, bool isKeepAlive, int code, string sessionId){
    assert(srcDir != "");
    code_ = code;
    isKeepAlive_ = isKeepAlive;
    isAccessStatic = true;
    path_ = path;
    srcDir_ = srcDir;
    mmFile_ = nullptr; 
    mmFileStat_ = { 0 };
    queryResult_ = "";
    sessionId_ = sessionId;
}

void HttpResponse::MakeResponse(Buffer& buff) {
    /* 判断请求的资源文件 */
    if(stat((srcDir_ + path_).data(), &mmFileStat_) < 0 || S_ISDIR(mmFileStat_.st_mode)) {
        code_ = 404;
    }
    else if(!(mmFileStat_.st_mode & S_IROTH)) {
        code_ = 403;
    }
    else if(code_ == -1) { 
        code_ = 200; 
    }
    ErrorHtml_();
    AddStateLine_(buff);
    AddHeader_(buff);
    AddContent_(buff);
}

void HttpResponse::SQLResponse(Buffer& buff, string& queryTable, string& action, 
        unordered_map<string, string>& queryCond, unordered_map<string, string>& cookies) {
    
    if (cookies.count("sessionId") < 1) {
        code_ = 401;
        LOG_DEBUG("User not log in!")
    }
    else {
        SessionPool* sessionPool = SessionPool::Instance();
        LOG_DEBUG("Verify Session Id: %s", cookies["sessionId"].c_str())
        if (sessionPool->sessionVerify(cookies["sessionId"])) {
            sessionPool->updateSession(cookies["sessionId"]);

            string doctorUsername = sessionPool->getUserID(cookies["sessionId"]);
            string doctorId = sqlAction::getDoctorIdByUsername(doctorUsername);
            LOG_DEBUG("doctor ID: %s", doctorId.c_str());

            queryCond.insert({"doctorIdInRecord", doctorId});
            if (action.substr(0, 3) == "get") {
                sqlAction::getQueryResult(queryResult_, queryTable, action, queryCond);
                LOG_DEBUG("Query result: %s", queryResult_.c_str())
                code_ = 200;
            }
            else if (action.substr(0, 3) == "add"){
                if (sqlAction::insertRecord(queryResult_, queryTable, action, queryCond)) {
                    code_ = 200;
                }
                else {
                    code_ = 400;
                }
            }
            else if (action.substr(0, 6) == "update") {
                if (sqlAction::updateRecord(queryResult_, queryTable, action, queryCond)) {
                    code_ = 200;
                }
                else {
                    code_ = 400;
                }
            }
        }
        else {
            code_ = 401;
            LOG_DEBUG("User authentication failure!")
        }
    }
    AddStateLine_(buff);
    AddHeader_(buff);
    buff.Append("Content-length: " + to_string(queryResult_.size()) + "\r\n\r\n");
}

char* HttpResponse::File() {
    if (isAccessStatic) {
        return mmFile_;
    }
    return &queryResult_[0];
}

size_t HttpResponse::FileLen() const {
    if (isAccessStatic) {
        return mmFileStat_.st_size;
    }
    return queryResult_.size();
}

void HttpResponse::ErrorHtml_() {
    if(CODE_PATH.count(code_) == 1) {
        path_ = CODE_PATH.find(code_)->second;
        stat((srcDir_ + path_).data(), &mmFileStat_);
    }
}

void HttpResponse::AddStateLine_(Buffer& buff) {
    string status;
    if(CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    }
    else {
        code_ = 400;
        status = CODE_STATUS.find(400)->second;
    }
    buff.Append("HTTP/1.1 " + to_string(code_) + " " + status + "\r\n");
}

void HttpResponse::AddHeader_(Buffer& buff) {
    buff.Append("Connection: ");
    if(isKeepAlive_) {
        buff.Append("keep-alive\r\n");
        buff.Append("keep-alive: max=6, timeout=120\r\n");
    } else{
        buff.Append("close\r\n");
    }
    buff.Append("Content-type: " + GetFileType_() + "\r\n");
    if (sessionId_ != "") {
        buff.Append("Set-Cookie: sessionId=" + sessionId_ + "; SameSite=Lax\r\n");
    }
    // buff.Append("Access-Control-Allow-Origin: *\r\n");
}

void HttpResponse::AddContent_(Buffer& buff) {
    getCache();
    LFUCache& Cache = getCache();
    string filename = srcDir_ + path_;
    // 缓存命中
    if (Cache.get(filename, mmFile_, mmFileStat_)) { }
    else {
        int srcFd = open(filename.data(), O_RDONLY);
        if(srcFd < 0) { 
            ErrorContent(buff, "File NotFound!");
            return; 
        }

        /* 将文件映射到内存提高文件的访问速度 
            MAP_PRIVATE 建立一个写入时拷贝的私有映射*/
        int* mmRet = (int*)mmap(0, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
        if(*mmRet == -1) {
            ErrorContent(buff, "File NotFound!");
            return; 
        }
        mmFile_ = (char*)mmRet;
        Cache.set(filename, mmFile_, mmFileStat_);
        close(srcFd);
    }
    LOG_DEBUG("file path %s", (srcDir_ + path_).data());
    buff.Append("Content-length: " + to_string(mmFileStat_.st_size) + "\r\n\r\n");
}

string HttpResponse::GetFileType_() {
    if (!isAccessStatic) {
        return "application/json";
    }
    /* 判断文件类型 */
    string::size_type idx = path_.find_last_of('.');
    if(idx == string::npos) {
        return "text/plain";
    }
    string suffix = path_.substr(idx);
    if(SUFFIX_TYPE.count(suffix) == 1) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

void HttpResponse::ErrorContent(Buffer& buff, string message) 
{
    string body;
    string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if(CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    } else {
        status = "Bad Request";
    }
    body += to_string(code_) + " : " + status  + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    buff.Append("Content-length: " + to_string(body.size()) + "\r\n\r\n");
    buff.Append(body);
}
