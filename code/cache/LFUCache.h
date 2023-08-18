#ifndef LFU_CACHE_H
#define LFU_CACHE_H

#include <string>
#include <mutex>
#include <unordered_map>
#include <sys/mman.h>
#include <sys/stat.h>
#include "../pool/memorypool.h"

#define LFU_CAPACITY 10

using std::string;

template<typename T>
class Node {
public:
    void setPre(Node* p) { pre_ = p; }
    void setNext(Node* p) { next_ = p; }
    Node* getPre() { return pre_; }
    Node* getNext() { return next_; }
    T& getValue() { return value_; }
private:
    T value_;
    Node* pre_;
    Node* next_;
};

struct Key {
    string key_;
    char* mmFile_;
    struct stat mmFileStat_;
    void unmapFile() {
        if(mmFile_) {
            munmap(mmFile_, mmFileStat_.st_size);
        }
    }
};

typedef Node<Key>* key_node;

class KeyList {
public:
    void init(int freq);
    void destory();
    int getFreq() { return freq_; }
    void add(key_node& node);
    void del(key_node& node);
    bool empty() { return Dummyhead_ == tail_; }
    key_node getLast() { return tail_; };

private:
    int freq_;
    key_node Dummyhead_;
    key_node tail_;
};

typedef Node<KeyList>* freq_node;

class LFUCache {
public:
    LFUCache(int capacity) { init(); }
    ~LFUCache();

    bool get(string& key, char* mmFile, struct stat& mmFileStat);
    void set(string& key, char* mmFile, struct stat& mmFileStat);
    size_t getCapacity() const { return capacity_; }

private:
    void init();
    void addFreq(key_node& nowk, freq_node& nowf);
    void del(freq_node& node);

    freq_node Dummyhead_;
    size_t capacity_;
    std::mutex mtx_;

    std::unordered_map<string, key_node> kmap_;
    std::unordered_map<string, freq_node> fmap_;
};

LFUCache& getCache();

#endif