//
// Created by liuzikai on 2/20/21.
//

#ifndef META_VISION_SOLAIS_COMMON_H
#define META_VISION_SOLAIS_COMMON_H

#include <string>
#include <vector>

using std::string;
using std::stringstream;
using std::vector;

namespace meta {

template<typename T>
struct Range {
    T min;  // inclusive
    T max;  // inclusive
    bool contains(const T &value) const { return min <= value && value <= max; }
};

struct SharedParameters {
    int imageWidth = 1280;
    int imageHeight = 720;
};

}

#endif //META_VISION_SOLAIS_COMMON_H
