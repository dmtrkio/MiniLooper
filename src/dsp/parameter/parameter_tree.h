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

namespace ml::dsp::parameter {
    class ParameterTree
    {
    public:
        // creates an empty tree with a given name
        explicit ParameterTree(std::string name);
        // wraps parameter in a parameter tree
        explicit ParameterTree(Parameter param);
        // creates a tree with parameter child nodes creates from parameters vector
        ParameterTree(std::string name, std::vector<Parameter> parameters);
        // creates a tree with given subtrees
        ParameterTree(std::string name, std::vector<ParameterTree> children);

        ParameterTree(const ParameterTree&) = default;
        ParameterTree& operator=(const ParameterTree&) = default;
        ParameterTree(ParameterTree&&) noexcept = default;
        ParameterTree& operator=(ParameterTree&&) noexcept = default;

        [[nodiscard]] const std::string& getName() const;
        void setName(std::string_view newName);

        // refers to a valid node (Parameter or a tree)
        [[nodiscard]] bool isValid() const noexcept;
        // refers to a tree
        [[nodiscard]] bool isSubTree() const;
        // refers to a parameter
        [[nodiscard]] bool isParameter() const;

        [[nodiscard]] Parameter& asParameterUnsafe();
        [[nodiscard]] const Parameter& asParameterUnsafe() const;

        [[nodiscard]] std::optional<std::reference_wrapper<Parameter>> asParameter() noexcept;
        [[nodiscard]] std::optional<std::reference_wrapper<Parameter>> getParameter(const std::string& name) noexcept;

        [[nodiscard]] ParameterTree operator[](std::string_view key) const noexcept;

        template <typename Fn>
        requires std::invocable<Fn, ParameterTree&>
        void forEachChild(Fn&& fn);

        template <typename Fn>
        requires std::invocable<Fn, const ParameterTree&>
        void forEachChild(Fn&& fn) const;

        ParameterTree addSubTree(ParameterTree subtree);
        ParameterTree addParameter(Parameter&& parameter);
        void addParameters(std::vector<Parameter> parameters);

        [[nodiscard]] bool operator==(const ParameterTree& other) const noexcept;
        [[nodiscard]] bool operator!=(const ParameterTree& other) const noexcept;

        [[nodiscard]] json toJson() const;
        bool copyParameterValuesFromJson(const json& j) noexcept;

    private:
        static void throwDuplicateNameException();
        static void throwInvalidTreeException();

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

    [[nodiscard]] inline bool ParameterTree::isValid() const noexcept
    {
        return node_ != nullptr;
    }

    [[nodiscard]] inline bool ParameterTree::isSubTree() const 
    {
        return isValid() && std::holds_alternative<Vec>(node_->data);
    }

    [[nodiscard]] inline bool ParameterTree::isParameter() const
    {
        return isValid() && std::holds_alternative<Parameter>(node_->data);
    }

    template <typename Fn>
    requires std::invocable<Fn, ParameterTree&>
    inline void ParameterTree::forEachChild(Fn&& fn)
    {
        if (!isValid()) throwInvalidTreeException();

        if (!isSubTree()) return;

        for (auto& child : std::get<Vec>(node_->data)) {
            fn(child);
        }
    }

    template <typename Fn>
    requires std::invocable<Fn, const ParameterTree&>
    inline void ParameterTree::forEachChild(Fn&& fn) const
    {
        if (!isValid()) throwInvalidTreeException();

        if (!isSubTree()) return;

        for (const auto& child : std::get<Vec>(node_->data)) {
            fn(child);
        }
    }
}