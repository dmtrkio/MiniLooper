#include "audio_parameter.h"

#include <exception>
#include <iostream>
#include <stdexcept>

namespace dsp::parameter {
    Parameter Parameter::makeFloat(const std::string& name, const float defaultValue, const Range<float> range)
    {
        if (!range.contains(defaultValue))
            throw std::runtime_error("defaultValue out of range");
        return Parameter(name, FloatData{RelaxedAtomic(defaultValue), defaultValue, range});
    }

    Parameter Parameter::makeInteger(const std::string& name, const std::int32_t defaultValue, const Range<std::int32_t> range)
    {
        if (!range.contains(defaultValue))
            throw std::runtime_error("defaultValue out of range");
        return Parameter(name, IntegerData{RelaxedAtomic(defaultValue), defaultValue, range});
    }

    Parameter Parameter::makeBoolean(const std::string& name, const bool defaultValue)
    {
        return Parameter(name, BooleanData{RelaxedAtomic(defaultValue), defaultValue});
    }

    const std::string& Parameter::getName() const noexcept { return name_; }

    ParameterType Parameter::getType() const noexcept
    {
        return std::visit([](const auto &p) { return p.type; }, data_);
    }

    void Parameter::setToDefault() noexcept
    {
        std::visit([](auto &p) { p.value.store(p.defaultValue); }, data_);
    }

    void Parameter::reset() noexcept { setToDefault(); }

    json Parameter::toJson() const
    {
        return std::visit([&](const auto& p) -> json {
            using T = std::decay_t<decltype(p)>;

            json j;
            j["name"] = name_;
            j["type"] = typeToString(p.type);
            j["value"] = p.value.load();
            j["defaultValue"] = p.defaultValue;

            if constexpr (std::is_same_v<T, FloatData> || std::is_same_v<T, IntegerData>) {
                j["range"] = { {"min", p.range.min}, {"max", p.range.max} };
            }

            return j;
        }, data_);
    }
    
    bool Parameter::trySetFromJson(const json& j) noexcept
    {
        try {
            if (j.at("name").get<std::string>() != name_ ||
                j.at("type").get<std::string>() != typeToString(getType())
            ) return false;

            const auto value = j.at("value");

            std::visit([&](auto &p) -> bool {
                using ValueType = std::decay_t<decltype(p.value.load())>;
                return set(value.get<ValueType>());
            }, data_);

            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to set parameter from JSON: " << e.what() << std::endl;
            return false;
        }
    }
}