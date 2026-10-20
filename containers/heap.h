#ifndef __HEAP_H__
#define __HEAP_H__

#include <functional>
#include <initializer_list>
#include <mutex>
#include <stdexcept>
#include <utility>

#include "vector.h"
#include "call.h"

template <typename T, typename Comp>
struct HeapTraits {
    using value_type = T;
    using Compare    = Comp;
};

template <typename Traits>
using HeapContainerTraits = VectorTraits<typename Traits::value_type>;

template <typename Traits>
class Heap : public ForEachExt<Heap<Traits>>, public FirstThatExt<Heap<Traits>>, public PersistTreeExt<Heap<Traits>, HeapContainerTraits<Traits>> {
private:
    using Container = Vector<HeapContainerTraits<Traits>>;
public:
    using value_type       = typename Traits::value_type;
    using Compare          = typename Traits::Compare;
    using Node             = typename Container::Node;
    using NodePtr          = Node*;
    using ForwardIterator  = typename Container::ForwardIterator;
    using BackwardIterator = typename Container::BackwardIterator;
private:
    Container          m_data;
    Compare            m_comp;
    mutable std::mutex m_mutex;

    bool internal_compare(const Node& a, const Node& b){
        return m_comp(a.getValue(), b.getValue());
    }

    void HeapifyUp(size_t index) {
        while (index > 0) {
            size_t parentIndex = (index - 1) / 2;
            if (!internal_compare(m_data[index], m_data[parentIndex])) break;
            std::swap(m_data[index], m_data[parentIndex]);
            index = parentIndex;
        }
    }

    void HeapifyDown(size_t index){
        const size_t n = m_data.size();
        size_t target, left, right;
        while(true){
            target = index;
            left = 2 * index + 1;
            right = left + 1;
            if (left < n && internal_compare(m_data[left], m_data[target])) target = left;
            if (right < n && internal_compare(m_data[right], m_data[target])) target = right;
            if (target == index) break;
            std::swap(m_data[index], m_data[target]);
            index = target;
        }
    }
public:
    Heap() = default;

    Heap(std::initializer_list<std::pair<value_type, Ref>> values, Compare comp = Compare()) : m_comp(std::move(comp)) {
        for (const auto& [value, ref] : values) 
            m_data.push_back(value, ref);
        build_heap();
    }

    Heap(const Heap& other) {
        std::lock_guard<std::mutex> lock(other.m_mutex);
        m_comp = other.m_comp; 
        m_data = Container(other.m_data);
    }

    Heap(Heap&& other) {
        std::lock_guard<std::mutex> lock(other.m_mutex);
        m_comp = std::move(other.m_comp);
        m_data = std::move(other.m_data);
    }

    Heap& operator=(const Heap& other) {
        if (this != &other) {
            std::scoped_lock lock(m_mutex, other.m_mutex);
            m_data = Container(other.m_data);
            m_comp = other.m_comp;
        }
        return *this;
    }

    Heap& operator=(Heap&& other) {
        if (this != &other) {
            std::scoped_lock lock(m_mutex, other.m_mutex);
            m_data = std::move(other.m_data);
            m_comp = std::move(other.m_comp);
        }
        return *this;
    }

    std::unique_lock<std::mutex> acquireLock() const {
        return std::unique_lock<std::mutex>(m_mutex);
    }
    
    ForwardIterator begin()    const { return m_data.begin(); }
    ForwardIterator end()      const { return m_data.end();   }
    BackwardIterator rbegin()  const { return m_data.rbegin(); }
    BackwardIterator rend()    const { return m_data.rend();   }

    void build_heap() {
        for (size_t i = m_data.size() / 2; i-- > 0; )
            HeapifyDown(i);
    }

    void push(const value_type &value, Ref ref) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_data.push_back(value, ref);
        HeapifyUp(m_data.size() - 1);
    }
    
    
    Node pop() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_data.empty()) throw std::out_of_range("pop(): heap vacio");
        Node result = m_data[0];
        std::swap(m_data[0], m_data[m_data.size() - 1]);
        m_data.pop_back();
        HeapifyDown(0);
        return result;
    }

    Node top() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_data.empty()) throw std::out_of_range("top(): heap vacio");
        return *m_data.begin();
    }

    size_t  size() const { return m_data.size(); }
    bool   empty() const { return m_data.empty(); }

    void clear(){
        std::lock_guard<std::mutex> lock(m_mutex);
        m_data.clear();
    }
};

template <typename T>
using MinHeap = Heap<HeapTraits<T, std::less<T>>>;
template <typename T>
using MaxHeap = Heap<HeapTraits<T, std::greater<T>>>;

#endif // __HEAP_H__