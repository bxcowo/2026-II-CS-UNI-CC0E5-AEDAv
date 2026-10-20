#pragma once

#include <concepts>
#include <utility>
#include <fstream>
#include <mutex>
#include <type_traits>
#include <stdexcept>

#include "GeneralNode.h"

template <typename Container>
concept ForwardTraversable = requires(const Container& container){
    container.begin();
    container.end();
    { container.begin() != container.end() } -> std::convertible_to<bool>;
    { ++std::declval<decltype(container.begin())&>()};
    { *std::declval<decltype(container.begin())&>()};
};

template <typename Container>
concept BackwardTraversable = requires(const Container& container){
    container.rbegin();
    container.rend();
    { container.rbegin() != container.rend() } -> std::convertible_to<bool>;
    { ++std::declval<decltype(container.rbegin())&>()};
    { *std::declval<decltype(container.rbegin())&>()};
};

template <typename Derived>
class Call{
protected:
    template <typename Iterator, typename Func, typename... Args>
    static decltype(auto) invoke(Iterator begin, Iterator end, Func&& func, Args&&... args) {
        if constexpr(std::is_void_v<std::invoke_result_t<Func, decltype(*begin), Args...>>) {
            for (auto iter = begin; iter != end; ++iter)
                func(*iter, args...);
        } else {
            for (auto iter = begin; iter != end; ++iter) {
                auto ret = func(*iter, args...);
                if(ret) return *iter;
            }
            throw std::runtime_error("call(): ningun elemento satisface el predicado");
        }
    }

    template <typename Func, typename... Args> requires ForwardTraversable<Derived>
    static decltype(auto) traverse(const Derived& self, Func&& func, Args&&... args){
        auto lock = self.acquireLock();
        return invoke(self.begin(), self.end(), std::forward<Func>(func), std::forward<Args>(args)...);
    }

    template <typename Func, typename... Args> requires BackwardTraversable<Derived>
    static decltype(auto) rtraverse(const Derived& self, Func&& func, Args&&... args){
        auto lock = self.acquireLock();
        return invoke(self.rbegin(), self.rend(), std::forward<Func>(func), std::forward<Args>(args)...);
    }
};

template <typename Derived>
class ForEachExt : private Call<Derived> {
    const Derived& self() const { return static_cast<const Derived&>(*this); }

public:
    using Parent = Call<Derived>;

    template <typename Func, typename... Args> requires ForwardTraversable<Derived>
    decltype(auto) call(Func&& func, Args&&... args) const {
        return Parent::traverse(self(), std::forward<Func>(func), std::forward<Args>(args)...);
    }

    template <typename Func, typename... Args> requires BackwardTraversable<Derived>
    decltype(auto) rcall(Func&& func, Args&&... args) const {
        return Parent::rtraverse(self(), std::forward<Func>(func), std::forward<Args>(args)...);
    }

    template <typename Func, typename... Args> requires ForwardTraversable<Derived>
    void ApplyFunction(Func&& func, Args&&... args) const{
        using result_type = std::invoke_result_t<Func, decltype(*self().begin()), Args...>;
        static_assert(std::is_void_v<result_type>, "ApplyFunction requiere de una función que devuelva void");
        call(std::forward<Func>(func), std::forward<Args>(args)...);
    }
};

template <typename Derived>
class FirstThatExt : private Call<Derived> {
    const Derived& self() const { return static_cast<const Derived&>(*this); }

public:
    using Parent = Call<Derived>;

    template <typename Func, typename... Args> requires ForwardTraversable<Derived>
    decltype(auto) FirstThat(Func&& func, Args&&... args) const {
        using result_type = std::invoke_result_t<Func, decltype(*self().begin()), Args...>;
        static_assert(!std::is_void_v<result_type>, "FirstThat requiere de una función que devuelva un valor");
        return Parent::traverse(self(), std::forward<Func>(func), std::forward<Args>(args)...);
    }

    template <typename Func, typename... Args> requires BackwardTraversable<Derived>
    decltype(auto) FirstThatReverse(Func&& func, Args&&... args) const {
        using result_type = std::invoke_result_t<Func, decltype(*self().rbegin()), Args...>;
        static_assert(!std::is_void_v<result_type>, "FirstThat requiere de una función que devuelva un valor");
        return Parent::rtraverse(self(), std::forward<Func>(func), std::forward<Args>(args)...);
    }
};

template <typename Derived, typename Traits>
class PersistBase : private Call<Derived> {
    const Derived& self() const { return static_cast<const Derived&>(*this); }
    Derived&       self()       { return static_cast<Derived&>(*this); }
public:
    using value_type = typename Traits::value_type;
    using Delim      = typename Traits::Node::Delim;
    using Node       = typename Traits::Node;
protected:
    virtual void persistAppend(const value_type& value, Ref ref) = 0;
public:
    using Parent = Call<Derived>;

    static constexpr Delim Open      = Delim('[');
    static constexpr Delim Separator = Delim(',');
    static constexpr Delim Close     = Delim(']');

    std::ostream& write(std::ostream& os) const requires ForwardTraversable<Derived> {
        bool first = true;
        os << Open;
        Parent::traverse(self(), [&](const Node& node){
            if (!first) os << Separator;
            os << node;
            first = false;
        });
        return os << Close;
    }

    std::istream& read(std::istream& is) {
        Delim d;
        GeneralNode<value_type> node;
        
        self().clear();
        is >> d;

        if (!is || d != Open){
            is.setstate(std::ios::failbit);
            return is;   
        }

        int next_val = (is >> std::ws).peek();
        if (Delim(static_cast<char>(next_val)) == Close) {
            is >> d;
            return is;
        }

        while(is >> node >> d){
            persistAppend(node.getValue(), node.getRef());
            if (d == Close) break;
            if (d != Separator){
                is.setstate(std::ios::failbit);
                break;
            }
        }
        return is;
    }

    virtual ~PersistBase() = default;

    friend std::ostream& operator<<(std::ostream& os, const PersistBase& p) requires ForwardTraversable<Derived>{
        return p.self().write(os);
    }

    friend std::istream& operator>>(std::istream& is, PersistBase& p) {
        return p.self().read(is);
    }
};

template <typename Derived, typename Traits>
class PersistExt : public PersistBase<Derived, Traits> {
    using Base = PersistBase<Derived, Traits>;

protected:
    using typename Base::value_type;

    void persistAppend(const value_type& value, Ref ref) override {
        static_cast<Derived&>(*this).push_back(value, ref);
    }
};

template <typename Derived, typename Traits>
class PersistTreeExt : public PersistBase<Derived, Traits> {
    using Base = PersistBase<Derived, Traits>;

protected:
    using typename Base::value_type;

    void persistAppend(const value_type& value, Ref ref) override {
        static_cast<Derived&>(*this).push(value, ref);
    }
};




