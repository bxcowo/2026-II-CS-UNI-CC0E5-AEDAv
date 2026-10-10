#ifndef __FOREACH_H__
#define __FOREACH_H__
#include <utility> // std::forward
#include <type_traits> // std::is_void_v, std::invoke_result_t
#include <stdexcept> // std::runtime_error
using namespace std;

template <typename Iterator, typename Func, typename... Args>
decltype(auto) call(Iterator begin, Iterator end, Func func, Args&&... args) {
    if constexpr(is_void_v<invoke_result_t<Func, decltype(*begin), Args...>>) {
        for (auto iter = begin; iter != end; ++iter)
            func(*iter, std::forward<Args>(args)...);
    } else {
        for (auto iter = begin; iter != end; ++iter) {
            auto ret = func(*iter, std::forward<Args>(args)...);
            if(ret) return *iter;
        }
        throw std::runtime_error("call(): ningun elemento satisface el predicado");
    }
}

// Variadic template to allow passing additional arguments to the function
// template <typename Container, typename Func, typename... Args>
// Iterator Level #1
// void ApplyFunction(Container &container, Func func, Args... args) {
//     for (size_t i = 0; i < container.size(); ++i) {
//         func(container[i], args...);
//     }
// }

// Iterator Level #2
// template <typename Container, typename Func, typename... Args>
// void ApplyFunction(Container &container, Func func, Args... args) {
//     for (auto iter = container.begin(); iter != container.end(); ++iter) {
//         func(*iter, args...);
//     }
// }

// Iterator Level #3
template <typename Iterator, typename Func, typename... Args>
void ApplyFunction(Iterator begin, Iterator end, Func func, Args&&... args) {
    // for (auto iter = begin; iter != end; ++iter)
    //     func(*iter, std::forward<Args>(args)...);
    ::call(begin, end, func, std::forward<Args>(args)...);
}

template <typename Iterator, typename Func, typename... Args>
decltype(auto) FirstThat(Iterator begin, Iterator end, Func func, Args&&... args) {
    // for (auto iter = begin; iter != end; ++iter)
    //     func(*iter, std::forward<Args>(args)...);
    return ::call(begin, end, func, std::forward<Args>(args)...);
}

// // Iterator Level #3
// template <typename Container, typename Func, typename... Args>
// void ApplyFunction(Container &container, Func func, Args... args) {
//     ::ApplyFunction(container.begin(), container.end(), func, args...);
// }

// Iterator Level #4
template <typename Container, typename Func, typename... Args>
void ApplyFunction(Container &container, Func func, Args&&... args) {
    for (auto &v : container)
        func(v, std::forward<Args>(args)...);
}

#endif // __FOREACH_H__