#include "task.h"

void Task::setVal(int val) {
    promise.set_value(val);
}

int Task::getVal() {
    return promise.get_future().get();
}
