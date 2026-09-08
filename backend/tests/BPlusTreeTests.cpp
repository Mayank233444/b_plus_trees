#include <cassert>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "BPlusTree.hpp"

namespace {
struct CheckResult { long long first; long long last; int leafDepth; std::vector<long long> keys; };

CheckResult checkNode(const json& nodes, const std::string& id, int depth, bool root, long long order, long long leafCapacity) {
    const json& node = nodes.at(id);
    const std::vector<long long> keys = node.at("keys").get<std::vector<long long> >();
    for (std::size_t i = 1; i < keys.size(); ++i) assert(keys[i - 1] < keys[i]);
    if (node.at("isLeaf").get<bool>()) {
        assert(root || static_cast<long long>(keys.size()) >= (leafCapacity + 1) / 2);
        assert(static_cast<long long>(keys.size()) <= leafCapacity);
        return {keys.front(), keys.back(), depth, keys};
    }
    const std::vector<std::string> children = node.at("children").get<std::vector<std::string> >();
    assert(children.size() == keys.size() + 1);
    assert(static_cast<long long>(children.size()) <= order);
    assert(root || static_cast<long long>(children.size()) >= (order + 1) / 2);
    CheckResult result = checkNode(nodes, children.front(), depth + 1, false, order, leafCapacity);
    for (std::size_t i = 1; i < children.size(); ++i) {
        CheckResult next = checkNode(nodes, children[i], depth + 1, false, order, leafCapacity);
        assert(keys[i - 1] == next.first);
        assert(result.last < next.first);
        assert(result.leafDepth == next.leafDepth);
        result.last = next.last;
        result.keys.insert(result.keys.end(), next.keys.begin(), next.keys.end());
    }
    return result;
}

void checkTree(const BPlusTree& tree, const std::set<long long>& expected) {
    const json state = tree.toJson();
    if (expected.empty()) { assert(state.at("rootId").get<std::string>().empty()); return; }
    CheckResult actual = checkNode(state.at("nodes"), state.at("rootId").get<std::string>(), 0, true, tree.order(), tree.leafCapacity());
    assert(std::vector<long long>(expected.begin(), expected.end()) == actual.keys);
    for (long long key : expected) assert(tree.search(key));
}
}

int main() {
    for (long long order = 3; order <= 6; ++order) {
        for (long long capacity = 2; capacity <= 5; ++capacity) {
            BPlusTree tree(order, capacity);
            std::set<long long> expected;
            for (long long key = 1; key <= 48; ++key) { assert(tree.insert(key)); expected.insert(key); checkTree(tree, expected); }
            for (long long key = 2; key <= 48; key += 2) { assert(tree.erase(key)); expected.erase(key); checkTree(tree, expected); }
            for (long long key = 47; key >= 1; key -= 2) { assert(tree.erase(key)); expected.erase(key); checkTree(tree, expected); }
        }
    }
    std::cout << "B+ tree invariants passed" << std::endl;
}
