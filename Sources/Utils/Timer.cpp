#include "Timer.h"

#include "Core/Error.h"

namespace Rastery {
Timer::Timer(bool preventBegin) {
    if (!preventBegin) {
        begin();
    }
}

void Timer::begin() { mBegin = std::chrono::high_resolution_clock::now(); }

void Timer::end() { mEnd = std::chrono::high_resolution_clock::now(); }

int64_t Timer::elapsedTime(TimeUnit unit) const {
    auto duration = mEnd - mBegin;
    switch (unit) {
        case TimeUnit::Seconds:
            return std::chrono::duration_cast<std::chrono::seconds>(duration).count();

        case TimeUnit::MilliSeconds:
            return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        case TimeUnit::MicroSeconds:
            return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();

        case TimeUnit::NanoSeconds:
            return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    }
    RASTERY_UNREACHABLE();
}

float Timer::elapsedMilliseconds() const { return float(elapsedTime(TimeUnit::MicroSeconds)) / 1000.f; }

}  // namespace Rastery