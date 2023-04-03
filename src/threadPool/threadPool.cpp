#include "threadPool.h"
#include "task.h"

int ThreadPool::init(int num) {
    threadNum = num;

    return 0;
}


int ThreadPool::start() {
    if (threadNum <= 0) {
        fprintf(stderr, "请初始化线程池\n");
        return -1;
    }

    if (!threads.empty()) {
        fprintf(stderr, "已经初始化\n");
        return -1;
    }

    for (int i = 0; i < threadNum; ++i) {
        threads.push_back(new std::thread(&ThreadPool::run, this));
    }
    return 0;
}



int ThreadPool::addTask(Task *task) {
    std::unique_lock<std::mutex> lock(mutex);

    tasks.push_back(task);

    /*唤醒一个线程去执行任务*/
    cv.notify_one();

    return 0;
}

void ThreadPool::run() {
    //这里所有线程都会去抢任务，抢到了就执行，抢不到就阻塞
    int ret = 0;
    while (!exitFlag) {
        Task *task = getTask();
        if (!task) {
            continue;
        }

        //这里最好try一下，防止整个线程池崩溃
        //try {
        ret = task->run();
        task->setVal(ret);


        delete task;
//        } catch (...) {
//            continue;
//        }
    }

}

Task *ThreadPool::getTask() {
    std::unique_lock<std::mutex> lock(mutex);


    cv.wait(lock, [this] {
        return !exitFlag && !tasks.empty();
    });


    if (exitFlag) {
        return nullptr;
    }

    //防止多次通知
    if (tasks.empty()) {
        return nullptr;
    }

    Task *task = tasks.front();
    tasks.pop_front();
    return task;
}

void ThreadPool::stop() {

    exitFlag = true;

    cv.notify_all();
    for (std::thread *th: threads) {
        th->join();
    }
}

ThreadPool::~ThreadPool() {
    for (auto &th: threads) {
        delete th;
    }

    threads.clear();
}