#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

#include "dsp/parameter/parameter_tree.h"

using namespace ml::dsp::parameter;

inline Parameter makeParam(const std::string& name)
{
    return Parameter::makeFloat(name, 0.0f, {0.0f, 1.0f});
}

TEST_CASE("ParameterTree construction and validity", "[parametertree][construct]")
{
    SECTION("Name-only constructor creates an empty subtree") {
        ParameterTree tree("root");
        REQUIRE(tree.isValid());
        REQUIRE(tree.getName() == "root");
        REQUIRE(tree.isSubTree());
        REQUIRE_FALSE(tree.isParameter());
    }

    SECTION("Parameter constructor creates a parameter node") {
        ParameterTree tree(makeParam("gain"));
        REQUIRE(tree.isValid());
        REQUIRE(tree.isParameter());
        REQUIRE_FALSE(tree.isSubTree());
    }

    SECTION("Name + vector<Parameter> constructor") {
        std::vector<Parameter> params;
        params.push_back(makeParam("p1"));
        params.push_back(makeParam("p2"));

        ParameterTree tree("group", std::move(params));
        REQUIRE(tree.isValid());
        REQUIRE(tree.getName() == "group");
        REQUIRE(tree.isSubTree());
    }

    SECTION("Name + vector<ParameterTree> constructor") {
        std::vector<ParameterTree> children;
        children.emplace_back("child1");
        children.emplace_back("child2");

        ParameterTree tree("parent", std::move(children));
        REQUIRE(tree.isValid());
        REQUIRE(tree.getName() == "parent");
        REQUIRE(tree.isSubTree());
    }
}

TEST_CASE("ParameterTree copy and move operations", "[parametertree][lifetime]")
{
    ParameterTree original("original");
    original.addParameter(makeParam("param1"));

    SECTION("Copy construction preserves structure") {
        ParameterTree copy(original);
        REQUIRE(original.isValid());
        REQUIRE(copy.isValid());
        REQUIRE(copy.getName() == "original");
        REQUIRE(copy.getParameter("param1").has_value());
    }

    SECTION("Copy assignment") {
        ParameterTree copy("other");
        copy = original;
        REQUIRE(original.isValid());
        REQUIRE(copy.getName() == "original");
        REQUIRE(copy.getParameter("param1").has_value());
    }

    SECTION("Move construction transfers structure") {
        ParameterTree moved(std::move(original));
        REQUIRE_FALSE(original.isValid());
        REQUIRE(moved.isValid());
        REQUIRE(moved.getName() == "original");
        REQUIRE(moved.getParameter("param1").has_value());
    }

    SECTION("Move assignment") {
        ParameterTree moved("other");
        moved = std::move(original);
        REQUIRE_FALSE(original.isValid());
        REQUIRE(moved.getName() == "original");
        REQUIRE(moved.getParameter("param1").has_value());
    }
}

TEST_CASE("ParameterTree operator[]", "[parametertree][access]")
{
    std::vector<ParameterTree> children;
    children.emplace_back("child1");
    children.emplace_back("child2");
    ParameterTree tree("parent", std::move(children));

    SECTION("Access existing child by name") {
        ParameterTree found = tree["child1"];
        REQUIRE(found.isValid());
        REQUIRE(found.getName() == "child1");
    }

    SECTION("Access non-existing child returns invalid tree") {
        ParameterTree notfound = tree["nonexistent"];
        REQUIRE_FALSE(notfound.isValid());
    }
}

TEST_CASE("ParameterTree parameter accessors", "[parametertree][access]")
{
    SECTION("asParameterUnsafe on parameter node") {
        ParameterTree tree(makeParam("freq"));
        REQUIRE(tree.asParameterUnsafe().getName() == "freq");
    }

    SECTION("asParameter returns reference for parameter node") {
        ParameterTree tree(makeParam("freq"));
        auto opt = tree.asParameter();
        REQUIRE(opt.has_value());
        REQUIRE(opt->get().getName() == "freq");
    }

    SECTION("asParameter returns nullopt for subtree") {
        ParameterTree tree("subtree");
        auto opt = tree.asParameter();
        REQUIRE_FALSE(opt.has_value());
    }

    SECTION("getParameter finds immediate child parameter by name") {
        ParameterTree tree("group");
        tree.addParameter(makeParam("gain"));

        auto opt = tree.getParameter("gain");
        REQUIRE(opt.has_value());
        REQUIRE(opt->get().getName() == "gain");
    }

    SECTION("getParameter returns nullopt when name not found") {
        ParameterTree tree("group");
        auto opt = tree.getParameter("missing");
        REQUIRE_FALSE(opt.has_value());
    }
}

TEST_CASE("ParameterTree mutation", "[parametertree][mutate]")
{
    SECTION("addSubTree inserts a child subtree") {
        ParameterTree tree("root");
        auto added = tree.addSubTree(ParameterTree("child"));

        REQUIRE(added.isValid());
        REQUIRE(added.getName() == "child");
        REQUIRE(tree["child"].isValid());
    }

    SECTION("addParameter inserts a parameter child") {
        ParameterTree tree("root");
        auto added = tree.addParameter(makeParam("newparam"));

        REQUIRE(added.isValid());
        REQUIRE(added.isParameter());
        REQUIRE(tree.getParameter("newparam").has_value());
    }

    SECTION("addParameters inserts multiple parameters") {
        ParameterTree tree("root");
        std::vector<Parameter> params;
        params.push_back(makeParam("p1"));
        params.push_back(makeParam("p2"));

        tree.addParameters(std::move(params));

        REQUIRE(tree.getParameter("p1").has_value());
        REQUIRE(tree.getParameter("p2").has_value());
    }

    SECTION("Duplicate name throws exception") {
        ParameterTree tree("root");
        tree.addParameter(makeParam("dup"));
        REQUIRE_THROWS(tree.addParameter(makeParam("dup")));
        REQUIRE_THROWS(tree.addSubTree(ParameterTree("dup")));
    }
}

TEST_CASE("ParameterTree forEachChild", "[parametertree][iterate]")
{
    SECTION("Iterates immediate children only") {
        ParameterTree tree("root");
        tree.addSubTree(ParameterTree("c1"));
        tree.addSubTree(ParameterTree("c2"));

        int count = 0;
        tree.forEachChild([&count](ParameterTree& child) {
            ++count;
            REQUIRE(child.isValid());
        });
        REQUIRE(count == 2);
    }

    SECTION("No-op on parameter node") {
        ParameterTree tree(makeParam("p"));
        int count = 0;
        tree.forEachChild([&count](ParameterTree&) { ++count; });
        REQUIRE(count == 0);
    }
}