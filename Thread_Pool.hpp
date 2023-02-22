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

#define MAX_THREAD 1000 //����߳���
#define MIN_THREAD 16   //��С�߳���
#define RATE_DOWN  1.5
#define RATE_UP    2.0



    class thread_pool
    {
    private://��Ա
        std::queue<std::function<void()>> task_queue;//�������
        std::list<std::thread> workers_list;    //�����߳��б�
        std::thread manager;                    //�������߳�,���ڸ��ݶ�̬�����̳߳ش�С

        std::mutex task_queue_mutex;            //���������
        std::mutex manager_mutex;               //�������߳�ר����

        std::condition_variable condition;      //�ź���,����֪ͨ�߳�
        bool stop;                              //���̳߳عر�ʱʹ��

    public://�ӿ�
        thread_pool();                          //���캯��
        ~thread_pool();                         //����

        void manager_working();                 //�������̹߳����߼� 
        void worker_working();                  //�����̹߳����߼�

        void create_thread(int create_number);  //�̴߳������� 
        void delete_thread(int delete_number);  //�߳��ͷź��� 
        void insert_task(std::function<void()>);
        int get_worker_number() { return workers_list.size(); };//���Ե�ǰ�ж����߳�
    };

    //ʵ�ֲ���
    thread_pool::thread_pool() :stop(false)          //���캯��
    {
       manager = std::move(std::thread([=] {
            manager_working();
            }));                                    //�������������߳�


        create_thread(MIN_THREAD);       //�����̳߳�
    }

    thread_pool::~thread_pool()                     //��������
    {
        stop = true;                                //�ر��̳߳�
        condition.notify_all();                     //���ѵ�ǰ���������߳�

        for (auto& it : workers_list)     //�����б��ر�
        {
            it.join();
        }
        manager.join();                             //�رչ������߳�
    }

    void thread_pool::create_thread(int create_number)
    {
        if (stop)                                    //���̳߳ر��ر�ʱ
        {
            return;
        }
        for (int i = 0; i < create_number; i++)
        {
            workers_list.emplace_back([=] {          //���������̲߳����������
                worker_working();
                });
        }
    }

    void thread_pool::delete_thread(int delete_number)
    {
        if (stop)                                    //�̳߳عر�ʱ�˳�
        {
            return;
        }

        for (int i = 0; i < delete_number; i++)
        {
            if (workers_list.size() > MIN_THREAD)      //�����ܹ����߳�������������߳���
            {
                std::thread T = std::move(workers_list.back());
                                          //�ȴ��߳�ִ�н����˳�
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
                });//���ر��̳߳ػ��߹����߳�����С����������*��ͱ���ʱ���ѹ����������߳�,���ߵ��߳�������������������*��߱���ʱ���ѹ�����ɾ���߳�,��ֻ�е�����������������߳������Żỽ�ѹ�����

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
