#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#define BlockSize 4096

#include <mutex>
#include <assert.h>

struct Slot {
    Slot *next;
};

class MemoryPool {
public:
    MemoryPool();
    ~MemoryPool();

    void init(int size);

    Slot *allocate();
    void deAllocate(Slot *p);

private:
    int slotSize_;

    Slot *currentBolck_;
    Slot *currentSlot_;
    Slot *lastSlot_;
    Slot *freeSlot_;

    std::mutex mtx_freeSlot_;
    std::mutex mtx_other_;

    size_t padPointer(char *p, size_t align);
    Slot *allocateBlock();
    Slot *nofree_solve();
};

void init_MemoryPool();
void* use_Memory(size_t size);
void free_Memory(size_t size, void* p);
MemoryPool& get_MemoryPool(int id);

template <typename T, typename... Args>
T *newElement(Args &&...args) {
    T *p;
    if ((p = reinterpret_cast<T *>(use_Memory(sizeof(T)))) != nullptr)
        // new(p) T1(value);
        // placement new:在指针p所指向的内存空间创建⼀个T1类型的对象，类似与realloc
        // 把已有的空间当成⼀个缓冲区来使⽤，减少了分配空间所耗费的时间
        // 因为直接⽤new操作符分配内存的话，在堆中查找⾜够⼤的剩余空间速度是⽐较慢的
        new (p) T(std::forward<Args>(args)...); // 完美转发
    return p;
}

// 调⽤p的析构函数，然后将其总内存池中释放
template <typename T>
void deleteElement(T *p) {
    // printf("deleteElement...\n");
    if (p)
        p->~T();
    free_Memory(sizeof(T), reinterpret_cast<void *>(p));
    // printf("deleteElement success\n");
}

#endif