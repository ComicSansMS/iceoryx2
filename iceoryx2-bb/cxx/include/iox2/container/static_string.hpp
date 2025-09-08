// Copyright (c) 2025 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache Software License 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0, or the MIT license
// which is available at https://opensource.org/licenses/MIT.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

#ifndef IOX2_INCLUDE_GUARD_CONTAINER_STATIC_STRING_HPP
#define IOX2_INCLUDE_GUARD_CONTAINER_STATIC_STRING_HPP

#include "iox2/container/config.hpp"
#include "iox2/container/optional.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <type_traits>

namespace iox2 {
namespace container {

/// A UTF-8 string with fixed static capacity and contiguous inplace storage.
/// Embedded zeroes (`'\0'`) in the middle of a string are not allowed.
/// @note Currently only Unicode code points less than 128 (0x80) are supported.
/// @tparam N Maximum number of UTF-8 code units that the string can store, excluding the terminating '\0'.
template <uint64_t N>
class StaticString {
  public:
    using ValueType = char;
    using SizeType = size_t;
    using DifferenceType = ptrdiff_t;
    using Reference = char&;
    using ConstReference = char const&;
    using Pointer = char*;
    using ConstPointer = char const*;
    using Iterator = Pointer;
    using ConstIterator = ConstPointer;
    using OptionalReference = Optional<std::reference_wrapper<char>>;
    using OptionalConstReference = Optional<std::reference_wrapper<char const>>;

    // Unchecked element access
    class UncheckedConstAccessor {
        friend class StaticString;

      private:
        StaticString const* m_parent;

        constexpr explicit UncheckedConstAccessor(StaticString const& parent)
            : m_parent(&parent) {
        }

      public:
        ~UncheckedConstAccessor() = default;
        UncheckedConstAccessor(UncheckedConstAccessor const&) = delete;
        UncheckedConstAccessor(UncheckedConstAccessor&&) = delete;
        auto operator=(UncheckedConstAccessor const&) -> UncheckedConstAccessor& = delete;
        auto operator=(UncheckedConstAccessor&&) -> UncheckedConstAccessor& = delete;

        constexpr auto operator[](SizeType index) const -> ConstReference {
            return m_parent->m_string[index];
        }

        constexpr auto begin() const noexcept -> ConstIterator {
            return &(m_parent->m_string[0]);
        }

        constexpr auto end() const noexcept -> ConstIterator {
            return &(m_parent->m_string[m_parent->m_size]);
        }

        constexpr auto data() const noexcept -> ConstPointer {
            return &(m_parent->m_string[0]);
        }

        constexpr auto c_str() const noexcept -> char const* {
            return data();
        }
    };

    class UncheckedAccessor {
        friend class StaticString;

      private:
        StaticString* m_parent;

        constexpr explicit UncheckedAccessor(StaticString& parent)
            : m_parent(&parent) {
        }

      public:
        ~UncheckedAccessor() = default;
        UncheckedAccessor(UncheckedAccessor const&) = delete;
        UncheckedAccessor(UncheckedAccessor&&) = delete;
        auto operator=(UncheckedAccessor const&) -> UncheckedAccessor& = delete;
        auto operator=(UncheckedAccessor&&) -> UncheckedAccessor& = delete;

        constexpr auto operator[](SizeType index) -> Reference {
            return m_parent->m_string[index];
        }

        constexpr auto operator[](SizeType index) const -> ConstReference {
            return m_parent->m_string[index];
        }

        constexpr auto begin() noexcept -> Iterator {
            return &(m_parent->m_string[0]);
        }

        constexpr auto begin() const noexcept -> ConstIterator {
            return &(m_parent->m_string[0]);
        }

        constexpr auto end() noexcept -> Iterator {
            return &(m_parent->m_string[m_parent->m_size]);
        }

        constexpr auto end() const noexcept -> ConstIterator {
            return &(m_parent->m_string[m_parent->m_size]);
        }

        constexpr auto data() noexcept -> Pointer {
            return &(m_parent->m_string[0]);
        }

        constexpr auto data() const noexcept -> ConstPointer {
            return &(m_parent->m_string[0]);
        }

        constexpr auto c_str() const noexcept -> char const* {
            return data();
        }
    };

  private:
    template <uint64_t>
    friend class StaticString;

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays) encapsulated storage
    char m_string[N + 1] = {};
    uint64_t m_size = 0;

  public:
    // constructors
    constexpr StaticString() noexcept = default;
    constexpr StaticString(StaticString const&) = default;
    constexpr StaticString(StaticString&&) = default;

    template <uint64_t M, std::enable_if_t<(N >= M), bool> = true>
    // NOLINTNEXTLINE(hicpp-explicit-conversions), conceptually a copy constructor
    constexpr StaticString(StaticString<M> const& rhs)
        : m_size(rhs.m_size) {
        for (size_t i = 0; i < m_size; ++i) {
            m_string[i] = rhs.m_string[i];
        }
    }

    // destructor
#if __cplusplus >= 202002L
    constexpr
#endif
        ~StaticString() = default;

    auto operator=(StaticString const&) -> StaticString& = default;
    auto operator=(StaticString&&) -> StaticString& = default;

    template <uint64_t M, std::enable_if_t<(N >= (M - 1)), bool> = true>
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays) statically bounds checked
    static auto from_utf8(char const (&utf8_str)[M]) -> Optional<StaticString> {
        if (utf8_str[M - 1] != '\0') {
            return nullopt;
        }
        StaticString ret;
        for (uint64_t i = 0; i < M - 1; ++i) {
            char const character = utf8_str[i];
            if (!ret.try_push_back(character)) {
                return nullopt;
            }
        }
        return ret;
    }

    static auto from_utf8_null_terminated_unchecked(char const* utf8_str) -> Optional<StaticString> {
        StaticString ret;
        while (*utf8_str != '\0') {
            if (!ret.try_push_back(*utf8_str)) {
                return nullopt;
            }
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic), unchecked access into c-style string
            ++utf8_str;
        }
        return ret;
    }

    constexpr auto try_push_back(char character) -> bool {
        if ((m_size < N) && (is_valid_next(character))) {
            m_string[m_size] = character;
            ++m_size;
            return true;
        } else {
            return false;
        }
    }

    constexpr auto try_pop_back() -> bool {
        if (m_size > 0) {
            m_string[m_size - 1] = '\0';
            --m_size;
            return true;
        } else {
            return false;
        }
    }

    static constexpr auto capacity() noexcept -> SizeType {
        return N;
    }

    constexpr auto size() const noexcept -> SizeType {
        return m_size;
    }

    constexpr auto empty() const -> bool {
        return size() == 0;
    }

    auto element_at(SizeType index) -> OptionalReference {
        if (index < m_size) {
            return m_string[index];
        } else {
            return nullopt;
        }
    }

    auto element_at(SizeType index) const -> OptionalConstReference {
        if (index < m_size) {
            return *m_string[index];
        } else {
            return nullopt;
        }
    }

    auto front_element() -> OptionalReference {
        if (!empty()) {
            return m_string[0];
        } else {
            return nullopt;
        }
    }

    auto front_element() const -> OptionalReference {
        if (!empty()) {
            return m_string[0];
        } else {
            return nullopt;
        }
    }

    auto back_element() -> OptionalReference {
        if (!empty()) {
            return m_string[size() - 1];
        } else {
            return nullopt;
        }
    }

    auto back_element() const -> OptionalConstReference {
        if (!empty()) {
            return m_string[size() - 1];
        } else {
            return nullopt;
        }
    }

    auto unchecked_access() -> UncheckedAccessor {
        return UncheckedAccessor { *this };
    }

    auto unchecked_access() const -> UncheckedConstAccessor {
        return UncheckedConstAccessor { *this };
    }

    friend auto operator==(StaticString const& lhs, StaticString const& rhs) -> bool {
        if (lhs.m_size != rhs.m_size) {
            return false;
        } else {
            auto const lhs_unchecked = lhs.unchecked_access();
            auto const rhs_unchecked = rhs.unchecked_access();
            auto const lhs_it_end = lhs_unchecked.end();
            auto lhs_it = lhs_unchecked.begin();
            auto rhs_it = rhs_unchecked.begin();
            while (lhs_it != lhs_it_end) {
                if (!(*lhs_it == *rhs_it)) {
                    return false;
                }
                ++lhs_it;
                ++rhs_it;
            }
            return true;
        }
    }

  private:
    auto is_valid_next(char character) noexcept -> bool {
        constexpr char const CODE_UNIT_UPPER_BOUND = 127;
        return (character > 0) && (character <= CODE_UNIT_UPPER_BOUND);
    }
};

} // namespace container
} // namespace iox2

#endif
