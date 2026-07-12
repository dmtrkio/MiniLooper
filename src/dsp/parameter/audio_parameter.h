#pragma once

#include <string>
#include <string_view>
#include <variant>
#include <optional>
#include <cstdint>
#include <type_traits>

#include "threading/atomic_wrapper.h"
#include "dsp/dsp.h"
#include "json.h"

namespace ml::dsp::parameter {
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

    class Parameter
    {
    public:
        static Parameter makeFloat(const std::string& name, const float defaultValue, const Range<float> range);
        static Parameter makeInteger(const std::string& name, const std::int32_t defaultValue, const Range<std::int32_t> range);
        static Parameter makeBoolean(const std::string& name, const bool defaultValue);

        [[nodiscard]] const std::string& getName() const noexcept;
        void setName(std::string_view newName);

        [[nodiscard]] ParameterType getType() const noexcept;

        template<typename T>
        [[nodiscard]] T get() const noexcept;

        template<typename T>
        [[nodiscard]] T getDefault() const noexcept;

        template<typename T>
        [[nodiscard]] std::optional<Range<T>> getRange() const noexcept;

        template<typename T>
        bool set(T value) noexcept;

        void setToDefault() noexcept;
        void reset() noexcept;

        [[nodiscard]] json toJson() const;
        [[nodiscard]] bool trySetFromJson(const json& j) noexcept;

    private:
        template<typename T>
        Parameter(std::string name, T&& data)
            : name_(std::move(name)), data_(std::forward<T>(data))
        {}

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

    template<typename T>
    T Parameter::get() const noexcept
    {
        return std::visit([](const auto &p) -> T {
            return static_cast<T>(p.value.load());
        }, data_);
    }

    template<typename T>
    T Parameter::getDefault() const noexcept
    {
        return std::visit([](const auto &p) -> T {
            return static_cast<T>(p.defaultValue);
        }, data_);
    }

    template<typename T>
    std::optional<Range<T>> Parameter::getRange() const noexcept
    {
        return std::visit([](const auto &p) -> std::optional<Range<T>> {
            using PType = std::decay_t<decltype(p)>;
            if constexpr (
                (std::is_same_v<PType, FloatData> || std::is_same_v<PType, IntegerData>)
                && std::is_same_v<decltype(p.defaultValue), T>) {
                return p.range;
            }

            return std::nullopt;
        }, data_);
    }

    template<typename T>
    bool Parameter::set(T value) noexcept
    {
        return std::visit([&](auto &p) -> bool {
            using PType = std::decay_t<decltype(p)>;
            const auto v = static_cast<decltype(p.defaultValue)>(value);
            if constexpr (std::is_same_v<PType, FloatData> || std::is_same_v<PType, IntegerData>) {
                if (!p.range.contains(v)) return false;
                p.value.store(p.range.clamp(v));
            } else if constexpr (std::is_same_v<PType, BooleanData>) {
                p.value.store(v);
            }
            return true;
        }, data_);
    }
}