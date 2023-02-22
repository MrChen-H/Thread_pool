#pragma once
#pragma once

#include<thread>
#include<queue>
#include<future>
#include<list>
#include<mutex>
#include<condition_variable>
#include<shared_mutex>

namespace Thread_Pool
{

#define MAX_THREAD 1000 //最大线程数
#define MIN_THREAD 16   //最小线程数
#define RATE_DOWN  1.5
#define RATE_UP    2.0



    class thread_pool
    {
    private://成员
        std::queue<std::function<void()>> task_queue;//任务队列
        std::list<std::thread> workers_list;    //工作线程列表
        std::thread manager;                    //管理者线程,用于根据动态调整线程池大小

        std::mutex task_queue_mutex;            //任务队列锁
        std::mutex manager_mutex;               //管理者线程专用锁

        std::condition_variable condition;      //信号量,用于通知线程
        bool stop;                              //当线程池关闭时使用

    public://接口
        thread_pool();                          //构造函数
        ~thread_pool();                         //析构

        void manager_working();                 //管理者线程工作逻辑 
        void worker_working();                  //工作线程工作逻辑

        void create_thread(int create_number);  //线程创建函数 
        void delete_thread(int delete_number);  //线程释放函数 
        void insert_task(std::function<void()>);
        int get_worker_number() { return workers_list.size(); };//测试当前有多少线程
    };

    //实现部分
    thread_pool::thread_pool() :stop(false)          //构造函数
    {
       manager = std::move(std::thread([=] {
            manager_working();
            }));                                    //创建管理者者线程


        create_thread(MIN_THREAD);       //创建线程池
    }

    thread_pool::~thread_pool()                     //析构函数
    {
        stop = true;                                //关闭线程池
        condition.notify_all();                     //唤醒当前池中所有线程

        for (auto& it : workers_list)     //遍历列表并关闭
        {
            it.join();
        }
        manager.join();                             //关闭管理者线程
    }

    void thread_pool::create_thread(int create_number)
    {
        if (stop)                                    //当线程池被关闭时
        {
            return;
        }
        for (int i = 0; i < create_number; i++)
        {
            workers_list.emplace_back([=] {          //创建工作线程并加入队列中
                worker_working();
                });
        }
    }

    void thread_pool::delete_thread(int delete_number)
    {
        if (stop)                                    //线程池关闭时退出
        {
            return;
        }

        for (int i = 0; i < delete_number; i++)
        {
            if (workers_list.size() > MIN_THREAD)      //保持总工作线程数不低于最低线程数
            {
                std::thread T = std::move(workers_list.back());
                                          //等待线程执行结束退出
                workers_list.pop_back();
                T.join(); 
            }
            else
            {
                return;
            }

        }
    }

    void thread_pool::manager_working()
    {
        std::unique_lock<std::mutex> lock(manager_mutex);

        while (!stop)
        {
            condition.wait(lock, [&] {
                return stop || ((workers_list.size() < task_queue.size()|| workers_list.size() > task_queue.size() * RATE_UP) && task_queue.size() > MIN_THREAD);
                });//当关闭线程池或者工作线程总数小于任务数量*最低倍率时唤醒管理者增加线程,或者当线程总数大于任务数*最高倍率时唤醒管理者删除线程,且只有当任务数量大于最低线程数量才会唤醒管理者

            if (workers_list.size() < task_queue.size())
            {
                if ((task_queue.size() * RATE_DOWN - workers_list.size()) < MAX_THREAD)
                {
                    create_thread(task_queue.size() * RATE_DOWN - workers_list.size());
                }
                else
                {
                    create_thread(MAX_THREAD-workers_list.size());
                }
            }
            else if (workers_list.size() > task_queue.size())
            {
                if ((workers_list.size() - task_queue.size() * RATE_DOWN) > MIN_THREAD)
                {
                    delete_thread( workers_list.size() - task_queue.size() * RATE_DOWN);
                   
                }
                else
                {
                    delete_thread(workers_list.size()-MIN_THREAD);
                }
            }
        }
    }

    void thread_pool::worker_working()
    {
        while (!stop)
        {
            std::unique_lock<std::mutex> lock(task_queue_mutex);
            condition.wait(lock, [=] { return stop || task_queue.size() > 0; });
            if (stop)
            {
                return;
            }
            std::function<void()> task = std::move(task_queue.front());
            task_queue.pop();
            task();
        }
    }

    void thread_pool::insert_task(std::function<void()> task_func)
    {
        std::unique_lock clock(task_queue_mutex);
        task_queue.push(task_func);
        condition.notify_one();
    }
} // namespace Thread_Pool
