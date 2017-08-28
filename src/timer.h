#pragma once

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

namespace iot
{
    class Timer
    {
    public:
        Timer();
        Timer(int milliSeconds);
        void StartCountdownMs(int milliSeconds);
        void StartCountdown(int seconds);
        int GetLeftMilliseconds() const;
        long long GetLeftMicroseconds() const;
        bool IsExpired() const;

    private:
#ifdef _WIN32
        LARGE_INTEGER m_end;
        LARGE_INTEGER m_frequency;
#else
        struct timeval m_end;
#endif
    };
}
