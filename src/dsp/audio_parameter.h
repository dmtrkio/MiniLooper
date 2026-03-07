#pragma once

#include <algorithm>
#include <atomic_wrapper.h>
#include <memory>
#include <string>
#include <variant>

namespace dsp::parameter {
    enum class ParameterType
    {
        Float,
        Integer,
        Boolean,
    };

    constexpr const char* typeToString(const ParameterType type)
    {
        switch (type) {
            case ParameterType::Float: return "Float";
            case ParameterType::Integer: return "Integer";
            case ParameterType::Boolean: return "Boolean";
            default: return "Invalid parameter type";
        }
    }

    template<typename T>
    struct Range
    {
        T min; // inclusive
        T max; // inclusive

        T clamp(T value) const { return std::clamp(value, min, max); }
        bool contains(T value) const { return value >= min && value <= max; }
    };

    class Parameter
    {
    public:
        static Parameter makeFloat(const std::string& name, const float defaultValue, const Range<float> range)
        {
            if (!range.contains(defaultValue))
                throw std::exception("defaultValue out of range");
            return Parameter(name, FloatData{RelaxedAtomic(defaultValue), defaultValue, range});
        }

        static Parameter makeInteger(const std::string& name, const std::int32_t defaultValue, const Range<std::int32_t> range)
        {
            if (!range.contains(defaultValue))
                throw std::exception("defaultValue out of range");
            return Parameter(name, IntegerData{RelaxedAtomic(defaultValue), defaultValue, range});
        }

        static Parameter makeBoolean(const std::string& name, const bool defaultValue)
        {
            return Parameter(name, BooleanData{RelaxedAtomic(defaultValue), defaultValue});
        }

        [[nodiscard]] const std::string& getName() const noexcept { return name_; }

        [[nodiscard]] ParameterType getType() const noexcept
        {
            return std::visit([](const auto &p) { return p.type; }, data_);
        }

        template<typename T>
        [[nodiscard]] T get() const noexcept
        {
            return std::visit([](const auto &p) -> T {
                return static_cast<T>(p.value.load());
            }, data_);
        }

        template<typename T>
        [[nodiscard]] T getDefault() const noexcept
        {
            return std::visit([](const auto &p) -> T {
                return static_cast<T>(p.defaultValue);
            }, data_);
        }

        template<typename T>
        void set(T value) noexcept
        {
            std::visit([&](const auto &p) {
                const auto v = static_cast<decltype(p.defaultValue)>(value);
                if constexpr (std::is_same_v<T, FloatData>) {
                    p.value.store(p.range.clamp(v));
                } else if constexpr (std::is_same_v<T, IntegerData>) {
                    p.value.store(p.range.clamp(v));
                } else if constexpr (std::is_same_v<T, BooleanData>) {
                    p.value.store(v);
                }
            }, data_);
        }

        void setToDefault() noexcept
        {
            std::visit([](auto &p) { p.value.store(p.defaultValue); }, data_);
        }

        void reset() noexcept { setToDefault(); }

    private:
        template<typename T>
        Parameter(std::string name, T&& data)
            : name_(std::move(name)), data_(std::forward<T>(data)) {}

        struct FloatData
        {
            static constexpr auto type = ParameterType::Float;
            RelaxedAtomic<float> value;
            float defaultValue;
            Range<float> range;
        };

        struct IntegerData
        {
            static constexpr auto type = ParameterType::Integer;
            RelaxedAtomic<std::int32_t> value;
            std::int32_t defaultValue;
            Range<std::int32_t> range;
        };

        struct BooleanData
        {
            static constexpr auto type = ParameterType::Boolean;
            RelaxedAtomic<bool> value;
            bool defaultValue;
        };

        using ParameterVariant = std::variant<FloatData, IntegerData, BooleanData>;

        std::string name_;
        ParameterVariant data_;
    };
}