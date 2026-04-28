#pragma once

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
        if (!isParameter()) throw std::bad_variant_access{};
        return std::get<Parameter>(node_->data);
    }

    [[nodiscard]] const Parameter& asParameterUnsafe() const
    {
        if (!isParameter()) throw std::bad_variant_access{};
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

    // Returns an invalid ParameterTree if not found (instead of raw pointer)
    [[nodiscard]] ParameterTree operator[](const std::string_view key) const
    {
        if (!isSubTree()) return ParameterTree{{}};

        const auto& children = std::get<Vec>(node_->data);
        const auto it = std::ranges::find_if(children, [&](const ParameterTree& node) {
            return node.getName() == key;
        });

        return (it != children.end()) ? *it : ParameterTree{{}};
    }

    template <typename Fn>
    requires std::invocable<Fn, ParameterTree&>
    void forEachChild(Fn&& fn)
    {
        if (!isSubTree()) return;

        for (auto& child : std::get<Vec>(node_->data)) {
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

        for (auto& child : std::get<Vec>(node_->data)) {
            child.forEachParameter(std::forward<Fn>(fn));
        }
    }

    // Adds a subtree by value (copies or moves into children vector)
    ParameterTree addSubTree(ParameterTree subtree)
    {
        if (!isSubTree() || !subtree.isValid()) return ParameterTree{{}};

        auto& children = std::get<Vec>(node_->data);

        const auto it = std::ranges::find_if(children, [&](const ParameterTree& node) {
            return node.getName() == subtree.getName();
        });

        if (it != children.end()) return ParameterTree{{}};

        children.push_back(std::move(subtree));
        return children.back();
    }

    ParameterTree addParameter(Parameter&& parameter)
    {
        if (!isSubTree()) return ParameterTree{{}};

        auto& children = std::get<Vec>(node_->data);

        const auto it = std::ranges::find_if(children, [&](const ParameterTree& node) {
            return node.getName() == parameter.getName();
        });

        if (it != children.end()) return ParameterTree{{}};

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

    // Equality compares identity (same underlying node), like ValueTree
    [[nodiscard]] bool operator==(const ParameterTree& other) const noexcept
    {
        return node_ == other.node_;
    }

    [[nodiscard]] bool operator!=(const ParameterTree& other) const noexcept
    {
        return node_ != other.node_;
    }

    [[nodiscard]] operator bool() const noexcept { return isValid(); }

private:
    using Vec = std::vector<ParameterTree>;
    using Variant = std::variant<Vec, Parameter>;

    struct Node
    {
        std::string name;           // Group name for Vec; empty for Parameter leaf
        Variant data;

        explicit Node(std::string n, Variant d)
            : name(std::move(n)), data(std::move(d)) {}

        explicit Node(Parameter param)
            : data(std::move(param)) {}
    };

    std::shared_ptr<Node> node_;

    // Private invalid-tree constructor
    explicit ParameterTree(std::nullptr_t) : node_(nullptr) {}
};

} // namespace dsp::parameter