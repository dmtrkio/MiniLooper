#include "parameter_tree.h"

#include <algorithm>
#include <utility>

namespace ml::dsp::parameter {
    ParameterTree::ParameterTree()
        : node_(nullptr) {}

    ParameterTree::ParameterTree(std::string name)
        : node_(std::make_shared<Node>(std::move(name), Vec{})) {}

    ParameterTree::ParameterTree(Parameter param)
        : node_(std::make_shared<Node>(std::move(param))) {}

    ParameterTree::ParameterTree(std::string name, std::vector<Parameter> parameters)
        : ParameterTree(std::move(name))
    {
        addParameters(std::move(parameters));
    }

    ParameterTree::ParameterTree(std::string name, std::vector<ParameterTree> children)
        : ParameterTree(std::move(name))
    {
        for (auto& subtree : children) {
            addSubTree(std::move(subtree));
        }
    }

    const std::string& ParameterTree::getName() const
    {
        if (!isValid()) throwInvalidTreeException();
        if (isParameter())
            return std::get<Parameter>(node_->data).getName();
        return node_->name;
    }

    Parameter& ParameterTree::asParameterUnsafe()
    {
        if (!isParameter()) throw std::runtime_error{"Not a parameter node"};
        return std::get<Parameter>(node_->data);
    }

    const Parameter& ParameterTree::asParameterUnsafe() const
    {
        if (!isParameter()) throw std::runtime_error{"Not a parameter node"};
        return std::get<Parameter>(node_->data);
    }

    std::optional<std::reference_wrapper<Parameter>> ParameterTree::asParameter() noexcept
    {
        if (!isParameter()) return std::nullopt;
        return std::get<Parameter>(node_->data);
    }

    std::optional<std::reference_wrapper<Parameter>> ParameterTree::getParameter(const std::string& name) noexcept
    {
        auto tree = (*this)[name];
        if (!tree.isValid()) return std::nullopt;
        return tree.asParameter();
    }

    ParameterTree ParameterTree::operator[](std::string_view key) const noexcept
    {
        if (!isSubTree()) return ParameterTree{};

        const auto& children = std::get<Vec>(node_->data);
        const auto it = std::ranges::find_if(children, [&](const ParameterTree& node) {
            try {
                return node.getName() == key;
            } catch (...) {
                assert(false && "should never run");
                return false;
            }
        });

        return (it != children.end()) ? *it : ParameterTree{};
    }

    ParameterTree ParameterTree::addSubTree(ParameterTree subtree)
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

    ParameterTree ParameterTree::addParameter(Parameter&& parameter)
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

    void ParameterTree::addParameters(std::vector<Parameter> parameters)
    {
        if (!isSubTree()) return;

        for (auto& parameter : parameters) {
            addParameter(std::move(parameter));
        }
    }

    bool ParameterTree::operator==(const ParameterTree& other) const noexcept
    {
        return node_ == other.node_;
    }

    bool ParameterTree::operator!=(const ParameterTree& other) const noexcept
    {
        return node_ != other.node_;
    }

    json ParameterTree::toJson() const
    {
        if (!isValid()) return nullptr;

        if (isParameter()) {
            return asParameterUnsafe().toJson();
        }

        json j = json::object();
        forEachChild([&](const ParameterTree& child) {
            j[child.getName()] = child.toJson();
        });
        return j;
    }
    
    bool ParameterTree::copyParameterValuesFromJson(const json& j) noexcept
    {
        if (!isValid()) return false;

        if (isParameter()) {
            return asParameterUnsafe().trySetFromJson(j);
        }

        if (!j.is_object()) return false;

        bool success = true;
        for (const auto& [key, value] : j.items()) {
            auto child = (*this)[key];
            if (!child.copyParameterValuesFromJson(value)) {
                success = false;
            }
        }
        return success;
    }

    void ParameterTree::throwDuplicateNameException()
    {
        throw std::runtime_error{"Duplicate key: a parameter or subtree with the same name already exists in the current tree"};
    }

    void ParameterTree::throwInvalidTreeException()
    {
        throw std::runtime_error{"Parameter Tree is invalid"};
    }
}