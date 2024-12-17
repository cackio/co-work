#pragma once

#include <iostream>
#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <functional>
#include <assert.h>
//线程池代码涉及很多c++11新特性
class thread_pool
{
public:
    
    thread_pool(const thread_pool& p) = default;            //默认构造和赋值构造
    thread_pool& operator=(const thread_pool& p) = default;
    thread_pool(int thread_num=10):stop(false)
    {
        assert(thread_num>0);
        for (int i = 0; i < thread_num; ++i)
        {
            threads.emplace_back([this]{             //不能用push_back，push_back会创建临时对象，会有拷贝
                while(1)
                {
                    std::unique_lock<std::mutex>locker(mtx); //加锁，因为要对线程进行访问
                    cv.wait(locker,[this]{return !tasks.empty() || stop ;});
                    if(tasks.empty()&&stop)return;
                    std::function<void()>task(std::move(tasks.front()));
                    tasks.pop();         //取出任务
                    locker.unlock();    //取出任务后解锁
                    task();             //执行任务
                }
            });
        }
        
    }


    template<class F,class ...Args>         //可变参数
    void add_task(F&& f,Args&&...args)      //在模版中&&意思是左值或者右值
    {
        //bind参数绑定
        std::function<void()>task = std::bind(std::forward<F>(f),std::forward<Args>(args)...); //forward完美转发，参数是什么值就传入什么值
        {
            std::unique_lock<std::mutex>locker(mtx);
            tasks.emplace(std::move(task));
        }
        cv.notify_one();        //唤醒线程
    }

    ~thread_pool()
    {
        {
        std::unique_lock<std::mutex>locker(mtx);          //共享数据都要加锁
        stop = true;
        }
        cv.notify_all();    //唤醒线程
        for(auto& it:threads)
        {
            it.join();        //等待所以线程执行完任务
        }
    }
private:
    std::condition_variable cv;         //条件变量
    std::mutex mtx;                   //锁
    std::queue<std::function<void()>>tasks; //任务队列
    std::vector<std::thread>threads;    //线程s
    bool stop;                     //标志
};