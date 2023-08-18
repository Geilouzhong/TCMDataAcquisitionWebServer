#ifndef LFU_CACHE_H
#define LFU_CACHE_H

#include <string>
#include <mutex>
#include <unordered_map>
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
    string key_, value_;
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

    bool get(string& key, string& value);
    void set(string& key, string& value);
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

void KeyList::init(int freq) {
    freq_ = freq;
    Dummyhead_ = newElement<Node<Key>>();
    tail_ = Dummyhead_;
    Dummyhead_->setNext(nullptr);
}

void KeyList::destory() {
    while (Dummyhead_ !=  nullptr) {
        key_node pre = Dummyhead_;
        Dummyhead_ = Dummyhead_->getNext();
        deleteElement(pre);
    }
}

// 节点添加到链表头部
void KeyList::add(key_node& node) {
    if (Dummyhead_->getNext() == nullptr) {
        tail_ = node;
    }
    else {
        Dummyhead_->getNext()->setPre(node);
    }
    node->setNext(Dummyhead_->getNext());
    node->setPre(Dummyhead_);
    Dummyhead_->setNext(node);

    assert(!empty());
}

void KeyList::del(key_node& node) {
    node->getPre()->setNext(node->getNext());
    if (node->getNext() == nullptr) {
        tail_ = node->getPre();
    }
    else {
        node->getNext()->setPre(node->getPre());
    }
}

void LFUCache::init() {
    Dummyhead_ = newElement<Node<KeyList>>();
    Dummyhead_->getValue().init(0);
    Dummyhead_->setNext(nullptr);
}

LFUCache::~LFUCache() {
    while (Dummyhead_) {
        freq_node pre = Dummyhead_;
        Dummyhead_ = Dummyhead_->getNext();
        pre->getValue().destory();
        deleteElement(pre);
    }
}

bool LFUCache::get(string& key, string& val) {
    if (!capacity_) return false;
    std::lock_guard<std::mutex> lock(mtx_);
    if (fmap_.find(key) != fmap_.end()) {
        key_node nowk = kmap_[key];
        freq_node nowf = fmap_[key];
        val += nowk->getValue().value_;
        addFreq(nowk, nowf);
        return true;
    }
    return false;
}

void LFUCache::del(freq_node& node) {
    node->getPre()->setNext(node->getNext());
    if (node->getNext() != nullptr) {
        node->getNext()->setPre(node->getPre());
    }
    node->getValue().destory();
    deleteElement(node);
}

void LFUCache::addFreq(key_node& nowk, freq_node& nowf) {
    freq_node nxt;
    if (nowf->getNext() == nullptr ||
        nowf->getNext()->getValue().getFreq() != nowf->getValue().getFreq() + 1) {
            nxt = newElement<Node<KeyList>>();
            nxt->getValue().init(nowf->getValue().getFreq() + 1);
            if (nowf->getNext() != nullptr) {
                nowf->getNext()->setPre(nxt);
            }
            nxt->setNext(nowf->getNext());
            nowf->setNext(nxt);
            nxt->setPre(nowf);
    }
    else {
        nxt = nowf->getNext();
    }
    fmap_[nowk->getValue().key_] = nxt;
    if (nowf != Dummyhead_) {
        nowf->getValue().del(nowk);
    }
    nxt->getValue().add(nowk);
    assert(!nxt->getValue().empty());
    if (nowf != Dummyhead_ && nxt->getValue().empty()) {
        del(nowf);
    }
}

void LFUCache::set(string& key, string& val) {
    if (!capacity_) return;
    std::lock_guard<std::mutex> lock(mtx_);
    if (kmap_.size() == capacity_) {
        freq_node head = Dummyhead_->getNext();
        key_node last = head->getValue().getLast();
        head->getValue().del(last);
        kmap_.erase(last->getValue().key_);
        fmap_.erase(last->getValue().key_);
        deleteElement(last);
        if (head->getValue().empty()) {
            del(head);
        }
    }
    key_node nowk = newElement<Node<Key>>();
    nowk->getValue().key_ = key;
    nowk->getValue().value_ = val;
    addFreq(nowk, Dummyhead_);
    kmap_[key] = nowk;
    fmap_[key] = Dummyhead_->getNext();
}

LFUCache& getCache() {
    static LFUCache cache(LFU_CAPACITY);
    return cache;
}

#endif