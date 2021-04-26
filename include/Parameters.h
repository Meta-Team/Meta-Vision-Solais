//
// Created by liuzikai on 4/26/21.
//

#ifndef META_VISION_SOLAIS_PARAMETERS_H
#define META_VISION_SOLAIS_PARAMETERS_H

#include "Parameters.pb.h"

namespace meta {

using package::ParamSet;
using package::Result;
using package::DoubleRange;
using package::ToggledDoubleRange;

inline bool inRange(double value, const DoubleRange &range) {
    return range.min() <= value && value <= range.max();
}

inline bool inRange(double value, const ToggledDoubleRange &range) {
    return range.min() <= value && value <= range.max();
}

}

#endif //META_VISION_SOLAIS_PARAMETERS_H
