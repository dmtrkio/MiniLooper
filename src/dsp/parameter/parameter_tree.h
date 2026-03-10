#pragma once

#include <map>
#include <variant>
#include <optional>
#include <ranges>
#include <concepts>

#include "audio_parameter.h"

namespace dsp::parameter {
    class ParameterTree
    {
    public:
        explicit ParameterTree(std::string name) : name_(std::move(name)), data_(Map{}) {}
        explicit ParameterTree(Parameter param) : name_(param.getName()), data_(param) {}

        [[nodiscard]] const std::string& getName() const { return name_; }

        [[nodiscard]] bool isSubTree() const { return std::holds_alternative<Map>(data_); }
        [[nodiscard]] bool isParameter() const { return std::holds_alternative<Parameter>(data_); }

        [[nodiscard]] Parameter& asParameterUnsafe()
        {
            if (!isParameter()) throw std::bad_variant_access{};
            return std::get<Parameter>(data_);
        }

        [[nodiscard]] const Parameter& asParameterUnsafe() const
        {
            if (!isParameter()) throw std::bad_variant_access{};
            return std::get<Parameter>(data_);
        }

        [[nodiscard]] std::optional<std::reference_wrapper<Parameter>> asParameter() noexcept
        {
            if (!isParameter()) return std::nullopt;
            return std::get<Parameter>(data_);
        }

        [[nodiscard]] std::optional<std::reference_wrapper<Parameter>> getParameter(const std::string& name)
        {
            auto *tree = this->operator[](name);
            if (!tree) return std::nullopt;
            return tree->asParameter();
        }

        [[nodiscard]] ParameterTree* operator[](const std::string& key)
        {
            if (!isSubTree()) return nullptr;
            auto& nested = std::get<Map>(data_);
            const auto it = nested.find(key);
            if (it == nested.end()) return nullptr;
            return &it->second;
        }

        [[nodiscard]] const ParameterTree* operator[](const std::string& key) const
        {
            if (!isSubTree()) return nullptr;
            const auto& nested = std::get<Map>(data_);
            const auto it = nested.find(key);
            if (it == nested.end()) return nullptr;
            return &it->second;
        }

        template <typename Fn>
        requires std::invocable<Fn, ParameterTree&>
        void forEach(Fn&& fn)
        {
            if (!isSubTree()) return;

            for (auto& value : std::get<Map>(data_) | std::views::values) {
                fn(value);
            }
        }

        template <typename Fn>
        requires std::invocable<Fn, Parameter&>
        void forEachParameter(Fn&& fn)
        {
            if (isParameter()) {
                fn(asParameterUnsafe());
                return;
            }

            for (auto& value : std::get<Map>(data_) | std::views::values) {
                value.forEachParameter(std::forward<Fn>(fn));
            }
        }

        ParameterTree* addSubTree(ParameterTree&& subtree)
        {
            if (!isSubTree()) return nullptr;
            auto [it, inserted] = std::get<Map>(data_).emplace(subtree.getName(), std::move(subtree));
            return &it->second;
        }

        Parameter* addParameter(Parameter&& parameter)
        {
            if (!isSubTree()) return nullptr;
            auto [it, inserted] = std::get<Map>(data_).emplace(parameter.getName(), std::move(parameter));
            return &it->second.asParameterUnsafe();
        }

    private:
        using Map = std::map<std::string, ParameterTree>;
        using Variant = std::variant<Map, Parameter>;

        std::string name_;
        Variant data_;
    };
}