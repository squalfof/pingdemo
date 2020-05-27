
#include "Scheduler.h"

bool Scheduler::GetTick(long idx) {
    auto& bucket = ticks_[idx % kTickCnt];
    if (bucket.cnt > 50) {
        return false;
    }
    std::unique_lock<std::mutex> umu(bucket.mu);
    bucket.cnt++;
    bucket.cv.wait(umu);

    return true;
}

void Scheduler::Start() {
    worker_ = std::thread(&Scheduler::TicTic, this);
    // TBD: join()
}

void Scheduler::TicTic() {
    while (true) {
        auto& bucket = ticks_[cursor_];
        int duration_ms = 0;
        {
            std::unique_lock<std::mutex> umu(bucket.mu);
            if (bucket.cnt) {
                duration_ms = bucket.cnt > 10 ? 10 : 5;
                bucket.cnt = 0;
            }
            bucket.cv.notify_all();
        }
        // wait for current round work
        if (duration_ms) {
            std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
        }
        NextTick();
    }
}

void Scheduler::NextTick() {
    // elevator
    if (direct_ == GoUp) {
        if (cursor_+1 == kTickCnt) {
            cursor_--;
            direct_ = GoDown;
        } else {
            cursor_++;
        }
    } else {
        if (cursor_ == 0) {
            cursor_++;
            direct_ = GoUp;
        } else {
            cursor_--;
        }
    }
}

