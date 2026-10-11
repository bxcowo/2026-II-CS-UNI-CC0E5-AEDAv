#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__
#include <mutex>
#include "GeneralNode.h"
#include "GeneralIterator.h"
#include "../foreach.h"

// Nodos
template <typename T>
class SingleLinkedListNode : public GeneralNode<T> {
    using Node    = SingleLinkedListNode<T>;
    using NodePtr = Node*;
public:
    NodePtr m_pNext = nullptr; 
    SingleLinkedListNode() : GeneralNode<T>(T{}, Ref{}), m_pNext(nullptr) {}
    SingleLinkedListNode(const T& value, Ref ref, NodePtr pNext) : GeneralNode<T>(value, ref), m_pNext(pNext) {}
};

template <typename T>
class DoubleLinkedListNode : public GeneralNode<T>{
    using Node    = DoubleLinkedListNode<T>;
    using NodePtr = Node*;
public:
    NodePtr m_pNext = nullptr;
    NodePtr m_pPrev = nullptr;
    DoubleLinkedListNode() : GeneralNode<T>(T{}, Ref{}), m_pNext(nullptr), m_pPrev(nullptr) {}
    DoubleLinkedListNode(const T& value, Ref ref, NodePtr next) : GeneralNode<T>(value, ref), m_pNext(next), m_pPrev(nullptr) {}
    DoubleLinkedListNode(const T& value, Ref ref, NodePtr pNext, NodePtr pPrev) : GeneralNode<T>(value, ref), m_pNext(pNext), m_pPrev(pPrev) {}
};

// Iteradores
template <typename NodeT>
class LinkedListForwardIterator : public GeneralIterator<LinkedListForwardIterator<NodeT>, NodeT> {
public:
    using node_type  = NodeT;
    using MySelf     = LinkedListForwardIterator<NodeT>;
    using Parent     = GeneralIterator<MySelf, node_type>;
    using Parent::Parent;

    MySelf& operator++() { Parent::m_ptr = Parent::m_ptr->m_pNext; return *this; }
};

template <typename NodeT>
class LinkedListBackwardIterator : public GeneralIterator<LinkedListBackwardIterator<NodeT>, NodeT> {
public:
    using node_type = NodeT;
    using MySelf     = LinkedListBackwardIterator<NodeT>;
    using Parent     = GeneralIterator<MySelf, node_type>;
    using Parent::Parent;

    MySelf& operator++() { Parent::m_ptr = Parent::m_ptr->m_pPrev; return *this; }
};

// Traits
template <typename T, typename _Compare>
struct LinkedListTraits {
    using value_type       = T;
    using Node             = SingleLinkedListNode<T>;
    using NodePtr          = Node*;
    using ForwardIterator  = LinkedListForwardIterator<Node>;
    using Compare          = _Compare;
};

template <typename T, typename _Compare>
struct DoubleLinkedListTraits {
    using value_type       = T;
    using Node             = DoubleLinkedListNode<T>;
    using NodePtr          = Node*;
    using ForwardIterator  = LinkedListForwardIterator<Node>;
    using BackwardIterator = LinkedListBackwardIterator<Node>;
    using Compare          = _Compare;
};

template <typename T> using LinkedListAscTraits        = LinkedListTraits<T, std::less<T>>;
template <typename T> using LinkedListDescTraits       = LinkedListTraits<T, std::greater<T>>;
template <typename T> using DoubleLinkedListAscTraits  = DoubleLinkedListTraits<T, std::less<T>>;
template <typename T> using DoubleLinkedListDescTraits = DoubleLinkedListTraits<T, std::greater<T>>;

template <typename Traits>
concept HasBackwardIterator = requires { typename Traits::BackwardIterator; };

template <typename Derived, typename Traits>
class ListBase {
public:
    using value_type      = typename Traits::value_type;
    using Node            = typename Traits::Node;
    using NodePtr         = typename Traits::NodePtr;
    using ForwardIterator = typename Traits::ForwardIterator;
    using Compare         = typename Traits::Compare;
    using Delim           = typename Node::Delim;

    // Métodos estáticos reutilizables por clases hijas especializadas
    static void    init(NodePtr&, NodePtr&) {}
    static NodePtr firstNode(NodePtr root) { return root; }
    static NodePtr endNode(NodePtr) { return nullptr; }
    static bool    isEmpty(NodePtr root) { return root == nullptr; }

    static void insertAfter(NodePtr prev, NodePtr n, NodePtr& tail) {
        Derived::linkAfter(prev, n);
        if (prev == tail) tail = n;
    }

    static void append(NodePtr& root, NodePtr& tail, NodePtr n) {
        if (!root) { root = tail = n; return; }
        Derived::insertAfter(tail, n, tail);
    }

    static void prepend(NodePtr& root, NodePtr& tail, NodePtr n) {
        if (!root) { root = tail = n; return; }
        Derived::linkBefore(root, n);
        root = n;
    }
private:
    NodePtr m_pRoot = nullptr; // puntero al primer nodo de la lista enlazada
    NodePtr m_pTail = nullptr; // puntero al último nodo de la lista enlazada
    Compare m_comp; // comparador para ordenar los nodos de la lista
    mutable std::mutex m_mutex; // mutex para sincronización

    NodePtr GetRoot() const { return m_pRoot; }
    NodePtr GetTail() const { return m_pTail; }

    NodePtr findInsertPos(const value_type& value) const {
        NodePtr prev = nullptr;
        NodePtr cur  = Derived::firstNode(m_pRoot);
        NodePtr end  = Derived::endNode(m_pRoot);
        while (cur != end) {
            if (m_comp(value, cur->getValue())) break;
            prev = cur;
            cur  = cur->m_pNext;
        }
        return prev;
    }
public:
    ListBase() { Derived::init(m_pRoot, m_pTail); }
    ListBase(const ListBase& another) { Derived::init(m_pRoot, m_pTail); *this = another; } // copia profunda de la lista enlazada
    ListBase& operator=(const ListBase& other) {
        if (this == &other) return *this;
        clear();
        other.ApplyFunction([this](Node& n){ push_back(n.getValue(), n.getRef()); });
        return *this;
    }
    ListBase(std::initializer_list<std::pair<value_type, Ref>> values) {
        Derived::init(m_pRoot, m_pTail);
        for (const auto &v : values) push_back(v.first, v.second);
    }

    void destroyAll() {
        NodePtr cur = Derived::firstNode(m_pRoot);
        NodePtr end = Derived::endNode(m_pRoot);
        while (cur != end) {
            NodePtr nx = cur->m_pNext;
            delete cur;
            cur = nx;
        }
        delete end;
        m_pRoot = m_pTail = nullptr;
    }

    void clear(){
        std::scoped_lock lock(m_mutex);
        destroyAll();
        Derived::init(m_pRoot, m_pTail);
    }

    virtual ~ListBase(){ destroyAll(); }

    void push_back(const value_type& value, Ref ref){
        std::scoped_lock lock(m_mutex);
        Derived::append(m_pRoot, m_pTail, new Node(value, ref, nullptr));
    }
    
    bool empty() const { return Derived::isEmpty(m_pRoot); }
    void insert(const value_type& value, Ref ref) {
        std::scoped_lock lock(m_mutex);
        NodePtr n = new Node(value, ref, nullptr);
        NodePtr prev = findInsertPos(value);

        if (prev){
            Derived::insertAfter(prev, n, m_pTail);
        } else if (Derived::isEmpty(m_pRoot)){
            Derived::append(m_pRoot, m_pTail, n);
        } else {
            Derived::prepend(m_pRoot, m_pTail, n);
        }
    }

    std::ostream& write(std::ostream& os) { return os << *this; }
    std::istream& read(std::istream& is) { return is >> *this; }

    friend std::ostream& operator <<(std::ostream& os, const ListBase& list) {
        bool first = true;
        os << "[";
        list.ApplyFunction([&](Node& n){
            if (!first)
                os << ",";
            os << n;
            first = false;
        });
        return os << "]";
    }
    
    friend std::istream &operator >>(std::istream &is, ListBase& list) {
        Delim d;
        Node node;
        list.clear();

        is >> d;
        if (is >> d && d != ']'){
            is.unget();
            while(is >> node >> d){
                list.push_back(node.getValue(), node.getRef());
                if (d == ']') break;
            }    
        }

        return is; 
    }
    
    // Iterators
    ForwardIterator begin() const { return ForwardIterator(Derived::firstNode(m_pRoot)); }
    ForwardIterator end() const { return ForwardIterator(Derived::endNode(m_pRoot)); }

    template <typename Func, typename... Args>
    void ApplyFunction(Func func, Args&&... args) const {
        call(func, std::forward<Args>(args)...);
    }

    template <typename Func, typename... Args>
    Node& FirstThat(Func func, Args&&... args) const {
        return call(func, std::forward<Args>(args)...);
    }

    template<typename Func, typename... Args>
    decltype(auto) call(Func func, Args&&... args) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return ::call(begin(), end(), func, std::forward<Args>(args)...);
    }
};

template <typename Derived, typename Traits>
class CircularListBase : public ListBase<Derived, Traits> {
    using Parent = ListBase<Derived, Traits>;
public:
    using value_type = typename Parent::value_type;
    using Node       = typename Traits::Node;
    using NodePtr    = typename Traits::NodePtr;
    using Parent::Parent;

    static void init(NodePtr& root, NodePtr& tail) {
        root = new Node();
        Derived::linkSelf(root);
        tail = root;
    }

    static NodePtr firstNode(NodePtr root) { return root->m_pNext; }
    static NodePtr endNode(NodePtr root) { return root; }
    static bool isEmpty(NodePtr root) { return root->m_pNext == root; }

    static void prepend(NodePtr& root, NodePtr& tail, NodePtr n) {
        Derived::insertAfter(root, n, tail);
    }

    CircularListBase() : Parent() {}
    CircularListBase(const CircularListBase& other) : Parent(other) {}
    CircularListBase(std::initializer_list<std::pair<value_type, Ref>> values) : Parent(values) {}
};

template <typename Derived, typename Traits>
class ReverseExt {
    const Derived& self() const { return static_cast<const Derived&>(*this); }
public:
    auto rbegin() const { return typename Traits::BackwardIterator(self().GetTail()); }
    auto rend() const { return typename Traits::BackwardIterator(Derived::endNode(self().GetRoot())); }
};

template <typename Traits>
class LinkedListBase : public ListBase<LinkedListBase<Traits>, Traits> {
    static_assert(!HasBackwardIterator<Traits>);
    using NodePtr = typename Traits::NodePtr;
public:
    using ListBase<LinkedListBase<Traits>, Traits>::ListBase;
    static void linkAfter(NodePtr prev, NodePtr n){
        n->m_pNext = prev->m_pNext;
        prev->m_pNext = n;
    }
    static void linkBefore(NodePtr next, NodePtr n){ n->m_pNext = next; }
};

template <typename Traits>
class CircularLinkedListBase : public CircularListBase<CircularLinkedListBase<Traits>, Traits>{
    static_assert(!HasBackwardIterator<Traits>);
    using NodePtr = typename Traits::NodePtr;
public:
    using CircularListBase<CircularLinkedListBase<Traits>, Traits>::CircularListBase;
    static void linkSelf(NodePtr n){ n->m_pNext = n; }
    static void linkAfter(NodePtr prev, NodePtr n){
        n->m_pNext = prev->m_pNext;
        prev->m_pNext = n;
    }
};

template <typename Traits>
class DoubleLinkedListBase : public ListBase<DoubleLinkedListBase<Traits>, Traits>, public ReverseExt<DoubleLinkedListBase<Traits>, Traits> {
    static_assert(HasBackwardIterator<Traits>);
    using NodePtr = typename Traits::NodePtr;
public:
    using ListBase<DoubleLinkedListBase<Traits>, Traits>::ListBase;
    static void linkAfter(NodePtr prev, NodePtr n){
        n->m_pNext = prev->m_pNext;
        n->m_pPrev = prev;
        if (prev->m_pNext) prev->m_pNext->m_pPrev = n;
        prev->m_pNext = n;
    }

    static void linkBefore(NodePtr next, NodePtr n){
        n->m_pNext = next;
        n->m_pPrev = nullptr;
        next->m_pPrev = n;
    }
};

template <typename Traits>
class CircularDoubleLinkedListBase : public CircularListBase<CircularDoubleLinkedListBase<Traits>, Traits>, public ReverseExt<CircularDoubleLinkedListBase<Traits>, Traits> {
    static_assert(HasBackwardIterator<Traits>);
    using NodePtr = typename Traits::NodePtr;
public:
    using CircularListBase<CircularDoubleLinkedListBase<Traits>, Traits>::CircularListBase;
    static void linkSelf(NodePtr n){
        n->m_pNext = n;
        n->m_pPrev = n;
    }
    
    static void linkAfter(NodePtr prev, NodePtr n){
        n->m_pNext = prev->m_pNext;
        n->m_pPrev = prev;
        prev->m_pNext->m_pPrev = n;
        prev->m_pNext = n;
    }
};

template<typename T> using LinkedAscList = LinkedListBase<LinkedListAscTraits<T>>;
template<typename T> using LinkedDescList = LinkedListBase<LinkedListDescTraits<T>>;
template<typename T> using CircularLinkedAscList = CircularLinkedListBase<LinkedListAscTraits<T>>;
template<typename T> using CircularLinkedDescList = CircularLinkedListBase<LinkedListDescTraits<T>>;
template<typename T> using DoubleLinkedAscList = DoubleLinkedListBase<DoubleLinkedListAscTraits<T>>;
template<typename T> using DoubleLinkedDescList = DoubleLinkedListBase<DoubleLinkedListDescTraits<T>>;
template<typename T> using CircularDoubleLinkedAscList = CircularDoubleLinkedListBase<DoubleLinkedListAscTraits<T>>;
template<typename T> using CircularDoubleLinkedDescList = CircularDoubleLinkedListBase<DoubleLinkedListDescTraits<T>>;

#endif // __LINKEDLIST_H__
