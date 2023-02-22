//#include "Thread_pool_head.hpp"
#include "Thread_Pool.hpp"
#include<iostream>
#include<fstream>

#define THREAD_NUMBER 1 

void show(int a,int b)
{
	std::cout << "ID: " << std::this_thread::get_id() << " Working start\n";
}

int main()
{
	Thread_Pool::thread_pool pool;
	std::fstream file_open("ID_Data.txt",std::ios::in|std::ios::out);
	std::function<void()> func = std::bind(show,1,2);

	for (int i=0;i< THREAD_NUMBER;i++)
	{
		pool.insert_task(func);
		file_open << "Now have " << pool.get_worker_number()<<" threads.\n";
	}
	system("pause");
	return 0;
}