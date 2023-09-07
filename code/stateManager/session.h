#ifndef SESSION_H
#define SESSION_H

#include <string>
#include <unordered_map>
#include <random>
#include <chrono>
#include <mutex>

class Session {
public:
    Session();
    ~Session() = default;
    
    void setUserID(const std::string& userId);
    void update();
    std::string getUserID() const;
    std::string getSessionID() const;
    bool verify();
private:
    std::string sessionId_;
    std::string userId_;
    std::chrono::_V2::system_clock::time_point expireTime_;
    std::string generateSessionID_(int length);

    static const std::string s_charset;
};

class SessionPool {
public:
    static SessionPool* Instance();
    bool sessionVerify(std::string sessionId);
    std::string addSession(std::string userId);
    void updateSession(std::string sessionId);
    std::string getUserID(std::string sessionId);
    void closeSessionPool(); 

private:
    std::unordered_map<std::string, Session*> pool_;
    std::mutex mtx_;

    SessionPool() = default;
    ~SessionPool();
};

#endif