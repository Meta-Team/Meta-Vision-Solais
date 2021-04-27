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
using package::ToggledDouble;
using package::ToggledInt;

inline bool inRange(double value, const DoubleRange &range) {
    return range.min() <= value && value <= range.max();
}

inline bool inRange(double value, const ToggledDoubleRange &range) {
    return range.min() <= value && value <= range.max();
}

inline DoubleRange *allocDoubleRange(double min = 0, double max = 0) {
    auto ret = new DoubleRange;
    ret->set_min(min);
    ret->set_max(max);
    return ret;
}

inline ToggledDoubleRange *allocToggledDoubleRange(bool enabled = false, double min = 0, double max = 0) {
    auto ret = new ToggledDoubleRange;
    ret->set_enabled(enabled);
    ret->set_min(min);
    ret->set_max(max);
    return ret;
}

inline ToggledDouble *allocToggledDouble(bool enabled = false, double val = 0) {
    auto ret = new ToggledDouble;
    ret->set_enabled(enabled);
    ret->set_val(val);
    return ret;
}

inline ToggledInt *allocToggledInt(bool enabled = false, int val = 0) {
    auto ret = new ToggledInt;
    ret->set_enabled(enabled);
    ret->set_val(val);
    return ret;
}

}

#endif //META_VISION_SOLAIS_PARAMETERS_H
