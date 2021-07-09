//
// Created by liuzikai on 6/27/21.
//

#ifndef META_VISION_SOLAIS_COMMON_H
#define META_VISION_SOLAIS_COMMON_H

#include <sstream>
#include <iomanip>

namespace meta {

using TimePoint = unsigned;  // 0.1ms

inline std::string currentTimeString() {
    // Reference: https://stackoverflow.com/questions/24686846/get-current-time-in-milliseconds-or-hhmmssmmm-format

    using namespace std::chrono;

    // Get current time
    auto now = system_clock::now();

    // Get number of milliseconds for the current second
    // (remainder after division into seconds)
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    // Convert to std::time_t in order to convert to std::tm (broken time)
    auto timer = system_clock::to_time_t(now);

    // Convert to broken time
    std::tm bt = *std::localtime(&timer);

    std::ostringstream oss;
    oss << std::put_time(&bt, "%Y%m%d%H%M%S");

    return oss.str();
}

}

#endif //META_VISION_SOLAIS_COMMON_H
