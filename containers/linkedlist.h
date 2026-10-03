#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__
#include <mutex>
#include <initializer_list>
#include "GeneralNode.h"
#include "GeneralIterator.h"
#include "../foreach.h"
#include "../types.h"

template <typename T>
class LinkedListNode : public GeneralNode<T> {
    using Node    = LinkedListNode<T>;
    using NodePtr = Node *;
public:
    NodePtr m_pNext = nullptr; // puntero al siguiente nodo
    LinkedListNode() : GeneralNode<T>(T{}, Ref{}), m_pNext(nullptr) {}
    LinkedListNode(const T& value, Ref ref, NodePtr pNext) : GeneralNode<T>(value, ref), m_pNext(pNext){}
    // TODO: El operator<< deberia ir en GeneralNode, no en LinkedListNode, para que sea generico y reusable.
    /*
    friend std::ostream &operator <<(std::ostream &os, const LinkedListNode<T> &node) {
        os << "(" << node.getValue() << "," << node.getRef() << ")";
        return os;
    }
    */
};

template <typename T>
class LinkedListForwardIterator : public GeneralIterator<LinkedListForwardIterator<T>, LinkedListNode<T>> {
public:
    using value_type        = LinkedListNode<T>;
    using MySelf            = LinkedListForwardIterator<T>;
    using Parent            = GeneralIterator<MySelf, value_type>;
    using Parent::Parent; // Inherit constructor
    MySelf& operator++() { Parent::m_ptr = Parent::m_ptr->m_pNext; return *this; }
};

template <typename T>
struct LinkedListAscTraits {
    using value_type        = T;
    using Node              = LinkedListNode<T>;
    using ForwardIterator   = LinkedListForwardIterator<T>;  // itera sobre Node, no sobre T
};

template <typename Traits>
class LinkedList {
public:
    using value_type        = typename Traits::value_type;
    using Node              = typename Traits::Node;
    using NodePtr           = Node *;
    using ForwardIterator   = typename Traits::ForwardIterator;
    using Delim             = typename Node::Delim;
private:
    NodePtr m_pRoot = nullptr; // puntero al primer nodo de la lista enlazada
    NodePtr m_pTail = nullptr; // puntero al último nodo de la lista enlazada
    // TODO: agregar mutex para sincronización de acceso concurrente
    mutable std::mutex m_mutex;             // mutex para sincronización

    NodePtr NodePtrGetRoot() const { return m_pRoot; }
    void internalInsert(const value_type& value, Ref ref, NodePtr& rParent);
    void swap(LinkedList& otro) noexcept;
public:
    LinkedList() {}
    // TODO: implementar constructor copia de LinkedList para crear nodos nuevos
    LinkedList(const LinkedList& otro); // [Hecho]
    // TODO: implementar LinkedList con nodos enlazados y métodos push_back.
    LinkedList& operator=(const LinkedList& otro); // [Hecho]

    // TODO: implementar la destruccion en un metodo clear()
    void clear(); // [Hecho]
    // TODO: implementar destructor para liberar memoria de forma segura
    virtual ~LinkedList() { clear(); }; // [Hecho]

    // TODO: implementar push_back
    void push_back(const value_type& value, Ref ref);

    // TODO: implementar insert() para LinkedList // [Hecho]
    void insert(const value_type& value, Ref ref){
        std::lock_guard<std::mutex> lock(this->m_mutex);
        internalInsert(value, ref, m_pRoot);
    } 
    
    // TODO: persistencia: write() y read() para LinkedList
    std::ostream &write(std::ostream &os) { return os << *this; }
    std::istream &read(std::istream &is)  { return is >> *this; }

    friend std::ostream &operator <<(std::ostream &os, const LinkedList<Traits> &list) {
        std::lock_guard<std::mutex> lock(list.m_mutex);
        NodePtr current = list.m_pRoot;
        os << "[";
        while (current != nullptr) {
            os << *current;
            current = current->m_pNext;
            if (current != nullptr) os << ",";
        }
        return os << "]";
    }

    // TODO: implementar
    friend std::istream &operator >>(std::istream &is, LinkedList<Traits> &list) {
        Delim d;
        Node node;
        LinkedList temp;

        is >> d;
        if (is >> d && d != ']'){
            is.unget();
            while(is >> node >> d){
                temp.push_back(node.getValue(), node.getRef());
                if (d == ']'){
                    break;
                }
            }    
        }

        std::lock_guard<std::mutex> lock(list.m_mutex);
        list.swap(temp);
        return is; 
    }

    // Iterators
    ForwardIterator begin() { return ForwardIterator(m_pRoot); }
    ForwardIterator end()   { return ForwardIterator(nullptr); }

    // TODO: implementar ApplyFunction(), FirstThat(), call, rcall para LinkedList
    // Chequear que hago para evitar codigo repetido
    template<typename Func, typename... Args>
    decltype(auto) call(Func func, Args&&... args){
        std::lock_guard<std::mutex> lock(this->m_mutex);
        return ::call(begin(), end(), std::forward<Func>(func), std::forward<Args>(args)...);
    }

    template <typename Func, typename... Args>
    void ApplyFunction(Func func, Args... args){
        call(func, std::forward<Args>(args)...);
    }

    template <typename Func, typename... Args>
    Node& FirstThat(Func func, Args... args){
        return call(func, std::forward<Args>(args)...);
    }
};


template <typename Traits>
void LinkedList<Traits>::swap(LinkedList& otro) noexcept{
    std::swap(this->m_pRoot, otro.m_pRoot);
    std::swap(this->m_pTail, otro.m_pTail);
}

template <typename Traits>
LinkedList<Traits>& LinkedList<Traits>::operator=(const LinkedList<Traits>& otro){ 
    if (this == &otro){
        return *this;
    }

    LinkedList temp(otro);
    std::lock_guard<std::mutex> lock(this->m_mutex);
    swap(temp);

    return *this;
}

template <typename Traits>
LinkedList<Traits>::LinkedList(const LinkedList& otro){
    std::lock_guard<std::mutex> lock(otro.m_mutex);
    
    if(!otro.m_pRoot){
        return;
    }

    this->m_pRoot = new Node(otro.m_pRoot->getValue(), otro.m_pRoot->getRef(), nullptr);

    NodePtr next = otro.m_pRoot->m_pNext;
    NodePtr curr = this->m_pRoot;

    while(next){
        curr->m_pNext = new Node(next->getValue(), next->getRef(), nullptr);
        curr = curr->m_pNext;
        next = next->m_pNext;
    }

    this->m_pTail = curr;
}

// TODO: explicar recursividad de cola de llamadas en insert() y internalInsert() [Hecho]
template<typename Traits>
void LinkedList<Traits>::internalInsert(const value_type& value, Ref ref, NodePtr &rParent){
    if (rParent == nullptr || value < rParent->getValue() ) {
        rParent = new Node(value, ref, rParent);
        if (rParent -> m_pNext == nullptr){
            m_pTail = rParent;
        }
        return;
    } 
    internalInsert(value, ref, rParent->m_pNext);
}

template<typename Traits>
void LinkedList<Traits>::push_back(const value_type& value, Ref ref){
    std::lock_guard<std::mutex> lock(this->m_mutex);
    NodePtr new_node = new Node(value, ref, nullptr);
    if (!this->m_pRoot){
        this->m_pRoot = new_node;
        this->m_pTail = new_node;
    } else {
        this->m_pTail->m_pNext = new_node;
        this->m_pTail = new_node;
    }
}

// Implementación de la función clear 
template<typename Traits>
void LinkedList<Traits>::clear(){
    std::lock_guard<std::mutex> lock (this->m_mutex);
    NodePtr current = m_pRoot;
    while (current) {
        NodePtr next = current->m_pNext;
        delete current;
        current = next;
    }
    m_pRoot = nullptr;
    m_pTail = nullptr;
}

#endif // __LINKEDLIST_H__
