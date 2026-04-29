#pragma once

#include <stdexcept>
#include <variant>
#include <vector>
#include <optional>
#include <concepts>
#include <string_view>
#include <memory>

#include "audio_parameter.h"

namespace dsp::parameter {
    class ParameterTree
    {
    public:
        explicit ParameterTree(std::string name)
            : node_(std::make_shared<Node>(std::move(name), Vec{})) {}

        explicit ParameterTree(Parameter param)
            : node_(std::make_shared<Node>(std::move(param))) {}

        ParameterTree(std::string name, std::vector<Parameter> parameters)
            : ParameterTree(std::move(name))
        {
            addParameters(std::move(parameters));
        }

        ParameterTree(std::string name, std::vector<ParameterTree> parameters)
            : ParameterTree(std::move(name))
        {
            for (auto& subtree : parameters) {
                addSubTree(std::move(subtree));
            }
        }

        // Lightweight copy — shares the underlying node
        ParameterTree(const ParameterTree&) = default;
        ParameterTree& operator=(const ParameterTree&) = default;
        ParameterTree(ParameterTree&&) noexcept = default;
        ParameterTree& operator=(ParameterTree&&) noexcept = default;

        [[nodiscard]] bool isValid() const noexcept { return node_ != nullptr; }

        [[nodiscard]] const std::string& getName() const
        {
            if (!isValid()) throw std::runtime_error{"Invalid ParameterTree"};
            if (isParameter())
                return std::get<Parameter>(node_->data).getName();
            return node_->name;
        }

        [[nodiscard]] bool isSubTree() const { return isValid() && std::holds_alternative<Vec>(node_->data); }
        [[nodiscard]] bool isParameter() const { return isValid() && std::holds_alternative<Parameter>(node_->data); }

        [[nodiscard]] Parameter& asParameterUnsafe()
        {
            if (!isParameter()) throw std::runtime_error{"Not a parameter node"};
            return std::get<Parameter>(node_->data);
        }

        [[nodiscard]] const Parameter& asParameterUnsafe() const
        {
            if (!isParameter()) throw std::runtime_error{"Not a parameter node"};
            return std::get<Parameter>(node_->data);
        }

        [[nodiscard]] std::optional<std::reference_wrapper<Parameter>> asParameter() noexcept
        {
            if (!isParameter()) return std::nullopt;
            return std::get<Parameter>(node_->data);
        }

        [[nodiscard]] std::optional<std::reference_wrapper<Parameter>> getParameter(const std::string& name)
        {
            auto tree = (*this)[name];
            if (!tree.isValid()) return std::nullopt;
            return tree.asParameter();
        }

        [[nodiscard]] ParameterTree operator[](std::string_view key) const
        {
            if (!isSubTree()) return ParameterTree{};

            const auto& children = std::get<Vec>(node_->data);
            const auto it = std::ranges::find_if(children, [&](const ParameterTree& node) {
                return node.getName() == key;
            });

            return (it != children.end()) ? *it : ParameterTree{};
        }

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

        ParameterTree addSubTree(ParameterTree subtree)
        {
            if (!isSubTree() || !subtree.isValid()) return ParameterTree{};

            auto& children = std::get<Vec>(node_->data);

            const auto it = std::ranges::find_if(children, [&](const ParameterTree& node) {
                return node.getName() == subtree.getName();
            });

            if (it != children.end()) throwDuplicateNameException();

            children.push_back(std::move(subtree));
            return children.back();
        }

        ParameterTree addParameter(Parameter&& parameter)
        {
            if (!isSubTree()) return ParameterTree{};

            auto& children = std::get<Vec>(node_->data);

            const auto it = std::ranges::find_if(children, [&](const ParameterTree& node) {
                return node.getName() == parameter.getName();
            });

            if (it != children.end()) throwDuplicateNameException();

            children.emplace_back(std::move(parameter));
            return children.back();
        }

        void addParameters(std::vector<Parameter> parameters)
        {
            if (!isSubTree()) return;

            for (auto& parameter : parameters) {
                addParameter(std::move(parameter));
            }
        }

        [[nodiscard]] bool operator==(const ParameterTree& other) const noexcept
        {
            return node_ == other.node_;
        }

        [[nodiscard]] bool operator!=(const ParameterTree& other) const noexcept
        {
            return node_ != other.node_;
        }

        [[nodiscard]] nlohmann::ordered_json toJson() const
        {
            if (!isValid()) return nullptr;

            if (isParameter()) {
                return asParameterUnsafe().toJson();
            }

            nlohmann::ordered_json j = nlohmann::ordered_json::object();
            forEachChild([&](const ParameterTree& child) {
                j[child.getName()] = child.toJson();
            });
            return j;
        }

    private:
        void throwDuplicateNameException()
        {
            throw std::runtime_error{"Duplicate key: a parameter or subtree with the same name already exists in the current tree"};
        }

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

        // Private default invalid tree constructor
        ParameterTree() : node_(nullptr) {}
    };
} // namespace dsp::parameter