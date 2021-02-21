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

/**
 * A binding between a parameter and one or more UI widgets, providing bidirectional data transfer. Derived to several
 * common binding types.
 */
class ValueUIBinding {
public:

    /**
     * Set up UI widget(s) based on the parameter value.
     */
    virtual void param2UI() = 0;

    /**
     * Retrieve parameter value from UI widget(s).
     */
    virtual void ui2Params() = 0;

    virtual ~ValueUIBinding() = default;
};

/**
 * A binding between a bool type enabled + a type T value and an optional CheckBox/Radio + a DoubleSpinBox.
 * @tparam T  Type of the value
 */
template<typename T = double>
class ValueCheckSpinBinding : public ValueUIBinding {
public:

    /**
     * Create a Value-to-Check-Spin binding.
     * @param enabled Optional. If check is not nullptr, enabled must not be nullptr.
     * @param value   Required.
     * @param check   Optional. If enabled is not nullptr, check must not be nullptr.
     * @param spinBox Required.
     */
    ValueCheckSpinBinding(bool *enabled, T *value, QAbstractButton *check, QDoubleSpinBox *spinBox)
            : enabled(enabled), value(value), check(check), spinBox(spinBox) {
        assert((enabled == nullptr) == (check == nullptr) && "enabled and check must be both nullptr or both valid");
        assert(value != nullptr && "value is required");
        assert(spinBox != nullptr && "spinBox is required");
    }

    void param2UI() override {
        if (check != nullptr) check->setChecked(*enabled);
        spinBox->setValue(*value);
    }

    void ui2Params() override {
        if (enabled != nullptr) *enabled = check->isChecked();
        *value = spinBox->value();
    }

private:
    bool *enabled;
    T *value;
    QAbstractButton *check;
    QDoubleSpinBox *spinBox;
};

/**
 * A binding between a bool enabled value (optional) + a Range (required) and a CheckBox/Radio (optional) + a
 * DoubleSpinBox for min (required) + a DoubleSpinBox for max (required).
 * @tparam T  Type of the Range value
 */
template<typename T = double>
class RangeCheckSpinBinding : public ValueUIBinding {
public:

    /**
     * Create a Range-Check-Spin binding.
     * @param enabled    Optional. If check is not nullptr, enabled must not be nullptr.
     * @param range      Required.
     * @param check      Optional. If enabled is not nullptr, check must not be nullptr.
     * @param minSpinBox Required.
     * @param maxSpinBox Required.
     */
    RangeCheckSpinBinding(bool *enabled, Range <T> *range, QAbstractButton *check,
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
    Range <T> *range;
    QAbstractButton *check;
    QDoubleSpinBox *minSpinBox;
    QDoubleSpinBox *maxSpinBox;
};

/**
 * A binding between a enum T value and RadioButtons.
 * @tparam T  Type of the value
 */
template<typename T>
class EnumRadioBinding : public ValueUIBinding {
public:

    /**
     * Create a Value-to-Check-Spin binding.
     * @param value    Required.
     * @param mapping  Required. A vector of pair<Enum value, QRadioButton *> showing the mapping relationship.
     */
    EnumRadioBinding(T *value, const std::vector<std::pair<T, QRadioButton *>> &mapping)
            : value(value), mapping(mapping) {}

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
