#pragma once
#include <deque>
#include <condition_variable>
#include <mutex>
#include <sys/time.h>

template<typename T>
class BlockQueue 
{
public:
    //阻塞队列底层是deque，通过封装deque的成员方法和变量实现BlockQueue数据类型
    explicit BlockQueue(size_t maxsize = 1000);
    ~BlockQueue();
    void flush();
    void Close();

    bool Is_empty();
    bool Is_full();

    void push_back(const T& item);
    void push_front(const T& item); 

    bool pop(T& item);  // 弹出的任务放入item
    bool pop(T& item, int timeout);  // 等待时间
    void clear();
    T front();
    T back();

    size_t capacity();
    size_t size();

private:
    std::deque<T> deq_;                     // 底层数据结构deque
    std::mutex mtx_;                           // 锁
    std::condition_variable cv_Consumer;   // 消费者条件变量cv
    std::condition_variable cv_Producer;   // 生产者条件变量cv
    bool is_Close;                                // 关闭标志
    size_t dp_capacity;                   // 容量

};
template<typename T>
BlockQueue<T>::BlockQueue(size_t maxsize){
    //assert(maxsize > 0);
    dp_capacity = maxsize;
    is_Close = false;
}

template<typename T>
BlockQueue<T>::~BlockQueue() {
    Close();
}

template<typename T>
void BlockQueue<T>::Close() {
    clear();
    is_Close = true;
    cv_Consumer.notify_all();
    cv_Producer.notify_all();
}

template<typename T>
void BlockQueue<T>::clear() {
    std::lock_guard<std::mutex> locker(mtx_);
    deq_.clear();
}

template<typename T>
bool BlockQueue<T>::Is_empty() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.empty();
}

template<typename T>
bool BlockQueue<T>::Is_full() {
    
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size() >= dp_capacity;
}

template<typename T>
void BlockQueue<T>::push_back(const T& item) 
{
    // 注意，条件变量需要搭配unique_lock
    std::unique_lock<std::mutex> locker(mtx_);    //unique_lock相比lock_guard更加灵活
    while(deq_.size() >= dp_capacity) {   // 队列满了，需要等待
        cv_Producer.wait(locker);     // 暂停生产，等待消费者唤醒生产条件变量
    }
    deq_.push_back(item);
    cv_Consumer.notify_one();         // 唤醒消费者
}

template<typename T>
void BlockQueue<T>::push_front(const T& item) 
{
    std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.size() >= dp_capacity) {   // 队列满了，需要等待
        cv_Producer.wait(locker);     // 暂停生产，等待消费者唤醒生产条件变量
    }
    deq_.push_front(item);
    cv_Consumer.notify_one();         // 唤醒消费者
}

template<typename T>
bool BlockQueue<T>::pop(T& item) 
{
    std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.empty()) {
        cv_Consumer.wait(locker);     // 队列空了，需要等待
    }
    item = deq_.front();
    deq_.pop_front();
    cv_Producer.notify_one();         // 唤醒生产者
    return true;
}

template<typename T>
bool BlockQueue<T>::pop(T &item, int timeout) {
    std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.empty()){
        if(cv_Consumer.wait_for(locker, std::chrono::seconds(timeout)) 
                == std::cv_status::timeout){
            return false;
        }
        if(is_Close){
            return false;
        }
    }
    item = deq_.front();
    deq_.pop_front();
    cv_Producer.notify_one();
    return true;
}

template<typename T>
T BlockQueue<T>::front() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.front();
}

template<typename T>
T BlockQueue<T>::back() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.back();
}

template<typename T>
size_t BlockQueue<T>::capacity() {
    std::lock_guard<std::mutex> locker(mtx_);
    return dp_capacity;
}

template<typename T>
size_t BlockQueue<T>::size() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size();
}

// 唤醒消费者
template<typename T>
void BlockQueue<T>::flush() {
    cv_Consumer.notify_one();
}

