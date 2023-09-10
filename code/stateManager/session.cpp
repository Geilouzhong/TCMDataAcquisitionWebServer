#include "session.h"

using namespace std;

const std::string Session::s_charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

Session::Session() {
    sessionId_ = generateSessionID_(16);
    userId_ = "";
    expireTime_ = chrono::system_clock::now() + chrono::minutes(30);
}

string Session::generateSessionID_(int length = 16) {
    
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, s_charset.length() - 1);
    string sessionID;
    for (int i = 0; i < length; ++i) {
        sessionID += s_charset[dis(gen)];
    }
    return sessionID;
}

string Session::getSessionID() const {
    return sessionId_;
}

string Session::getUserID() const {
    return userId_;
}

void Session::setUserID(const string & userId) {
    userId_ = userId;
}

void Session::update() {
    expireTime_ = chrono::system_clock::now() + chrono::minutes(30);
}

bool Session::verify() {
    return chrono::system_clock::now() < expireTime_;
}

void SessionPool::closeSessionPool() {
    lock_guard<mutex> lock(mtx_);
    for (auto it = pool_.begin(); it != pool_.end(); ++it) {
        delete it->second;
    }
}

SessionPool::~SessionPool() {
    closeSessionPool();
}

string SessionPool::addSession(string userId) {
    Session* newSession = new Session();
    newSession->setUserID(userId);
    string sessionId = newSession->getSessionID();
    lock_guard<mutex> lock(mtx_);
    pool_.insert({sessionId, newSession});
    return sessionId;
}

bool SessionPool::sessionVerify(string sessionId) {
    lock_guard<mutex> lock(mtx_);
    auto it = pool_.find(sessionId);
    if (it != pool_.end()) {
        return it->second->verify();
    }
    return false;
}

void SessionPool::updateSession(string sessionId) {
    lock_guard<mutex> lock(mtx_);
    auto it = pool_.find(sessionId);
    if (it != pool_.end()) {
        it->second->update();
    }
}

string SessionPool::getUserID(string sessionId) {
    lock_guard<mutex> lock(mtx_);
    auto it = pool_.find(sessionId);
    if (it != pool_.end()) {
        return it->second->getUserID();
    }
    return "";
}

void SessionPool::delSession(string sessionId) {
    lock_guard<mutex> lock(mtx_);
    auto it = pool_.find(sessionId);
    if (it != pool_.end()) {
        Session* tmp = it->second;
        pool_.erase(it);
        delete tmp;
    }
}

void SessionPool::print() {
    for (auto it = pool_.begin(); it != pool_.end(); ++it) {
        printf("%s\n", it->first.c_str());
    }
}

SessionPool* SessionPool::Instance() {
    static SessionPool sessionPool;
    return &sessionPool;
}