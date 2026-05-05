#pragma once

#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "audio_parameter.h"
#include "json.h"

namespace dsp::parameter {
    class ParameterTree
    {
    public:
        explicit ParameterTree(std::string name);
        explicit ParameterTree(Parameter param);
        ParameterTree(std::string name, std::vector<Parameter> parameters);
        ParameterTree(std::string name, std::vector<ParameterTree> children);

        ParameterTree(const ParameterTree&) = default;
        ParameterTree& operator=(const ParameterTree&) = default;
        ParameterTree(ParameterTree&&) noexcept = default;
        ParameterTree& operator=(ParameterTree&&) noexcept = default;

        [[nodiscard]] bool isValid() const noexcept { return node_ != nullptr; }

        [[nodiscard]] const std::string& getName() const;

        [[nodiscard]] bool isSubTree() const { return isValid() && std::holds_alternative<Vec>(node_->data); }
        [[nodiscard]] bool isParameter() const { return isValid() && std::holds_alternative<Parameter>(node_->data); }

        [[nodiscard]] Parameter& asParameterUnsafe();
        [[nodiscard]] const Parameter& asParameterUnsafe() const;

        [[nodiscard]] std::optional<std::reference_wrapper<Parameter>> asParameter() noexcept;
        [[nodiscard]] std::optional<std::reference_wrapper<Parameter>> getParameter(const std::string& name);

        [[nodiscard]] ParameterTree operator[](std::string_view key) const;

        template <typename Fn>
        requires std::invocable<Fn, ParameterTree&>
        void forEachChild(Fn&& fn) const
        {
            if (!isSubTree()) return;

            for (auto& child : std::get<Vec>(node_->data)) {
                fn(child);
            }
        }

        template <typename Fn>
        requires std::invocable<Fn, Parameter&>
        void forEachParameter(Fn&& fn) const
        {
            if (isParameter()) {
                fn(asParameterUnsafe());
                return;
            }

            for (auto& child : std::get<Vec>(node_->data)) {
                child.forEachParameter(std::forward<Fn>(fn));
            }
        }

        ParameterTree addSubTree(ParameterTree subtree);
        ParameterTree addParameter(Parameter&& parameter);
        void addParameters(std::vector<Parameter> parameters);

        [[nodiscard]] bool operator==(const ParameterTree& other) const noexcept;
        [[nodiscard]] bool operator!=(const ParameterTree& other) const noexcept;

        [[nodiscard]] json toJson() const;

    private:
        void throwDuplicateNameException();

        using Vec = std::vector<ParameterTree>;
        using Variant = std::variant<Vec, Parameter>;

        struct Node
        {
            std::string name;
            Variant data;

            explicit Node(std::string n, Variant d)
                : name(std::move(n)), data(std::move(d)) {}

            explicit Node(Parameter param)
                : data(std::move(param)) {}
        };

        std::shared_ptr<Node> node_;

        ParameterTree();
    };

    ParameterTree testParameterTree();
}