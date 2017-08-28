#include "timer.h"
#include <stddef.h>

namespace iot
{
    Timer::Timer()
    {
#ifdef _WIN32
        QueryPerformanceFrequency(&m_frequency);
#endif
        StartCountdownMs(0);
    }

    Timer::Timer(int milliSeconds)
    {
#ifdef _WIN32
        QueryPerformanceFrequency(&m_frequency);
#endif
        StartCountdownMs(milliSeconds);
    }

    void Timer::StartCountdownMs(int milliSeconds)
    {
#ifdef _WIN32
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);

        LARGE_INTEGER interval;
        interval.QuadPart = (m_frequency.QuadPart * milliSeconds) / 1000;

        m_end.QuadPart = now.QuadPart + interval.QuadPart;
#else
        struct timeval now;
        gettimeofday(&now, NULL);

        struct timeval interval = { milliSeconds / 1000, (milliSeconds % 1000) * 1000 };

        timeradd(&now, &interval, &m_end);
#endif
    }

    void Timer::StartCountdown(int seconds)
    {
        StartCountdownMs(seconds * 1000);
    }

    int Timer::GetLeftMilliseconds() const
    {
        return static_cast<int>(GetLeftMicroseconds() / 1000);
    }

    long long Timer::GetLeftMicroseconds() const
    {
#ifdef _WIN32
        LARGE_INTEGER now, res;
        QueryPerformanceCounter(&now);

        res.QuadPart = m_end.QuadPart - now.QuadPart;
        return res.QuadPart < 0 ? 0 : (res.QuadPart * 1000000) / m_frequency.QuadPart;
#else
        struct timeval now, res;
        gettimeofday(&now, NULL);

        timersub(&m_end, &now, &res);
        return (res.tv_sec < 0) ? 0 : static_cast<long long>(res.tv_sec) * 1000000 + res.tv_usec;
#endif
    }

    bool Timer::IsExpired() const
    {
        return GetLeftMicroseconds() <= 0;
    }
}
