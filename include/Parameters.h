//
// Created by liuzikai on 4/26/21.
//

#ifndef META_VISION_SOLAIS_PARAMETERS_H
#define META_VISION_SOLAIS_PARAMETERS_H

#include "Parameters.pb.h"

namespace meta {

using package::ParamSet;
using package::Result;
using package::FloatRange;
using package::ToggledFloatRange;
using package::ToggledFloat;
using package::ToggledInt;
using package::IntPair;
using package::FloatPair;
using package::ResultPoint2f;
using package::ResultPoint3f;

inline bool inRange(float value, const FloatRange &range) {
    return range.min() <= value && value <= range.max();
}

inline bool inRange(float value, const ToggledFloatRange &range) {
    return range.min() <= value && value <= range.max();
}

inline FloatRange *allocFloatRange(float min = 0, float max = 0) {
    auto ret = new FloatRange;
    ret->set_min(min);
    ret->set_max(max);
    return ret;
}

inline ToggledFloatRange *allocToggledFloatRange(bool enabled = false, float min = 0, float max = 0) {
    auto ret = new ToggledFloatRange;
    ret->set_enabled(enabled);
    ret->set_min(min);
    ret->set_max(max);
    return ret;
}

inline ToggledFloat *allocToggledFloat(bool enabled = false, float val = 0) {
    auto ret = new ToggledFloat;
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

inline IntPair *allocIntPair(int x, int y) {
    auto ret = new IntPair;
    ret->set_x(x);
    ret->set_y(y);
    return ret;
}

inline FloatPair *allocFloatPair(float x, float y) {
    auto ret = new FloatPair;
    ret->set_x(x);
    ret->set_y(y);
    return ret;
}

inline ResultPoint2f *allocResultPoint2f(float x, float y) {
    auto ret = new ResultPoint2f;
    ret->set_x(x);
    ret->set_y(y);
    return ret;
}

inline ResultPoint3f *allocResultPoint3f(float x, float y, float z) {
    auto ret = new ResultPoint3f;
    ret->set_x(x);
    ret->set_y(y);
    ret->set_z(z);
    return ret;
}

}

#endif //META_VISION_SOLAIS_PARAMETERS_H
