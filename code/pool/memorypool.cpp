#include "memorypool.h"

MemoryPool::MemoryPool() {}

MemoryPool::~MemoryPool() {
    Slot* cur = currentBolck_;
    while(cur) {
        Slot* next = cur->next;
        // free(reinterpret_cast<void *>(cur));
        // 转化为 void 指针，是因为 void 类型不需要调⽤析构函数,只释放空间
        operator delete(reinterpret_cast<void*>(cur));
        cur = next;
    }
}

void MemoryPool::init(int size) {
    assert(size > 0);
    slotSize_ = size;
    currentBolck_ = nullptr;
    currentSlot_ = nullptr;
    lastSlot_ = nullptr;
    freeSlot_ = nullptr;
}

// Block的第一个Slot用于放下一个Block的指针（Slot*），剩下空间需要对齐
// padPointer主要用于计算对齐需要补的空间
inline size_t MemoryPool::padPointer(char* p, size_t align) {
    size_t result = reinterpret_cast<size_t>(p);
    return ((align - result) % align); 
}

Slot* MemoryPool::allocateBlock() {
    char* newBlock = reinterpret_cast<char *>(operator new(BlockSize));
    char* body = newBlock + sizeof(Slot*);
    // 计算为了对⻬需要空出多少位置
    // size_t bodyPadding = padPointer(body, sizeof(slotSize_));
    size_t bodyPadding = padPointer(body, static_cast<size_t>(slotSize_));
    
    // 注意：多个线程（eventLoop 共⽤⼀个MemoryPool）
    Slot* useSlot;
    {
        std::lock_guard<std::mutex> lock(mtx_other_);
        // newBlock接到Block链表的头部
        reinterpret_cast<Slot *>(newBlock)->next = currentBolck_;
        currentBolck_ = reinterpret_cast<Slot *>(newBlock);
        // 为该Block开始的地⽅加上bodyPadding个char* 空间
        currentSlot_ = reinterpret_cast<Slot *>(body + bodyPadding);
        lastSlot_ = reinterpret_cast<Slot *>(newBlock + BlockSize - slotSize_ + 1);
        useSlot = currentSlot_;
        // slot指针⼀次移动8个字节
        currentSlot_ += (slotSize_ >> 3);
    }
    return useSlot;
}

Slot* MemoryPool::nofree_solve() {
    if(currentSlot_ >= lastSlot_)
        return allocateBlock();
    Slot* useSlot;
    {
        std::lock_guard<std::mutex> lock(mtx_other_);
        useSlot = currentSlot_;
        currentSlot_ += (slotSize_ >> 3);
    }
    return useSlot;
}

Slot* MemoryPool::allocate() {
    if (freeSlot_) {
        {
            std::lock_guard<std::mutex> lock(mtx_freeSlot_);
            if (freeSlot_) {
                Slot* result = freeSlot_;
                freeSlot_ = freeSlot_->next;
                return result;
            }
        }
    }

    return nofree_solve();
}

inline void MemoryPool::deAllocate(Slot* p) {
    if (p) {
        // 将slot加⼊释放队列
        std::lock_guard<std::mutex> lock(mtx_freeSlot_);
        p->next = freeSlot_;
        freeSlot_ = p;
    }
}

MemoryPool& get_MemoryPool(int id) {
    static MemoryPool memorypool_[64];
    return memorypool_[id];
}

// 数组中分别存放Slot⼤⼩为8，16，...，512字节的BLock链表
void init_MemoryPool() {
    for(int i = 0; i < 64; ++i) {
        get_MemoryPool(i).init((i + 1) << 3);
    }
}

// 超过512字节就直接new
void* use_Memory(size_t size) {
    if(!size)
        return nullptr;
    if(size > 512)
        return operator new(size);
    
    // 相当于(size / 8)向上取整
    return reinterpret_cast<void *>(get_MemoryPool(((size + 7) >> 3) - 1).allocate());
}

void free_Memory(size_t size, void* p) {
    if (!p) return;
    if (size > 512) {
        operator delete (p);
        return;
    }
    get_MemoryPool(((size + 7) >> 3) - 1).deAllocate(reinterpret_cast<Slot*>(p));
}