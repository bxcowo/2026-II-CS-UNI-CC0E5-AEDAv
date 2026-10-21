#ifndef __BINARYTREE_H__
#define __BINARYTREE_H__

#include <initializer_list>
#include <mutex>
#include <utility>
#include <functional>
#include <iterator>

#include "call.h"
#include "GeneralIterator.h"
#include "GeneralNode.h"

enum class TreeOrder { Pre, In, Post };

template <typename T>
class BinaryTreeNode : public GeneralNode<T> {
public:
    using value_type = T;
    using MySelf     = BinaryTreeNode<T>;
    using Node       = MySelf;
    using NodePtr    = Node*;
    using Parent     = GeneralNode<T>;
    using Parent::Parent; // Inherit constructor

    NodePtr m_parent    = nullptr;  
    NodePtr m_pChild[2] = {nullptr, nullptr}; // left and right children

    template <int Prio>
    static NodePtr extreme(NodePtr node){
        if (!node) return nullptr;
        while (node->m_pChild[Prio]) node = node->m_pChild[Prio];
        return node;
    }

    template <int Prio>
    static NodePtr firstLeaf(NodePtr node){
        if (!node) return nullptr;
        while (node->m_pChild[0] || node->m_pChild[1])
            node = node->m_pChild[Prio] ? node->m_pChild[Prio] : node->m_pChild[1 - Prio];
        return node;
    }

    template <int Prio>
    static NodePtr inOrderNext(NodePtr node) {
        constexpr int Second = 1 - Prio;
        if (!node) return nullptr;
        if (node->m_pChild[Second]) return extreme<Prio>(node->m_pChild[Second]);
        NodePtr p = node->m_parent;
        while (p && node == p->m_pChild[Second]) {
            node = p;
            p = p->m_parent;
        }
        return p;
    }

    template <int Prio>
    static NodePtr preOrderNext(NodePtr node) {
        constexpr int Second = 1 - Prio;
        if (!node) return nullptr;
        if (node->m_pChild[Prio]) return node->m_pChild[Prio];
        if (node->m_pChild[Second]) return node->m_pChild[Second];
        NodePtr p = node->m_parent;
        while (p && (node == p->m_pChild[Second] || !p->m_pChild[Second])) {
            node = p;
            p = p->m_parent;
        }
        return p ? p->m_pChild[Second] : nullptr;
    }

    template <int Prio>
    static NodePtr postOrderNext(NodePtr node) {
        constexpr int Second = 1 - Prio;
        if (!node || !node->m_parent) return nullptr;
        NodePtr p = node->m_parent;
        if (node == p->m_pChild[Second] || !p->m_pChild[Second]) return p;
        return firstLeaf<Prio>(p->m_pChild[Second]);
    }
};

template <typename NodeT, TreeOrder Order, bool Reverse>
class BinaryTreeIterator : public GeneralIterator<BinaryTreeIterator<NodeT, Order, Reverse>, NodeT> {
public:
    using Node = NodeT;
    using NodePtr = Node*;
    using MySelf = BinaryTreeIterator<NodeT, Order, Reverse>;
    using Parent = GeneralIterator<MySelf, NodeT>;
    using Parent::Parent;
private:
    static constexpr int Prio = Reverse ? 1 : 0;
    static constexpr TreeOrder Effective = (!Reverse || Order == TreeOrder::In) ? Order : (Order == TreeOrder::Pre ? TreeOrder::Post : TreeOrder::Pre);
public:
    static NodePtr first(NodePtr root){
        if constexpr (Effective == TreeOrder::In) return Node::template extreme<Prio>(root);
        else if constexpr (Effective == TreeOrder::Pre) return root;
        else return Node::template firstLeaf<Prio>(root);
    }

    MySelf& operator++() {
        if constexpr (Effective == TreeOrder::In)
            this->m_ptr = Node::template inOrderNext<Prio>(this->m_ptr);
        else if constexpr (Effective == TreeOrder::Pre)
            this->m_ptr = Node::template preOrderNext<Prio>(this->m_ptr);
        else
            this->m_ptr = Node::template postOrderNext<Prio>(this->m_ptr);
        return *this;
    }
};

template <typename T, typename Comp>
struct BinaryTreeTraits {
    using value_type       = T;
    using Compare          = Comp;
    using Node             = BinaryTreeNode<value_type>;
    using ForwardIterator  = BinaryTreeIterator<Node, TreeOrder::In, false>;
    using BackwardIterator = BinaryTreeIterator<Node, TreeOrder::In, true>;
};

template <typename Tree, TreeOrder Order>
class BinaryTreeView : public ForEachExt<BinaryTreeView<Tree, Order>>, public FirstThatExt<BinaryTreeView<Tree, Order>> {
public:
    using value_type       = typename Tree::value_type;
    using Node             = typename Tree::Node;
    using ForwardIterator  = BinaryTreeIterator<Node, Order, false>;
    using BackwardIterator = BinaryTreeIterator<Node, Order, true>;
private:
    const Tree& m_tree;
public:
    BinaryTreeView(const Tree& tree) : m_tree(tree) {}
    auto acquireLock() const { return m_tree.acquireLock(); }

    ForwardIterator begin()   const { return ForwardIterator(ForwardIterator::first(m_tree.m_root)); }
    ForwardIterator end()     const { return ForwardIterator(nullptr); }
    BackwardIterator rbegin() const { return BackwardIterator(BackwardIterator::first(m_tree.m_root)); }
    BackwardIterator rend()   const { return BackwardIterator(nullptr); }
};

template <typename Traits>
class BinaryTree : public ForEachExt<BinaryTree<Traits>>, public FirstThatExt<BinaryTree<Traits>>, public PersistTreeExt<BinaryTree<Traits>, Traits> {
    using Persist = PersistTreeExt<BinaryTree<Traits>, Traits>;
    template <typename, TreeOrder> friend class BinaryTreeView;

public:
    using value_type       = typename Traits::value_type;
    using Compare          = typename Traits::Compare;
    using Node             = typename Traits::Node;
    using NodePtr          = Node*;
    using ForwardIterator  = typename Traits::ForwardIterator;
    using BackwardIterator = typename Traits::BackwardIterator;

private:
    NodePtr            m_root = nullptr; // root of the binary tree
    size_t             m_size = 0;
    Compare            m_comp;
    mutable std::mutex m_mutex;

    void internal_insert(const value_type& value, Ref ref) {
        NodePtr parent = nullptr;
        NodePtr* link  = &m_root;
        while (*link) {
            parent = *link;
            link   = &parent->m_pChild[m_comp(parent->getValue(), value)];
        }
        *link = new Node(value, ref);
        (*link)->m_parent = parent;
        ++m_size;
    }

    static NodePtr clone(NodePtr src, NodePtr parent) {
        if (!src) return nullptr;
        NodePtr node = new Node(src->getValue(), src->getRef());
        node->m_parent = parent;
        node->m_pChild[0] = clone(src->m_pChild[0], node);
        node->m_pChild[1] = clone(src->m_pChild[1], node);
        return node;
    }

    static void destroy(NodePtr root){
        while(root){
            if (NodePtr left = root->m_pChild[0]){
                root->m_pChild[0] = left->m_pChild[1];
                left->m_pChild[1] = root;
                root = left;
            } else {
                NodePtr next = root->m_pChild[1];
                delete root;
                root = next;
            }
        }
    }

    
public:
    BinaryTree() = default;
    BinaryTree(Compare comp) : m_comp(std::move(comp)) {}

    BinaryTree(std::initializer_list<std::pair<value_type, Ref>> values, Compare comp = Compare()) : m_comp(std::move(comp)){
        for (const auto& [value, ref] : values){
            internal_insert(value, ref);
        }
    }

    BinaryTree(const BinaryTree& other){
        auto lock = other.acquireLock();
        m_comp = other.m_comp;
        m_root = clone(other.m_root, nullptr);
        m_size = other.m_size;
    }

    BinaryTree(BinaryTree&& other){
        auto lock = other.acquireLock();
        m_comp = std::move(other.m_comp);
        m_root = std::exchange(other.m_root, nullptr);
        m_size = std::exchange(other.m_size, 0);
    }

    BinaryTree& operator=(const BinaryTree& other){
        if (this != &other){
            std::scoped_lock lock(m_mutex, other.m_mutex);
            NodePtr copy = clone(other.m_root, nullptr);
            destroy(m_root);
            m_root = copy;
            m_size = other.m_size;
            m_comp = other.m_comp;
        }
        return *this;
    }

    BinaryTree& operator=(BinaryTree&& other){
        if (this != &other){
            std::scoped_lock lock(m_mutex, other.m_mutex);
            destroy(m_root);
            m_root = std::exchange(other.m_root, nullptr);
            m_size = std::exchange(other.m_size, 0);
            m_comp = std::move(other.m_comp);
        }
        return *this;
    }

    ~BinaryTree() { destroy(m_root); }
    
    std::unique_lock<std::mutex> acquireLock() const {
        return std::unique_lock<std::mutex>(m_mutex);
    }

    ForwardIterator begin()   const { return ForwardIterator(ForwardIterator::first(m_root)); }
    ForwardIterator end()     const { return ForwardIterator(nullptr); }
    BackwardIterator rbegin() const { return BackwardIterator(BackwardIterator::first(m_root)); }
    BackwardIterator rend()   const { return BackwardIterator(nullptr); }

    template <TreeOrder Order>
    BinaryTreeView<BinaryTree, Order> traversal() const { return BinaryTreeView<BinaryTree, Order>(*this); }

    auto preorder()  const { return traversal<TreeOrder::Pre>(); }
    auto inorder()   const { return traversal<TreeOrder::In>(); }
    auto postorder() const { return traversal<TreeOrder::Post>(); }

    void push(const value_type& value, Ref ref) {
        std::lock_guard<std::mutex> lock(m_mutex);
        internal_insert(value, ref);
    }

    size_t size() const { return m_size; }
    bool empty()  const { return m_size == 0; }

    void clear(){
        std::lock_guard<std::mutex> lock(m_mutex);
        destroy(m_root);
        m_root = nullptr;
        m_size = 0;
    }

    std::ostream& write(std::ostream& os) const {
        auto lock = acquireLock();
        bool first = true;
        os << Persist::Open;
        for (const Node& node : preorder()) {
            if (!first) os << Persist::Separator;
            os << node;
            first = false;
        }
        return os << Persist::Close;
    }
};


template <typename T>
using BST = BinaryTree<BinaryTreeTraits<T, std::less<T>>>;
template <typename T>
using DescBST = BinaryTree<BinaryTreeTraits<T, std::greater<T>>>;

#endif // __BINARYTREE_H__