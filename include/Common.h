//
// Created by liuzikai on 2/20/21.
//

#ifndef META_VISION_SOLAIS_COMMON_H
#define META_VISION_SOLAIS_COMMON_H

namespace meta {

template<typename T>
struct Range {
    T min;  // inclusive
    T max;  // inclusive
    bool contains(const T &value) const { return min <= value && value <= max; }
};

}

#endif //META_VISION_SOLAIS_COMMON_H
