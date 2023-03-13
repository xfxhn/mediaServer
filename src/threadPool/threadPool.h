#ifndef THREAD_THREADPOOL_H
#define THREAD_THREADPOOL_H

#include <thread>
#include <mutex>
#include <vector>
#include <list>
#include <condition_variable>

class Task;

class ThreadPool {
public:
	//初始化线程池
	int init(int num);

	//启动线程
	int start();

	int addTask(Task* task);

	Task* getTask();

	void stop();

	~ThreadPool();

private:
	//入口函数
	void run();

	int threadNum = 0;


	std::mutex mutex;
	std::vector<std::thread*> threads;

	std::list<Task*> tasks;
	std::condition_variable cv;


	bool exitFlag{false};
};

#endif //THREAD_THREADPOOL_H
