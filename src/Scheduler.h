
#pragma once

#include "basic.h"

class Scheduler {
private:
    class Bucket {
    public:
        Bucket() = default;
        std::mutex mu;
	    std::condition_variable cv;
        atomic<int> cnt{0};
    private:
        Bucket(const Bucket&) = delete;
        Bucket& operator=(const Bucket&) = delete;
    };

    static const int kTickCnt = 12;
    Bucket ticks_[kTickCnt];
    int cursor_ = 0;
    std::thread worker_;

    enum Direction {
        GoUp = 0,
        GoDown = 1,
    };
    Direction direct_ = GoUp;


public:
    Scheduler() = default;
    // qps control
    bool GetTick(long idx);

    void Start();

private:
    void TicTic();
    void NextTick();
};