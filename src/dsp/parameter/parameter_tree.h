#pragma once

#include <variant>
#include <vector>
#include <optional>
#include <concepts>
#include <string_view>

#include "audio_parameter.h"

namespace dsp::parameter {
    class ParameterTree
    {
    public:
        explicit ParameterTree(std::string name)
            : name_(std::move(name)) {}

        explicit ParameterTree(Parameter param)
            : data_(std::move(param)) {}

        ParameterTree(std::string name, std::vector<Parameter> parameters)
            : ParameterTree(std::move(name))
        {
            addParameters(std::move(parameters));
        }

        [[nodiscard]] const std::string& getName() const
        {
            if (isParameter())
                return std::get<Parameter>(data_).getName();
            return name_;
        }

        [[nodiscard]] bool isSubTree() const { return std::holds_alternative<Vec>(data_); }
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
            auto* tree = (*this)[name];
            if (!tree) return std::nullopt;
            return tree->asParameter();
        }

        [[nodiscard]] const ParameterTree* operator[](const std::string_view key) const
        {
            if (!isSubTree()) return nullptr;

            const auto& children = std::get<Vec>(data_);
            const auto it = std::ranges::find_if(children, [&](const ParameterTree& node) {
                return node.getName() == key;
            });

            return (it != children.end()) ? &(*it) : nullptr;
        }

        [[nodiscard]] ParameterTree* operator[](const std::string_view key)
        {
            return const_cast<ParameterTree*>(
                std::as_const(*this)[key]
            );
        }

        template <typename Fn>
        requires std::invocable<Fn, ParameterTree&>
        void forEachChild(Fn&& fn)
        {
            if (!isSubTree()) return;

            for (auto& child : std::get<Vec>(data_)) {
                fn(child);
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

            for (auto& child : std::get<Vec>(data_)) {
                child.forEachParameter(std::forward<Fn>(fn));
            }
        }

        ParameterTree* addSubTree(ParameterTree&& subtree)
        {
            if (!isSubTree()) return nullptr;

            auto& children = std::get<Vec>(data_);

            const auto it = std::ranges::find_if(children, [&](const ParameterTree& node) {
                return node.getName() == subtree.getName();
            });

            if (it != children.end()) return nullptr;

            children.emplace_back(std::move(subtree));
            return &children.back();
        }

        Parameter* addParameter(Parameter&& parameter)
        {
            if (!isSubTree()) return nullptr;

            auto& children = std::get<Vec>(data_);

            const auto it = std::ranges::find_if(children, [&](const ParameterTree& node) {
                return node.getName() == parameter.getName();
            });

            if (it != children.end()) return nullptr;

            children.emplace_back(std::move(parameter));
            return &children.back().asParameterUnsafe();
        }

        void addParameters(std::vector<Parameter> parameters)
        {
            if (!isSubTree()) return;

            for (auto& parameter : parameters) {
                addParameter(std::move(parameter));
            }
        }

    private:
        using Vec = std::vector<ParameterTree>;
        using Variant = std::variant<Vec, Parameter>;

        std::string name_{};
        Variant data_{Vec{}};
    };
}