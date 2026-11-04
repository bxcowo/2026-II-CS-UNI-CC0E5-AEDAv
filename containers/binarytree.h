#ifndef __BINARYTREE_H__
#define __BINARYTREE_H__
#include "GeneralNode.h"
#include "GeneralIterator.h"
#include "GeneralContainer.h"
#include "mixins.h"
#include "../utilities/printcolored.h"

enum class TreeOrder : size_t { Pre = 0, In = 1, Post = 2 };

template <typename T, typename Derived>
class BaseBinaryTreeNode : public GeneralNode<T> {
public:
    using value_type = T;
    using Node       = Derived;
    using NodePtr    = Node*;
    using Parent     = GeneralNode<T>;
    using Parent::Parent; // Inherit constructor

    NodePtr m_pChild[2] = {nullptr, nullptr}; // left and right children
    NodePtr m_parent    = nullptr;

    template <size_t Prio>
    static NodePtr extreme(NodePtr node){
        if (!node) return nullptr;
        while (node->m_pChild[Prio]) node = node->m_pChild[Prio];
        return node;
    }

    template <size_t Prio>
    static NodePtr firstLeaf(NodePtr node){
        if (!node) return nullptr;
        while (node->m_pChild[0] || node->m_pChild[1])
            node = node->m_pChild[Prio] ? node->m_pChild[Prio] : node->m_pChild[1 - Prio];
        return node;
    }

    template <size_t Prio>
    static NodePtr inOrderNext(NodePtr node) {
        constexpr size_t Second = 1 - Prio;
        if (!node) return nullptr;
        if (node->m_pChild[Second]) return extreme<Prio>(node->m_pChild[Second]);
        NodePtr parent = node->m_parent;
        while (parent && node == parent->m_pChild[Second]) {
            node   = parent;
            parent = parent->m_parent;
        }
        return parent;
    }

    template <size_t Prio>
    static NodePtr preOrderNext(NodePtr node) {
        constexpr size_t Second = 1 - Prio;
        if (!node) return nullptr;
        if (node->m_pChild[Prio]) return node->m_pChild[Prio];
        if (node->m_pChild[Second]) return node->m_pChild[Second];
        NodePtr parent = node->m_parent;
        while (parent && (node == parent->m_pChild[Second] || !parent->m_pChild[Second])) {
            node   = parent;
            parent = parent->m_parent;
        }
        return parent ? parent->m_pChild[Second] : nullptr;
    }

    template <size_t Prio>
    static NodePtr postOrderNext(NodePtr node) {
        constexpr size_t Second = 1 - Prio;
        if (!node || !node->m_parent) return nullptr;
        NodePtr parent = node->m_parent;
        if (node == parent->m_pChild[Second] || !parent->m_pChild[Second]) return parent;
        return firstLeaf<Prio>(parent->m_pChild[Second]);
    }
};

template<typename T>
class BinaryTreeNode : public BaseBinaryTreeNode<T, BinaryTreeNode<T>> {
public: 
    using Parent = BaseBinaryTreeNode<T, BinaryTreeNode<T>>;
    using Parent::Parent;
};

template <typename NodeT, bool Reverse>
class BinaryTreeIterator
    : public GeneralIterator<BinaryTreeIterator<NodeT, Reverse>, NodeT> {
public:
    using Node    = NodeT;
    using NodePtr = Node*;
    using MySelf  = BinaryTreeIterator<NodeT, Reverse>;
    using Parent  = GeneralIterator<MySelf, NodeT>;
    using NodeFn  = NodePtr (*)(NodePtr);

private:
    NodeFn m_next = nullptr;

    static NodePtr self(NodePtr node){ return node; }

    static constexpr NodeFn s_first[2][3] = {
        // Normal
        { &self,
          &Node::template extreme<0>,
          &Node::template firstLeaf<0> },
        // Inverso
        { &Node::template firstLeaf<1>,
          &Node::template extreme<1>,
          &self }
    };

    static constexpr NodeFn s_next[2][3] = {
        // Normal
        { &Node::template preOrderNext<0>,
          &Node::template inOrderNext<0>,
          &Node::template postOrderNext<0> },
        // Inverso
        { &Node::template postOrderNext<1>,
          &Node::template inOrderNext<1>,
          &Node::template preOrderNext<1> }
    };

public:
    explicit BinaryTreeIterator(NodePtr ptr, NodeFn next = nullptr) : Parent(ptr), m_next(next) {}

    static MySelf make(NodePtr root, TreeOrder order) {
        if (!root) return MySelf(nullptr);
        const auto index = static_cast<size_t>(order);
        return MySelf(s_first[Reverse][index](root), s_next[Reverse][index]);
    }

    MySelf& operator++() {
        if (this->m_ptr && m_next) this->m_ptr = m_next(this->m_ptr);
        return *this;
    }
};

template <typename T>
struct BinaryTreeTraits {
    using Node             = BinaryTreeNode<T>;
    using ForwardIterator  = BinaryTreeIterator<Node, false>;
    using BackwardIterator = BinaryTreeIterator<Node, true>;
};

template <typename T>
struct BinaryTreeAscTraits : public AscendingTraits<T>, public BinaryTreeTraits<T> {};

template <typename T>
struct BinaryTreeDescTraits : public DescendingTraits<T>, public BinaryTreeTraits<T> {};

template <typename Traits, template <typename> typename... Mixins>
class BaseBinaryTree;

template <typename Traits>
using BinaryTree = BaseBinaryTree<Traits, Callable, Applicable, Searchable, ReverseCallable, Printable, Readable>;

template <typename Traits, template <typename> typename... Mixins>
class BaseBinaryTree : public GeneralContainer<Traits, Mixins...> {
public:
    using MySelf           = BaseBinaryTree;
    using Parent           = GeneralContainer<Traits, Mixins...>;
    using typename Parent::value_type;
    using typename Parent::Node;
    using typename Parent::NodePtr;
    using typename Parent::ForwardIterator;
    using BackwardIterator  = typename Traits::BackwardIterator;
    using Compare           = typename Traits::Compare;

protected:
    using Parent::m_mutex;
    NodePtr m_root = nullptr;

private:
    size_t m_size = 0;

    void clear_recursive(NodePtr node);
    NodePtr internal_insert(NodePtr& node, const value_type& value, Ref ref, NodePtr parent);
    NodePtr findNode(const value_type& value) const;
    void transplant(NodePtr u, NodePtr v);

protected:
    virtual NodePtr insertImpl(const value_type& value, Ref ref);
    virtual void removeNodeImpl(NodePtr node);
    NodePtr eraseNode(NodePtr node);

    static constexpr const char* kEmptyCell       = " ";
    static constexpr const char* kHorizontal      = "─";
    static constexpr const char* kLeftChildCorner  = "┌";
    static constexpr const char* kRightChildCorner = "┐";
    static constexpr const char* kBothChildren    = "┴";
    static constexpr const char* kOnlyLeftChild   = "┘";
    static constexpr const char* kOnlyRightChild  = "└";

public:
    BaseBinaryTree() = default;
    BaseBinaryTree(const BaseBinaryTree& another);
    BaseBinaryTree(BaseBinaryTree&& another) noexcept;
    BaseBinaryTree(std::initializer_list<std::pair<value_type, Ref>> values);

    BaseBinaryTree& operator=(const BaseBinaryTree& another);
    BaseBinaryTree& operator=(BaseBinaryTree&& another) noexcept;

    virtual ~BaseBinaryTree();

    void insert(const value_type& value, Ref ref) {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        insertImpl(value, ref);
        ++m_size;
    }

    void push_back(const value_type& value, Ref ref) override {
        insert(value, ref);
    }

    void remove(const value_type& value);
    void clear() override;

    size_t size() const {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        return m_size;
    }

    bool empty() const override {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        return m_size == 0;
    }

    auto acquireLock() const {
        return std::unique_lock<std::recursive_mutex>(m_mutex);
    }

    NodePtr GetRoot() const {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        return m_root;
    }

    ForwardIterator begin() const override { return begin(TreeOrder::In); }
    ForwardIterator end() const override { return ForwardIterator(nullptr); }
    BackwardIterator rbegin() const {return rbegin(TreeOrder::In); }
    BackwardIterator rend() const { return BackwardIterator(nullptr); }

    ForwardIterator begin(TreeOrder order) const {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        return ForwardIterator::make(m_root, order);
    }
    
    BackwardIterator rbegin(TreeOrder order) const {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        return BackwardIterator::make(m_root, order);
    }

    void PrintVertical(std::ostream& os = std::cout) const {
        static const Color kLevelColors[] = {
            Color::Red, Color::Green, Color::Yellow,
            Color::Blue, Color::Magenta, Color::Cyan
        };

        auto lock = acquireLock();
        if (!m_root) return;

        struct Item {
            size_t col, center, depth;
            std::string label;
        };

        std::unordered_map<NodePtr, Item> items;
        size_t width = 0, height = 0;

        // Inorden: la columna crece de izquierda a derecha
        for (auto it = begin(TreeOrder::In); it != end(); ++it) {
            NodePtr node = &*it;

            std::ostringstream text;
            text << node->getValue();
            std::string label = text.str();

            size_t depth = 0;
            for (NodePtr p = node->m_parent; p; p = p->m_parent) ++depth;

            items[node] = {width, width + label.size() / 2, depth, label};
            width += label.size() + 1;
            height = std::max(height, depth + 1);
        }

        // Filas pares: nodos. Filas impares: conectores
        std::vector<std::vector<std::string>> canvas(2 * height - 1, std::vector<std::string>(width, kEmptyCell));

        for (const auto& [node, item] : items) {
            const size_t row = item.depth * 2;

            for (size_t i = 0; i < item.label.size(); ++i)
                canvas[row][item.col + i] = std::string(1, item.label[i]);

            NodePtr left = node->m_pChild[0], right = node->m_pChild[1];
            if (!left && !right) continue;

            const size_t from = left  ? items.at(left).center  : item.center;
            const size_t to   = right ? items.at(right).center : item.center;
            auto& connectors = canvas[row + 1];

            for (size_t col = from; col <= to; ++col) connectors[col] = kHorizontal;
            if (left)  connectors[from] = kLeftChildCorner;
            if (right) connectors[to]   = kRightChildCorner;
            connectors[item.center] = (left && right) ? kBothChildren : (left ? kOnlyLeftChild : kOnlyRightChild);
        }

        for (size_t row = 0; row < canvas.size(); ++row) {
            const auto& cells = canvas[row];

            size_t last = cells.size();
            while (last > 0 && cells[last - 1] == kEmptyCell) --last;

            std::string line;
            for (size_t col = 0; col < last; ++col) line += cells[col];

            PrintColored(os, line, kLevelColors[(row / 2) % 6]);
        }
    }

    void save(std::ostream& os) const {
        auto lock = acquireLock();
        std::string sep;
        os << "[";
        for (auto it = begin(TreeOrder::Pre); it != end(); ++it) {
            os << sep << *it;
            sep = ",";
        }
        os << "]";
    }

    friend std::ostream& operator<<(std::ostream& os, const BaseBinaryTree& tree) {
        tree.save(os);
        return os;
    }
};

template <typename Traits, template <typename> typename... Mixins>
BaseBinaryTree<Traits, Mixins...>::BaseBinaryTree(const BaseBinaryTree& another) : ContainerCore<Traits>(), GeneralContainer<Traits, Mixins...>() {
    *this = another;
}

template <typename Traits, template <typename> typename... Mixins>
BaseBinaryTree<Traits, Mixins...>::BaseBinaryTree(BaseBinaryTree&& another) noexcept : ContainerCore<Traits>(), GeneralContainer<Traits, Mixins...>() {
    *this = std::move(another);
}

template <typename Traits, template <typename> typename... Mixins>
BaseBinaryTree<Traits, Mixins...>::BaseBinaryTree(std::initializer_list<std::pair<value_type, Ref>> values) : ContainerCore<Traits>(), GeneralContainer<Traits, Mixins...>() {
    for (const auto& value : values)
        push_back(value.first, value.second);
}

template <typename Traits, template <typename> typename... Mixins>
BaseBinaryTree<Traits, Mixins...>::~BaseBinaryTree() {
    clear();
}

template <typename Traits, template <typename> typename... Mixins>
void BaseBinaryTree<Traits, Mixins...>::clear_recursive(NodePtr node) {
    if (!node) return;
    clear_recursive(node->m_pChild[0]);
    clear_recursive(node->m_pChild[1]);
    delete node;
}

template <typename Traits, template <typename> typename... Mixins>
auto BaseBinaryTree<Traits, Mixins...>::internal_insert(NodePtr& node, const value_type& value, Ref ref, NodePtr parent) -> NodePtr {
    NodePtr* link           = &node;
    NodePtr  current_parent = parent;

    while (*link) {
        current_parent = *link;
        const size_t branch = Compare()((*link)->getValue(), value) ? 1 : 0;
        link = &(*link)->m_pChild[branch];
    }

    *link = new Node(value, ref);
    (*link)->m_parent = current_parent;
    return *link;
}

template <typename Traits, template <typename> typename... Mixins>
auto BaseBinaryTree<Traits, Mixins...>::findNode(
    const value_type& value) const -> NodePtr {
    NodePtr current = m_root;
    while (current) {
        if (Compare()(value, current->getValue())) {
            current = current->m_pChild[0];
        } else if (Compare()(current->getValue(), value)) {
            current = current->m_pChild[1];
        } else {
            return current;
        }
    }
    return nullptr;
}

template <typename Traits, template <typename> typename... Mixins>
void BaseBinaryTree<Traits, Mixins...>::transplant(NodePtr u, NodePtr v) {
    if (!u->m_parent) {
        m_root = v;
    } else {
        u->m_parent->m_pChild[u == u->m_parent->m_pChild[1]] = v;
    }
    if (v) v->m_parent = u->m_parent;
}

template <typename Traits, template <typename> typename... Mixins>
auto BaseBinaryTree<Traits, Mixins...>::insertImpl(const value_type& value, Ref ref) -> NodePtr {
    return internal_insert(m_root, value, ref, nullptr);
}

template <typename Traits, template <typename> typename... Mixins>
void BaseBinaryTree<Traits, Mixins...>::removeNodeImpl(NodePtr node) {
    eraseNode(node);
}

template <typename Traits, template <typename> typename... Mixins>
auto BaseBinaryTree<Traits, Mixins...>::eraseNode(NodePtr node) -> NodePtr {
    NodePtr start = nullptr;

    if (!node->m_pChild[0] || !node->m_pChild[1]) {
        NodePtr child  = node->m_pChild[0] ? node->m_pChild[0] : node->m_pChild[1];
        NodePtr parent = node->m_parent;
        transplant(node, child);
        start = parent;
    } else {
        NodePtr successor = node->m_pChild[1];
        while (successor->m_pChild[0])
            successor = successor->m_pChild[0];

        NodePtr successor_parent = successor->m_parent;
        if (successor_parent != node) {
            transplant(successor, successor->m_pChild[1]);
            successor->m_pChild[1] = node->m_pChild[1];
            successor->m_pChild[1]->m_parent = successor;
            start = successor_parent;
        } else {
            start = successor;
        }

        transplant(node, successor);
        successor->m_pChild[0] = node->m_pChild[0];
        successor->m_pChild[0]->m_parent = successor;
    }

    delete node;
    return start;
}

template <typename Traits, template <typename> typename... Mixins>
void BaseBinaryTree<Traits, Mixins...>::remove(const value_type& value) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    NodePtr node = findNode(value);
    if (!node)
        throw std::out_of_range("BinaryTree::remove: valor no encontrado");
    removeNodeImpl(node);
    --m_size;
}

template <typename Traits, template <typename> typename... Mixins>
void BaseBinaryTree<Traits, Mixins...>::clear() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    clear_recursive(m_root);
    m_root = nullptr;
    m_size = 0;
}

template <typename Traits, template <typename> typename... Mixins>
auto BaseBinaryTree<Traits, Mixins...>::operator=(const BaseBinaryTree& another) -> BaseBinaryTree& {
    if (this == &another) return *this;

    std::scoped_lock lock(m_mutex, another.m_mutex);
    clear();

    // Preorden: evita que la copia degenere en una lista
    for (auto it = ForwardIterator::make(another.m_root, TreeOrder::Pre); it != ForwardIterator(nullptr); ++it) {
        push_back((*it).getValue(), (*it).getRef());
    }
    return *this;
}

template <typename Traits, template <typename> typename... Mixins>
auto BaseBinaryTree<Traits, Mixins...>::operator=(BaseBinaryTree&& another) noexcept -> BaseBinaryTree& {
    this->move_from(another, [&] {
        m_root = std::exchange(another.m_root, nullptr);
        m_size = std::exchange(another.m_size, 0);
    });
    return *this;
}

template <typename T> using AscendingBinaryTree = BinaryTree<BinaryTreeAscTraits<T>>;
template <typename T> using DescendingBinaryTree = BinaryTree<BinaryTreeDescTraits<T>>;

#endif // __BINARYTREE_H__