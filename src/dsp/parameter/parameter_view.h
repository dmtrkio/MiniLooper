#pragma once

#include <concepts>

#include "dsp/parameter/audio_parameter.h"

namespace dsp::parameter {
    template <typename T>
    concept ParameterViewType = std::same_as<T, float> ||
                                std::same_as<T, std::int32_t> ||
                                std::same_as<T, bool>;

    template <ParameterViewType T>
    class ParameterView
    {
    public:
        ParameterView();
        explicit ParameterView(Parameter& param);

        [[nodiscard]] T get() const noexcept;
        bool set(T v) noexcept;

        void referTo(Parameter& param) noexcept;

    private:
        Parameter* param_;
    };

    template <ParameterViewType T>
    ParameterView<T>::ParameterView() : param_(nullptr) {}

    template <ParameterViewType T>
    ParameterView<T>::ParameterView(Parameter& param) : param_(&param) {}

    template <ParameterViewType T>
    T ParameterView<T>::get() const noexcept
    {
        if (param_) {
            return param_->get<T>();
        } else {
            return T{};
        }
    }

    template <ParameterViewType T>
    bool ParameterView<T>::set(T v) noexcept
    {
        if (param_) {
            return param_->set(v);
        } else {
            return false;
        }
    }

    template <ParameterViewType T>
    void ParameterView<T>::referTo(Parameter& param) noexcept
    {
        param_ = &param;
    }

    using IntegerParameterView = ParameterView<std::int32_t>;
    using FloatParameterView = ParameterView<float>;
    using BooleanParameterView = ParameterView<bool>;
}