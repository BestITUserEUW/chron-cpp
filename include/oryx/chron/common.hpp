#pragma once

#include <chrono>

#ifdef ORYX_CHRON_BUILD_SHARED_LIBS
    #ifdef _WIN32
        #ifdef chron_cpp_EXPORTS
            #define ORYX_CHRON_API __declspec(dllexport)
        #else
            #define ORYX_CHRON_API __declspec(dllimport)
        #endif
    #else
        #define ORYX_CHRON_API __attribute__((visibility("default")))
    #endif
#else
    #define ORYX_CHRON_API
#endif

namespace oryx::chron {

using Clock = std::chrono::system_clock;
using TimePoint = std::chrono::time_point<Clock>;
using Duration = Clock::duration;

}  // namespace oryx::chron