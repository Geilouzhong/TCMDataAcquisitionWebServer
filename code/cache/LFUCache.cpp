#include "LFUCache.h"

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
        pre->getValue().unmapFile();
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
    node->getValue().unmapFile();
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

bool LFUCache::get(string& key, char* mmFile, struct stat& mmFileStat) {
    if (!capacity_) return false;
    std::lock_guard<std::mutex> lock(mtx_);
    if (fmap_.find(key) != fmap_.end()) {
        key_node nowk = kmap_[key];
        freq_node nowf = fmap_[key];
        mmFile = nowk->getValue().mmFile_;
        mmFileStat = nowk->getValue().mmFileStat_;
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

void LFUCache::set(string& key, char* mmFile, struct stat& mmFileStat) {
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
    nowk->getValue().mmFile_ = mmFile;
    nowk->getValue().mmFileStat_ = mmFileStat;
    addFreq(nowk, Dummyhead_);
    kmap_[key] = nowk;
    fmap_[key] = Dummyhead_->getNext();
}

LFUCache& getCache() {
    static LFUCache cache(LFU_CAPACITY);
    return cache;
}