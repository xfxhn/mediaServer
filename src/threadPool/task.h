
#ifndef THREAD_TASH_H
#define THREAD_TASH_H

#include <future>

class Task {
public:
    virtual int run() = 0;

    void setVal(int val);

    int getVal();

    virtual ~Task() = default;

private:
    std::promise<int> promise;
};

#endif //THREAD_TASH_H
