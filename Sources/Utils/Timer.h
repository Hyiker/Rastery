#pragma once
#include <chrono>
#include <cstdint>

namespace Rastery {
/** Simple Timer for high precision timing.
 *
 */
enum class TimeUnit { Seconds, MilliSeconds, MicroSeconds, NanoSeconds };

class Timer {
   public:
    Timer(bool preventBegin = false);

    void begin();
    void end();

    int64_t elapsedTime(TimeUnit unit = TimeUnit::MilliSeconds) const;

    float elapsedMilliseconds() const;

   private:
    std::chrono::time_point<std::chrono::high_resolution_clock> mBegin, mEnd;
};
}  // namespace Rastery
