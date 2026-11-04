#ifndef AVLTREE_H
#define AVLTREE_H

#include "binarytree.h"

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <utility>

template <typename T>
class AVLNode : public BaseBinaryTreeNode<T, AVLNode<T>> {
public:
    using Parent = BaseBinaryTreeNode<T, AVLNode<T>>;
    using Parent::Parent;
    size_t m_height = 1;
};

template <typename T>
struct AVLTraits {
    using Node             = AVLNode<T>;
    using ForwardIterator  = BinaryTreeIterator<Node, false>;
    using BackwardIterator = BinaryTreeIterator<Node, true>;
};

template <typename T>
struct AVLTreeAscTraits : public AscendingTraits<T>, public AVLTraits<T> {};

template <typename T>
struct AVLTreeDescTraits : public DescendingTraits<T>, public AVLTraits<T> {};

template <typename Traits, template <typename> typename... Mixins>
class BaseAVLTree : public BaseBinaryTree<Traits, Mixins...> {
public:
    using Parent     = BaseBinaryTree<Traits, Mixins...>;
    using Node       = typename Parent::Node;
    using NodePtr    = typename Parent::NodePtr;
    using value_type = typename Parent::value_type;

    BaseAVLTree() = default;

    BaseAVLTree(const BaseAVLTree& other) : Parent() {
        Parent::operator=(other);
    }

    BaseAVLTree(BaseAVLTree&& other) noexcept : Parent(std::move(other)) {}

    BaseAVLTree(std::initializer_list<std::pair<value_type, Ref>> values) : Parent() {
        for (const auto& value : values)
            this->push_back(value.first, value.second);
    }

    BaseAVLTree& operator=(const BaseAVLTree& other) {
        Parent::operator=(other);
        return *this;
    }

    BaseAVLTree& operator=(BaseAVLTree&& other) noexcept {
        Parent::operator=(std::move(other));
        return *this;
    }

    size_t height() const {
        auto lock = this->acquireLock();
        return this->m_root ? this->m_root->m_height : 0;
    }

    bool isBalanced() const {
        auto lock = this->acquireLock();
        return isBalancedNode(this->m_root);
    }

protected:
    NodePtr insertImpl(const value_type& value, Ref ref) override {
        NodePtr node = Parent::insertImpl(value, ref);
        rebalanceFrom(node->m_parent, true);
        return node;
    }

    void removeNodeImpl(NodePtr node) override {
        NodePtr start = this->eraseNode(node);
        rebalanceFrom(start);
    }

    static size_t nodeHeight(NodePtr node) {
        return node ? node->m_height : 0;
    }

    static std::ptrdiff_t balanceFactor(NodePtr node) {
        return static_cast<std::ptrdiff_t>(nodeHeight(node->m_pChild[0])) - static_cast<std::ptrdiff_t>(nodeHeight(node->m_pChild[1]));
    }

    static void updateHeight(NodePtr node) {
        node->m_height = 1 + std::max(nodeHeight(node->m_pChild[0]), nodeHeight(node->m_pChild[1]));
    }

    // dir = 0: rotación izquierda; dir = 1: rotación derecha
    NodePtr rotate(NodePtr x, size_t dir) {
        NodePtr y      = x->m_pChild[1 - dir];
        NodePtr beta   = y->m_pChild[dir];
        NodePtr parent = x->m_parent;

        y->m_parent = parent;
        if (!parent)
            this->m_root = y;
        else
            parent->m_pChild[x == parent->m_pChild[1]] = y;

        y->m_pChild[dir]     = x;
        x->m_parent          = y;
        x->m_pChild[1 - dir] = beta;
        if (beta) beta->m_parent = x;

        updateHeight(x);
        updateHeight(y);
        return y;
    }

    // stopWhenStable solo es válido al insertar: al eliminar,
    // el sucesor puede llegar con una altura desactualizada.
    void rebalanceFrom(NodePtr node, bool stopWhenStable = false) {
        while (node) {
            const size_t previous = node->m_height;
            updateHeight(node);
            const std::ptrdiff_t factor = balanceFactor(node);

            if (factor > 1) {
                if (balanceFactor(node->m_pChild[0]) < 0)
                    rotate(node->m_pChild[0], 0);
                node = rotate(node, 1);
            } else if (factor < -1) {
                if (balanceFactor(node->m_pChild[1]) > 0)
                    rotate(node->m_pChild[1], 1);
                node = rotate(node, 0);
            }

            if (stopWhenStable && node->m_height == previous) return;
            node = node->m_parent;
        }
    }

    static bool isBalancedNode(NodePtr node) {
        if (!node) return true;
        const std::ptrdiff_t factor = balanceFactor(node);
        if (factor < -1 || factor > 1) return false;
        return isBalancedNode(node->m_pChild[0]) && isBalancedNode(node->m_pChild[1]);
    }
};

template <typename Traits>
using AVLTree = BaseAVLTree<Traits, Callable, Applicable, Searchable, ReverseCallable, Printable, Readable>;

template <typename T> using AscendingAVLTree = AVLTree<AVLTreeAscTraits<T>>;
template <typename T> using DescendingAVLTree = AVLTree<AVLTreeDescTraits<T>>;

#endif // AVLTREE_H