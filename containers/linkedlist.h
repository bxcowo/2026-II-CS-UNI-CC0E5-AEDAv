#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__
#include <mutex>
#include "GeneralNode.h"
#include "GeneralIterator.h"
#include "../foreach.h"

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

template <typename NodeT>
class LinkedListForwardIterator : public GeneralIterator<LinkedListForwardIterator<NodeT>, NodeT> {
public:
    using value_type = NodeT;
    using MySelf     = LinkedListForwardIterator<NodeT>;
    using Parent     = GeneralIterator<MySelf, value_type>;
    using Parent::Parent;

    MySelf& operator++() { Parent::m_ptr = Parent::m_ptr->m_pNext; return *this; }
};

template <typename NodeT>
class LinkedListBidirectionalIterator : public GeneralIterator<LinkedListBidirectionalIterator<NodeT>, NodeT> {
public:
    using value_type = NodeT;
    using MySelf     = LinkedListBidirectionalIterator<NodeT>;
    using Parent     = GeneralIterator<MySelf, value_type>;
    using Parent::Parent;

    MySelf& operator++() { Parent::m_ptr = Parent::m_ptr->m_pNext; return *this; }
    MySelf& operator--() { Parent::m_ptr = Parent::m_ptr->m_pPrev; return *this; }
};

template <typename T, typename NodeT, typename _Compare, typename IterT>
struct DefaultTraits {
    using value_type = T;
    using Node       = NodeT;
    using NodePtr    = Node*;
    using Iter       = IterT;
    using Compare    = _Compare;

    static void    init(NodePtr&, NodePtr&) {}
    static NodePtr makeNode(const value_type& value, Ref ref) { return new Node(value, ref, nullptr); }
    static void    append(NodePtr&, NodePtr&, NodePtr) {}
    static void    prepend(NodePtr&, NodePtr&, NodePtr) {}
    static void    insertAfter(NodePtr, NodePtr, NodePtr&) {}
    static void    destroyAll(NodePtr&, NodePtr&) {}
    static NodePtr firstNode(NodePtr root) { return root; }
    static NodePtr endNode(NodePtr) { return nullptr; }
    static bool    isEmpty(NodePtr root) { return root == nullptr; }
};

template <typename T, typename _Compare>
struct LinkedListTraits : DefaultTraits<T, SingleLinkedListNode<T>, _Compare, LinkedListForwardIterator<SingleLinkedListNode<T>>> {
    using Base    = DefaultTraits<T, SingleLinkedListNode<T>, _Compare, LinkedListForwardIterator<SingleLinkedListNode<T>>>;
    using NodePtr = typename Base::NodePtr;
    
    static void append(NodePtr& root, NodePtr& tail, NodePtr n){
        if (!root){
            root = tail = n;
        } else {
            tail->m_pNext = n;
            tail = n;
        }
    }

    static void prepend(NodePtr& root, NodePtr&, NodePtr n){
        n->m_pNext = root;
        root = n;
    }

    static void insertAfter(NodePtr prev, NodePtr n, NodePtr& tail){
        n->m_pNext = prev->m_pNext;
        prev->m_pNext = n;
        if (prev == tail) tail = n;
    }

    static void destroyAll(NodePtr& root, NodePtr& tail) {
        NodePtr cur = root;
        while (cur) {
            NodePtr next = cur->m_pNext;
            delete cur;
            cur = next;
        }
        root = nullptr;
        tail = nullptr;
    }
};

template <typename T, typename _Compare>
struct CircularLinkedListTraits : DefaultTraits<T, SingleLinkedListNode<T>, _Compare, LinkedListForwardIterator<SingleLinkedListNode<T>>> {
    using Base    = DefaultTraits<T, SingleLinkedListNode<T>, _Compare, LinkedListForwardIterator<SingleLinkedListNode<T>>>;
    using Node    = typename Base::Node;
    using NodePtr = typename Base::NodePtr;

    static void init(NodePtr& root, NodePtr& tail) {
        root = new Node(); // Nodo centinela
        root->m_pNext = root;
        tail = root;
    }

    static NodePtr firstNode(NodePtr root) { return root->m_pNext; }
    static NodePtr endNode(NodePtr root) { return root; }
    static bool isEmpty(NodePtr root) { return root->m_pNext == root; }

    static void append(NodePtr& root, NodePtr& tail, NodePtr n){
        n->m_pNext = root;
        tail->m_pNext = n;
        tail = n;
    }

    static void insertAfter(NodePtr prev, NodePtr n, NodePtr& tail){
        n->m_pNext = prev->m_pNext;
        prev->m_pNext = n;
        if (prev == tail) tail = n;
    }

    static void prepend(NodePtr& root, NodePtr& tail, NodePtr n){
        insertAfter(root, n, tail);
    }

    static void destroyAll(NodePtr& root, NodePtr& tail){
        if(root){
            NodePtr cur = root->m_pNext;
            while(cur != root){
                NodePtr next = cur->m_pNext;
                delete cur;
                cur = next;
            }
            delete root;
        }
        root = nullptr;
        tail = nullptr;
    }
};

template <typename T, typename _Compare>
struct DoubleLinkedListTraits : DefaultTraits<T, DoubleLinkedListNode<T>, _Compare, LinkedListBidirectionalIterator<DoubleLinkedListNode<T>>> {
    using Base    = DefaultTraits<T, DoubleLinkedListNode<T>, _Compare, LinkedListBidirectionalIterator<DoubleLinkedListNode<T>>>;
    using NodePtr = typename Base::NodePtr;
    
    static void append(NodePtr& root, NodePtr& tail, NodePtr n){
        if (!root){
            root = tail = n;
        } else {
            n->m_pPrev = tail;
            tail->m_pNext = n;
            tail = n;
        }
    }

    static void prepend(NodePtr& root, NodePtr&, NodePtr n){
        n->m_pNext = root;
        n->m_pPrev = nullptr;
        root->m_pPrev = n;
        root = n;
    }

    static void insertAfter(NodePtr prev, NodePtr n, NodePtr& tail){
        n->m_pNext = prev->m_pNext;
        n->m_pPrev = prev;
        if (prev->m_pNext) prev->m_pNext->m_pPrev = n;
        prev->m_pNext = n;
        if (prev == tail) tail = n;
    }

    static void destroyAll(NodePtr& root, NodePtr& tail) {
        NodePtr cur = root;
        while (cur) {
            NodePtr next = cur->m_pNext;
            delete cur;
            cur = next;
        }
        root = nullptr;
        tail = nullptr;
    }
};


template <typename T, typename _Compare>
struct CircularDoubleLinkedListTraits : DefaultTraits<T, DoubleLinkedListNode<T>, _Compare, LinkedListBidirectionalIterator<DoubleLinkedListNode<T>>> {
    using Base    = DefaultTraits<T, DoubleLinkedListNode<T>, _Compare, LinkedListBidirectionalIterator<DoubleLinkedListNode<T>>>;
    using Node    = typename Base::Node;
    using NodePtr = typename Base::NodePtr;

    static void init(NodePtr& root, NodePtr& tail) {
        root = new Node(); // Nodo centinela
        root->m_pNext = root;
        root->m_pPrev = root;
        tail = root;
    }

    static NodePtr firstNode(NodePtr root) { return root->m_pNext; }
    static NodePtr endNode(NodePtr root)   { return root; }
    static bool    isEmpty(NodePtr root)   { return root->m_pNext == root; }

    static void append(NodePtr& root, NodePtr& tail, NodePtr n) {
        n->m_pNext = root;
        n->m_pPrev = root->m_pPrev;
        root->m_pPrev->m_pNext = n;
        root->m_pPrev = n;
        tail = n;
    }

    static void prepend(NodePtr& root, NodePtr& tail, NodePtr n) {
        insertAfter(root, n, tail);
    }

    static void insertAfter(NodePtr prev, NodePtr n, NodePtr& tail) {
        n->m_pNext = prev->m_pNext;
        n->m_pPrev = prev;
        prev->m_pNext->m_pPrev = n;
        prev->m_pNext = n;
        if (prev == tail) tail = n;
    }

    static void destroyAll(NodePtr& root, NodePtr& tail) {
        if (root) {
            NodePtr cur = root->m_pNext;
            while (cur != root) {
                NodePtr next = cur->m_pNext;
                delete cur;
                cur = next;
            }
            delete root;
        }
        root = nullptr;
        tail = nullptr;
    }
};

template <typename T>
struct LinkedListAscTraits : LinkedListTraits<T, std::less<T>> {};

template <typename T>
struct LinkedListDescTraits : LinkedListTraits<T, std::greater<T>> {};

template <typename T>
struct CircularLinkedListAscTraits : CircularLinkedListTraits<T, std::less<T>> {};

template <typename T>
struct CircularLinkedListDescTraits : CircularLinkedListTraits<T, std::greater<T>> {};

template <typename T>
struct DoubleLinkedListAscTraits : DoubleLinkedListTraits<T, std::less<T>> {};

template <typename T>
struct DoubleLinkedListDescTraits : DoubleLinkedListTraits<T, std::greater<T>> {};

template <typename T>
struct CircularDoubleLinkedListAscTraits : CircularDoubleLinkedListTraits<T, std::less<T>> {};

template <typename T>
struct CircularDoubleLinkedListDescTraits : CircularDoubleLinkedListTraits<T, std::greater<T>> {};


template <typename Traits>
class LinkedList {
public:
    using value_type        = typename Traits::value_type;
    using Node              = typename Traits::Node;
    using NodePtr           = typename Traits::NodePtr;
    using Iter              = typename Traits::Iter;
    using Compare           = typename Traits::Compare;
    using Delim             = typename Node::Delim;
private:
    NodePtr m_pRoot = nullptr; // puntero al primer nodo de la lista enlazada
    NodePtr m_pTail = nullptr; // puntero al último nodo de la lista enlazada
    
    Compare m_comp; // comparador para ordenar los nodos de la lista
    mutable std::mutex m_mutex; // mutex para sincronización

    NodePtr GetRoot() const { return m_pRoot; }
    NodePtr findInsertPos(const value_type& value) const {
        NodePtr prev = nullptr;
        NodePtr cur  = Traits::firstNode(m_pRoot);
        NodePtr end  = Traits::endNode(m_pRoot);
        while (cur != end) {
            if (m_comp(value, cur->getValue()))
                break;
            prev = cur;
            cur  = cur->m_pNext;
        }
        return prev;
    }
public:
    // Constructores
    LinkedList() { Traits::init(m_pRoot, m_pTail); }
    LinkedList(const LinkedList& another){ Traits::init(m_pRoot, m_pTail); *this = another; } // copia profunda de la lista enlazada
    LinkedList& operator=(const LinkedList& another);
    LinkedList(std::initializer_list<std::pair<value_type, Ref>> values) {
        Traits::init(m_pRoot, m_pTail);
        for (const auto &v : values)
            push_back(v.first, v.second);
    }

    void clear();
    virtual ~LinkedList(){ Traits::destroyAll(m_pRoot, m_pTail); }
    void push_back(const value_type& value, Ref ref);
    bool empty() const { return Traits::isEmpty(m_pRoot); }
    void insert(const value_type& value, Ref ref);

    std::ostream& write(std::ostream& os) { return os << *this; }
    std::istream& read(std::istream& is) { return is >> *this; }

    friend std::ostream& operator <<(std::ostream& os, const LinkedList<Traits>& list) {
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
    
    friend std::istream &operator >>(std::istream &is, LinkedList<Traits> &list) {
        Delim d;
        Node node;
        list.clear();

        is >> d;
        if (is >> d && d != ']'){
            is.unget();
            while(is >> node >> d){
                list.push_back(node.getValue(), node.getRef());
                if (d == ']'){
                    break;
                }
            }    
        }

        return is; 
    }
    
    // Iterators
    Iter begin() const { return Iter(Traits::firstNode(m_pRoot)); }
    Iter end() const { return Iter(Traits::endNode(m_pRoot)); }

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

template <typename Traits>
LinkedList<Traits>& LinkedList<Traits>::operator=(const LinkedList<Traits>& other){ 
    if (this == &other){
        return *this;
    }

    clear();
    other.ApplyFunction([this](Node& n){
        push_back(n.getValue(), n.getRef());
    });
    return *this;
}

template <typename Traits>
void LinkedList<Traits>::clear(){
    std::scoped_lock lock(m_mutex);
    Traits::destroyAll(m_pRoot, m_pTail);
    Traits::init(m_pRoot, m_pTail);
}

template <typename Traits>
void LinkedList<Traits>::push_back(const value_type& value, Ref ref){
    std::scoped_lock lock(m_mutex);
    NodePtr n = Traits::makeNode(value, ref);
    Traits::append(m_pRoot, m_pTail, n);
}

template <typename Traits>
void LinkedList<Traits>::insert(const value_type& value, Ref ref){
    std::scoped_lock lock(m_mutex);
    NodePtr n = Traits::makeNode(value, ref);
    NodePtr prev = findInsertPos(value);

    if (prev){
        Traits::insertAfter(prev, n, m_pTail);
    } else if (Traits::isEmpty(m_pRoot)){
        Traits::append(m_pRoot, m_pTail, n);
    } else {
        Traits::prepend(m_pRoot, m_pTail, n);
    }
}

#endif // __LINKEDLIST_H__
