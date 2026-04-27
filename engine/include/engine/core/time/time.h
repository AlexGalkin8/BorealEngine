#pragma once

#include <chrono>

namespace core {

class Time
{
public:
    using clock = std::chrono::high_resolution_clock;

    Time()
        : start_time_(clock::now())
        , last_frame_time_(start_time_)
        , delta_seconds_(0.0)
        , total_seconds_(0.0)
        , paused_(false)
    {
    }

    void reset()
    {
        start_time_ = clock::now();
        last_frame_time_ = start_time_;
        delta_seconds_ = 0.0;
        total_seconds_ = 0.0;
        paused_ = false;
    }

    void pause()
    {
        if (paused_)
            return;

        paused_ = true;
        delta_seconds_ = 0.0;
    }

    void resume()
    {
        if (!paused_)
            return;

        paused_ = false;
        last_frame_time_ = clock::now();
        delta_seconds_ = 0.0;
    }

    bool isPaused() const { return paused_; }

    double tick(double min_dt = 0.0, double max_dt = 0.1)
    {
        if (paused_)
        {
            delta_seconds_ = 0.0;
            return delta_seconds_;
        }

        auto now = clock::now();
        delta_seconds_ = std::chrono::duration<double>(now - last_frame_time_).count();
        last_frame_time_ = now;

        if (delta_seconds_ < min_dt)
            delta_seconds_ = min_dt;
        else if (delta_seconds_ > max_dt)
            delta_seconds_ = max_dt;

        total_seconds_ = std::chrono::duration<double>(now - start_time_).count();
        return delta_seconds_;
    }

    double getDeltaSeconds() const { return delta_seconds_; }
    double getTotalSeconds() const { return total_seconds_; }

private:
    clock::time_point start_time_;
    clock::time_point last_frame_time_;
    double delta_seconds_;
    double total_seconds_;
    bool paused_;
};

} // namespace core
