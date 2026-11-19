/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <memory>
#include <type_traits>
#include <utility>
#include <variant>

namespace Dynarmic::Common {

/// Holds a T on the heap with value semantics, so that a std::variant can hold a type that is
/// incomplete where the variant is declared (a type that itself contains that variant).
template<typename T>
class RecursiveWrapper {
public:
    RecursiveWrapper(const T& value)
            : ptr(std::make_unique<T>(value)) {}
    RecursiveWrapper(T&& value)
            : ptr(std::make_unique<T>(std::move(value))) {}

    RecursiveWrapper(const RecursiveWrapper& other)
            : ptr(std::make_unique<T>(*other.ptr)) {}
    // Copies rather than steals, so that a moved-from wrapper still holds a value.
    RecursiveWrapper(RecursiveWrapper&& other)
            : ptr(std::make_unique<T>(std::move(*other.ptr))) {}

    RecursiveWrapper& operator=(const RecursiveWrapper& other) {
        *ptr = *other.ptr;
        return *this;
    }
    RecursiveWrapper& operator=(RecursiveWrapper&& other) noexcept {
        std::swap(ptr, other.ptr);
        return *this;
    }

    ~RecursiveWrapper() = default;

    T& get() { return *ptr; }
    const T& get() const { return *ptr; }

private:
    std::unique_ptr<T> ptr;
};

namespace detail {

template<typename T>
struct IsRecursiveWrapper : std::false_type {};
template<typename T>
struct IsRecursiveWrapper<RecursiveWrapper<T>> : std::true_type {};

template<typename T>
decltype(auto) Unwrap(T& x) {
    if constexpr (IsRecursiveWrapper<std::remove_const_t<T>>::value) {
        return x.get();
    } else {
        return x;
    }
}

}  // namespace detail

/// Visits a std::variant, presenting any RecursiveWrapper<T> alternative to `lambda` as the T it holds.
template<typename ReturnT, typename Variant, typename Lambda>
inline ReturnT VisitVariant(Variant&& variant, Lambda&& lambda) {
    return std::visit([&](auto& alternative) -> ReturnT { return lambda(detail::Unwrap(alternative)); }, variant);
}

}  // namespace Dynarmic::Common
