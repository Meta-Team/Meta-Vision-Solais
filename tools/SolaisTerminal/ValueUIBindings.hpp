//
// Created by liuzikai on 2/20/21.
//

#ifndef META_VISION_SOLAIS_VALUEUIBINDINGS_HPP
#define META_VISION_SOLAIS_VALUEUIBINDINGS_HPP

#include "Common.h"
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QRadioButton>

namespace meta {

class ValueUIBinding {
public:
    virtual void param2UI() = 0;

    virtual void ui2Params() = 0;

    virtual ~ValueUIBinding() {}
};

template<typename T = double>
class ValueCheckSpinBinding : public ValueUIBinding {
public:
    /**
     * Create an Value-to-Check-Spin binding.
     * @param value   Required. Convention: 0 for not enabled.
     * @param check   Optional.
     * @param spinBox Required.
     */
    ValueCheckSpinBinding(T *value, QAbstractButton *check, QDoubleSpinBox *spinBox)
            : value(value), check(check), spinBox(spinBox) {
        assert(value != nullptr && "value is required");
        assert(spinBox != nullptr && "spinBox is required");
    }

    void param2UI() override {
        if (check != nullptr) check->setChecked(*value != 0);
        spinBox->setValue(*value);
    }

    void ui2Params() override {
        *value = (check == nullptr || check->isChecked()) ? spinBox->value() : 0;
    }

private:
    T *value;
    QAbstractButton *check;
    QDoubleSpinBox *spinBox;
};

template<typename T = double>
class RangeCheckSpinBinding : public ValueUIBinding {
public:
    /**
     * Create an Range-Check-Spin binding.
     * @param enabled    Optional. If check is not nullptr, enabled must not be nullptr.
     * @param range      Required.
     * @param check      Optional. If enabled is not nullptr, check must not be nullptr.
     * @param minSpinBox Required.
     * @param maxSpinBox Required.
     */
    RangeCheckSpinBinding(bool *enabled, Range<T> *range, QAbstractButton *check,
                          QDoubleSpinBox *minSpinBox, QDoubleSpinBox *maxSpinBox)
            : enabled(enabled), range(range), check(check), minSpinBox(minSpinBox), maxSpinBox(maxSpinBox) {
        assert((enabled == nullptr) == (check == nullptr) && "enabled and check must be both nullptr or both valid");
        assert(range != nullptr && "range is required");
        assert(minSpinBox != nullptr && "minSpinBox is required");
        assert(maxSpinBox != nullptr && "maxSpinBox is required");
    }

    void param2UI() override {
        if (check) check->setChecked(*enabled);
        minSpinBox->setValue(range->min);
        maxSpinBox->setValue(range->max);
    }

    void ui2Params() override {
        if (enabled) *enabled = check->isChecked();
        range->min = minSpinBox->value();
        range->max = maxSpinBox->value();
    }

private:
    bool *enabled;
    Range<T> *range;
    QAbstractButton *check;
    QDoubleSpinBox *minSpinBox;
    QDoubleSpinBox *maxSpinBox;
};

template<typename T>
class EnumRadioBinding : public ValueUIBinding {
public:
    EnumRadioBinding(T *value, const std::vector<std::pair<T, QRadioButton *>> &mapping) : value(value), mapping(mapping) {}

    void param2UI() override {
        for (auto &item : mapping) {
            if (*value == item.first) {
                item.second->setChecked(true);
                break;
            }
        }
    }

    void ui2Params() override {
        for (auto &item : mapping) {
            if (item.second->isChecked()) {
                *value = static_cast<T>(item.first);
                break;
            }
        }
    }

private:
    T *value;
    std::vector<std::pair<T, QRadioButton *>> mapping;
};

}

#endif //META_VISION_SOLAIS_VALUEUIBINDINGS_HPP
