#ifndef BPLUS_TREE_HPP
#define BPLUS_TREE_HPP

#include <algorithm>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "json.hpp"

using json = nlohmann::json;
using ll = long long;

class BPlusTree;

struct Snapshot {
    json state;
    std::string message;
    std::string highlightNodeId;
};

class SnapshotManager {
public:
    std::vector<Snapshot> snapshots;
    void takeSnapshot(const BPlusTree& tree, const std::string& message,
                      const std::string& highlightNodeId = "");
};

class BPlusTree {
public:
    explicit BPlusTree(ll order = 4, ll leafCapacity = 3) { reset(order, leafCapacity); }

    void reset(ll order, ll leafCapacity) {
        order_ = std::max(3LL, order);
        leafCapacity_ = std::max(2LL, leafCapacity);
        nodes_.clear(); root_ = nullptr; nextId_ = 1;
    }
    ll order() const { return order_; }
    ll leafCapacity() const { return leafCapacity_; }

    bool insert(ll key, SnapshotManager* snapshots = nullptr) {
        snapshot(snapshots, "Start inserting " + std::to_string(key));
        if (!root_) {
            root_ = makeNode(true); root_->keys.push_back(key);
            snapshot(snapshots, "Create the root leaf and place " + std::to_string(key), root_);
            return true;
        }
        Node* leaf = findLeaf(key, snapshots, "descend toward");
        if (std::binary_search(leaf->keys.begin(), leaf->keys.end(), key)) {
            snapshot(snapshots, std::to_string(key) + " already exists; duplicates are not inserted", leaf);
            return false;
        }
        leaf->keys.insert(std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key), key);
        snapshot(snapshots, "Insert " + std::to_string(key) + " in the target leaf", leaf);
        if (static_cast<ll>(leaf->keys.size()) > leafCapacity_) {
            snapshot(snapshots, "Leaf overflow: split the leaf", leaf);
            Node* right = splitLeaf(leaf);
            snapshot(snapshots, "Leaf split complete; copy " + std::to_string(right->keys.front()) + " upward", right);
            insertIntoParent(leaf, right, snapshots);
        } else refreshUp(leaf->parent);
        refreshSubtree(root_);
        snapshot(snapshots, "Insertion of " + std::to_string(key) + " is complete", leaf);
        return true;
    }

    bool erase(ll key, SnapshotManager* snapshots = nullptr) {
        snapshot(snapshots, "Start deleting " + std::to_string(key));
        if (!root_) { snapshot(snapshots, "The tree is empty; nothing to delete"); return false; }
        Node* leaf = findLeaf(key, snapshots, "look for");
        std::vector<ll>::iterator it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
        if (it == leaf->keys.end() || *it != key) {
            snapshot(snapshots, std::to_string(key) + " is not present", leaf); return false;
        }
        leaf->keys.erase(it);
        snapshot(snapshots, "Remove " + std::to_string(key) + " from the leaf", leaf);
        if (leaf == root_) {
            if (leaf->keys.empty()) { root_ = nullptr; snapshot(snapshots, "The final key was removed; the tree is empty"); }
            else snapshot(snapshots, "Deletion is complete", leaf);
            return true;
        }
        if (static_cast<ll>(leaf->keys.size()) < minLeafKeys()) rebalanceLeaf(leaf, snapshots);
        else refreshUp(leaf->parent);
        refreshSubtree(root_);
        snapshot(snapshots, "Deletion of " + std::to_string(key) + " is complete");
        return true;
    }

    bool search(ll key, SnapshotManager* snapshots = nullptr) const {
        snapshot(snapshots, "Start searching for " + std::to_string(key));
        if (!root_) { snapshot(snapshots, "The tree is empty; " + std::to_string(key) + " was not found"); return false; }
        Node* leaf = findLeaf(key, snapshots, "search for");
        const bool found = std::binary_search(leaf->keys.begin(), leaf->keys.end(), key);
        snapshot(snapshots, found ? std::to_string(key) + " was found in this leaf" : std::to_string(key) + " is not in this leaf", leaf);
        return found;
    }

    json toJson() const {
        json result = {{"rootId", root_ ? root_->id : ""}, {"order", order_},
                       {"leafCapacity", leafCapacity_}, {"nodes", json::object()}};
        if (!root_) return result;
        std::queue<Node*> pending; pending.push(root_);
        while (!pending.empty()) {
            Node* node = pending.front(); pending.pop();
            json children = json::array();
            if (!node->leaf) for (Node* child : node->children) { children.push_back(child->id); pending.push(child); }
            result["nodes"][node->id] = {{"id", node->id}, {"keys", node->keys}, {"isLeaf", node->leaf},
                                         {"children", children}, {"nextLeaf", node->leaf && node->next ? node->next->id : ""}};
        }
        return result;
    }

private:
    struct Node {
        std::string id; bool leaf; std::vector<ll> keys; std::vector<Node*> children;
        Node* parent = nullptr; Node* next = nullptr;
        Node(const std::string& id, bool leaf) : id(id), leaf(leaf) {}
    };
    ll order_ = 4, leafCapacity_ = 3, nextId_ = 1;
    Node* root_ = nullptr;
    std::vector<std::unique_ptr<Node> > nodes_;

    void snapshot(SnapshotManager* snapshots, const std::string& message, Node* node = nullptr) const {
        if (snapshots) snapshots->takeSnapshot(*this, message, node ? node->id : "");
    }
    Node* makeNode(bool leaf) {
        nodes_.emplace_back(new Node("node_" + std::to_string(nextId_++), leaf));
        return nodes_.back().get();
    }
    ll minLeafKeys() const { return (leafCapacity_ + 1) / 2; }
    ll minInternalChildren() const { return (order_ + 1) / 2; }
    static std::size_t childIndex(const Node* parent, const Node* child) {
        return static_cast<std::size_t>(std::find(parent->children.begin(), parent->children.end(), child) - parent->children.begin());
    }
    ll firstKey(const Node* node) const {
        const Node* current = node; while (!current->leaf) current = current->children.front(); return current->keys.front();
    }
    void refreshKeys(Node* node) {
        if (!node || node->leaf) return;
        node->keys.clear();
        for (std::size_t i = 1; i < node->children.size(); ++i) node->keys.push_back(firstKey(node->children[i]));
    }
    void refreshUp(Node* node) { while (node) { refreshKeys(node); node = node->parent; } }
    void refreshSubtree(Node* node) {
        if (!node || node->leaf) return;
        for (Node* child : node->children) refreshSubtree(child);
        refreshKeys(node);
    }
    Node* findLeaf(ll key, SnapshotManager* snapshots, const std::string& verb) const {
        Node* current = root_;
        while (!current->leaf) {
            snapshot(snapshots, "Visit an internal node to " + verb + " " + std::to_string(key), current);
            const std::size_t index = static_cast<std::size_t>(std::upper_bound(current->keys.begin(), current->keys.end(), key) - current->keys.begin());
            current = current->children[index];
        }
        snapshot(snapshots, "Reach the target leaf", current); return current;
    }
    Node* splitLeaf(Node* left) {
        Node* right = makeNode(true); const std::size_t leftCount = (left->keys.size() + 1) / 2;
        right->keys.assign(left->keys.begin() + leftCount, left->keys.end()); left->keys.erase(left->keys.begin() + leftCount, left->keys.end());
        right->next = left->next; left->next = right; right->parent = left->parent; return right;
    }
    Node* splitInternal(Node* left) {
        Node* right = makeNode(false); const std::size_t leftChildren = (left->children.size() + 1) / 2;
        right->children.assign(left->children.begin() + leftChildren, left->children.end()); left->children.erase(left->children.begin() + leftChildren, left->children.end());
        for (Node* child : right->children) child->parent = right;
        right->parent = left->parent; refreshKeys(left); refreshKeys(right); return right;
    }
    void insertIntoParent(Node* left, Node* right, SnapshotManager* snapshots) {
        Node* parent = left->parent;
        if (!parent) {
            parent = makeNode(false); parent->children = {left, right}; left->parent = parent; right->parent = parent; refreshKeys(parent); root_ = parent;
            snapshot(snapshots, "Create a new root with the copied separator", parent); return;
        }
        const std::size_t index = childIndex(parent, left); parent->children.insert(parent->children.begin() + index + 1, right); right->parent = parent; refreshKeys(parent);
        snapshot(snapshots, "Insert the copied separator into the parent", parent);
        if (static_cast<ll>(parent->children.size()) > order_) {
            snapshot(snapshots, "Internal node overflow: split the internal node", parent);
            Node* sibling = splitInternal(parent);
            snapshot(snapshots, "Internal split complete; promote a separator", sibling);
            insertIntoParent(parent, sibling, snapshots);
        } else refreshUp(parent->parent);
    }
    void removeChild(Node* parent, Node* child) { parent->children.erase(parent->children.begin() + childIndex(parent, child)); refreshKeys(parent); }
    void rebalanceLeaf(Node* leaf, SnapshotManager* snapshots) {
        Node* parent = leaf->parent; const std::size_t index = childIndex(parent, leaf);
        Node* left = index > 0 ? parent->children[index - 1] : nullptr;
        Node* right = index + 1 < parent->children.size() ? parent->children[index + 1] : nullptr;
        if (left && static_cast<ll>(left->keys.size()) > minLeafKeys()) {
            snapshot(snapshots, "Leaf underflow: borrow the largest key from the left sibling", left);
            leaf->keys.insert(leaf->keys.begin(), left->keys.back()); left->keys.pop_back(); refreshUp(parent);
            snapshot(snapshots, "Redistribution restores the leaf occupancy", leaf); return;
        }
        if (right && static_cast<ll>(right->keys.size()) > minLeafKeys()) {
            snapshot(snapshots, "Leaf underflow: borrow the smallest key from the right sibling", right);
            leaf->keys.push_back(right->keys.front()); right->keys.erase(right->keys.begin()); refreshUp(parent);
            snapshot(snapshots, "Redistribution restores the leaf occupancy", leaf); return;
        }
        if (left) {
            snapshot(snapshots, "Leaf underflow: merge this leaf into its left sibling", left);
            left->keys.insert(left->keys.end(), leaf->keys.begin(), leaf->keys.end()); left->next = leaf->next; removeChild(parent, leaf);
            snapshot(snapshots, "Leaf merge complete; remove its separator from the parent", left);
        } else {
            snapshot(snapshots, "Leaf underflow: merge the right sibling into this leaf", leaf);
            leaf->keys.insert(leaf->keys.end(), right->keys.begin(), right->keys.end()); leaf->next = right->next; removeChild(parent, right);
            snapshot(snapshots, "Leaf merge complete; remove its separator from the parent", leaf);
        }
        rebalanceInternal(parent, snapshots);
    }
    void rebalanceInternal(Node* node, SnapshotManager* snapshots) {
        if (node == root_) {
            if (node->children.size() == 1) { root_ = node->children.front(); root_->parent = nullptr; refreshSubtree(root_); snapshot(snapshots, "Root has one child, so reduce the tree height", root_); }
            else refreshKeys(node);
            return;
        }
        if (static_cast<ll>(node->children.size()) >= minInternalChildren()) { refreshUp(node); return; }
        Node* parent = node->parent; const std::size_t index = childIndex(parent, node);
        Node* left = index > 0 ? parent->children[index - 1] : nullptr;
        Node* right = index + 1 < parent->children.size() ? parent->children[index + 1] : nullptr;
        if (left && static_cast<ll>(left->children.size()) > minInternalChildren()) {
            snapshot(snapshots, "Internal underflow: borrow the rightmost child from the left sibling", left);
            Node* moved = left->children.back(); left->children.pop_back(); node->children.insert(node->children.begin(), moved); moved->parent = node; refreshUp(parent);
            snapshot(snapshots, "Internal redistribution restores the subtree", node); return;
        }
        if (right && static_cast<ll>(right->children.size()) > minInternalChildren()) {
            snapshot(snapshots, "Internal underflow: borrow the leftmost child from the right sibling", right);
            Node* moved = right->children.front(); right->children.erase(right->children.begin()); node->children.push_back(moved); moved->parent = node; refreshUp(parent);
            snapshot(snapshots, "Internal redistribution restores the subtree", node); return;
        }
        if (left) {
            snapshot(snapshots, "Internal underflow: merge into the left sibling", left);
            left->children.insert(left->children.end(), node->children.begin(), node->children.end()); for (Node* child : node->children) child->parent = left; refreshKeys(left); removeChild(parent, node);
            snapshot(snapshots, "Internal merge complete; repair the parent", left);
        } else {
            snapshot(snapshots, "Internal underflow: merge the right sibling into this node", node);
            node->children.insert(node->children.end(), right->children.begin(), right->children.end()); for (Node* child : right->children) child->parent = node; refreshKeys(node); removeChild(parent, right);
            snapshot(snapshots, "Internal merge complete; repair the parent", node);
        }
        rebalanceInternal(parent, snapshots);
    }
};

inline void SnapshotManager::takeSnapshot(const BPlusTree& tree, const std::string& message, const std::string& highlightNodeId) {
    snapshots.push_back(Snapshot{tree.toJson(), message, highlightNodeId});
}

#endif
