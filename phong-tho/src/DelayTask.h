//
// Created by Thái Nguyễn on 30/1/25.
//

#ifndef PHONG_THO_DELAYTASK_H
#define PHONG_THO_DELAYTASK_H

#include <Arduino.h>

class DelayTask {

private:
    uint32_t _delayTime;
    uint32_t _lastTick = 0;
    bool _executed = false;
    std::function<void()> _task;

public:

    DelayTask() = default;

    DelayTask(uint32_t delayTime, std::function<void()> task) {
        _delayTime = delayTime;
        _task = task;
    }
    ~DelayTask() = default;

    void setDelayTime(uint32_t delayTime) {
        _delayTime = delayTime;
    }

    void setTask(std::function<void()> task) {
        _task = task;
    }

    void tick() {
        if (_executed) _executed = false;
        _lastTick = millis();
    }

    void run() {
        if (!_executed && millis() - _lastTick >= _delayTime) {
            _task();
            _executed = true;
        }
    }
};

#endif //PHONG_THO_DELAYTASK_H
