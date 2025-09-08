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

#include "iox2/container/static_string.hpp"

#include "testing/test_utils.hpp"

#include "gtest/gtest.h"

#include <limits>

namespace {

// Use the detection idiom to check if the static array bounds check is evaluated correctly
// see https://en.cppreference.com/w/cpp/types/void_t.html
template <class...>
using DetectT = void;

constexpr uint64_t const G_ARBITRARY_CAPACITY = 55;
// NOLINTNEXTLINE(modernize-type-traits), _v requires C++17
static_assert(std::is_standard_layout<iox2::container::StaticString<G_ARBITRARY_CAPACITY>>::value,
              "StaticString must be standard layout");
static_assert(iox2::container::StaticString<G_ARBITRARY_CAPACITY>::capacity() == G_ARBITRARY_CAPACITY,
              "Capacity must be determined by template argument");

TEST(StaticString, default_constructor_initializes_to_empty) {
    constexpr uint64_t const STRING_SIZE = 5;
    iox2::container::StaticString<STRING_SIZE> const sut;
    ASSERT_TRUE(sut.empty());
    ASSERT_EQ(sut.size(), 0);
}

TEST(StaticString, from_utf8_construction_from_c_style_ascii_string) {
    constexpr uint64_t const STRING_SIZE = 15;
    auto const opt_sut = iox2::container::StaticString<STRING_SIZE>::from_utf8("hello world!");
    ASSERT_TRUE(opt_sut.has_value());
    auto const& sut = opt_sut.value();
    ASSERT_EQ(sut.size(), 12);
}

TEST(StaticString, from_utf8_fails_if_string_is_not_null_terminated) {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays) testing
    char const array_not_null_terminated[] = { 'A', 'B', 'C' };
    constexpr uint64_t const STRING_SIZE = 15;
    auto const opt_sut = iox2::container::StaticString<STRING_SIZE>::from_utf8(array_not_null_terminated);
    ASSERT_TRUE(!opt_sut.has_value());
}

template <bool IsSigned = std::numeric_limits<char>::is_signed>
struct InvalidChar;
template <>
struct InvalidChar<true> : std::integral_constant<int, std::numeric_limits<char>::min()> { };
template <>
struct InvalidChar<false> : std::integral_constant<int, std::numeric_limits<char>::max()> { };

TEST(StaticString, from_utf8_fails_if_string_has_invalid_characters) {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays) testing
    char input_array[] = { 'A', 'B', 'C', '\0' };
    char const invalid_character = static_cast<char>(InvalidChar<>::value);
    constexpr uint64_t const STRING_SIZE = 15;
    ASSERT_TRUE(iox2::container::StaticString<STRING_SIZE>::from_utf8(input_array).has_value());
    input_array[0] = invalid_character;
    ASSERT_TRUE(!iox2::container::StaticString<STRING_SIZE>::from_utf8(input_array).has_value());
    input_array[0] = 'A';
    input_array[1] = invalid_character;
    ASSERT_TRUE(!iox2::container::StaticString<STRING_SIZE>::from_utf8(input_array).has_value());
    input_array[1] = 'B';
    input_array[2] = invalid_character;
    ASSERT_TRUE(!iox2::container::StaticString<STRING_SIZE>::from_utf8(input_array).has_value());
}


template <uint64_t, class = void>
struct DetectInvalidFromUtf8WithStringABC : std::false_type { };
template <uint64_t M>
struct DetectInvalidFromUtf8WithStringABC<M, DetectT<decltype(iox2::container::StaticString<M>::from_utf8("ABC"))>>
    : std::true_type { };

TEST(StaticString, from_utf8_works_up_to_capacity) {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays) testing
    char const array_too_many_elements[] = { 'A', 'B', 'C', '\0' };
    constexpr uint64_t const STRING_SIZE = 3;
    auto const opt_sut = iox2::container::StaticString<STRING_SIZE>::from_utf8(array_too_many_elements);
    ASSERT_TRUE(opt_sut.has_value());
    ASSERT_STREQ(opt_sut->unchecked_access().c_str(), "ABC");
    static_assert(DetectInvalidFromUtf8WithStringABC<4>::value, "ABC fits into capacity 4");
    static_assert(DetectInvalidFromUtf8WithStringABC<3>::value, "ABC fits into capacity 3");
    static_assert(!DetectInvalidFromUtf8WithStringABC<2>::value, "ABC does not fit into capacity 2");
    static_assert(!DetectInvalidFromUtf8WithStringABC<1>::value, "ABC does not fit into capacity 1");
    static_assert(!DetectInvalidFromUtf8WithStringABC<0>::value, "ABC does not fit into capacity 0");
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers) capacity has no significance for this test
template <typename T, typename U = decltype(iox2::container::StaticString<99>::from_utf8(std::declval<T&&>()))>
constexpr auto can_call_from_utf8_with(T&& /* unused */) -> std::true_type {
    return {};
}
// NOLINTNEXTLINE(modernize-type-traits), _v requires C++17
template <typename T, typename = std::enable_if_t<!std::is_array<std::remove_reference_t<T>>::value, bool>>
constexpr auto can_call_from_utf8_with(T&& /* unused */) -> std::false_type {
    return {};
}

TEST(StaticString, from_utf8_works_only_with_statically_known_strings) {
    static_assert(can_call_from_utf8_with("ABC"));
    static_assert(!can_call_from_utf8_with(static_cast<char const*>("ABC")));
}

TEST(StaticString, from_utf8_null_terminated_unchecked_construction_from_null_terminated_c_style_string) {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays) testing
    char const test_string[] = "Hello World";
    constexpr uint64_t const STRING_SIZE = 15;
    auto const opt_sut = iox2::container::StaticString<STRING_SIZE>::from_utf8(test_string);
    ASSERT_TRUE(opt_sut.has_value());
    ASSERT_EQ(opt_sut->size(), sizeof(test_string) - 1);
    EXPECT_STREQ(opt_sut->unchecked_access().c_str(), static_cast<char const*>(test_string));
}

TEST(StaticString, from_utf8_null_terminated_unchecked_fails_if_string_has_invalid_characters) {
    // NOLINTBEGIN(clang-analyzer-security.insecureAPI.strcpy,cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay) testing
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays) testing
    char const test_string[] = "Hello World";
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays) testing
    char mutable_string[sizeof(test_string)];
    strcpy(mutable_string, test_string);
    char const* str_ptr = mutable_string;
    constexpr uint64_t const STRING_SIZE = 15;
    ASSERT_TRUE(iox2::container::StaticString<STRING_SIZE>::from_utf8_null_terminated_unchecked(str_ptr).has_value());
    mutable_string[0] = InvalidChar<>::value;
    ASSERT_TRUE(!iox2::container::StaticString<STRING_SIZE>::from_utf8_null_terminated_unchecked(str_ptr).has_value());
    strcpy(mutable_string, test_string);
    mutable_string[1] = InvalidChar<>::value;
    ASSERT_TRUE(!iox2::container::StaticString<STRING_SIZE>::from_utf8_null_terminated_unchecked(str_ptr).has_value());
    strcpy(mutable_string, test_string);
    mutable_string[2] = InvalidChar<>::value;
    ASSERT_TRUE(!iox2::container::StaticString<STRING_SIZE>::from_utf8_null_terminated_unchecked(str_ptr).has_value());
    strcpy(mutable_string, test_string);
    mutable_string[3] = InvalidChar<>::value;
    ASSERT_TRUE(!iox2::container::StaticString<STRING_SIZE>::from_utf8_null_terminated_unchecked(str_ptr).has_value());
    strcpy(mutable_string, test_string);
    mutable_string[sizeof(test_string) - 3] = InvalidChar<>::value;
    ASSERT_TRUE(!iox2::container::StaticString<STRING_SIZE>::from_utf8_null_terminated_unchecked(str_ptr).has_value());
    strcpy(mutable_string, test_string);
    mutable_string[sizeof(test_string) - 2] = InvalidChar<>::value;
    ASSERT_TRUE(!iox2::container::StaticString<STRING_SIZE>::from_utf8_null_terminated_unchecked(str_ptr).has_value());
    // NOLINTEND(clang-analyzer-security.insecureAPI.strcpy,cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
}

TEST(StaticString, try_push_back_appends_character_to_string_if_there_is_room) {
    constexpr uint64_t const STRING_SIZE = 5;
    iox2::container::StaticString<STRING_SIZE> sut;
    ASSERT_TRUE(sut.try_push_back('A'));
    ASSERT_EQ(sut.size(), 1);
    EXPECT_EQ(*sut.back_element(), 'A');
    EXPECT_STREQ(sut.unchecked_access().c_str(), "A");
    ASSERT_TRUE(sut.try_push_back('B'));
    ASSERT_EQ(sut.size(), 2);
    EXPECT_EQ(*sut.back_element(), 'B');
    EXPECT_STREQ(sut.unchecked_access().c_str(), "AB");
    ASSERT_TRUE(sut.try_push_back('C'));
    ASSERT_EQ(sut.size(), 3);
    EXPECT_EQ(*sut.back_element(), 'C');
    EXPECT_STREQ(sut.unchecked_access().c_str(), "ABC");
    ASSERT_TRUE(sut.try_push_back('D'));
    ASSERT_EQ(sut.size(), 4);
    EXPECT_EQ(*sut.back_element(), 'D');
    EXPECT_STREQ(sut.unchecked_access().c_str(), "ABCD");
    ASSERT_TRUE(sut.try_push_back('E'));
    ASSERT_EQ(sut.size(), 5);
    EXPECT_EQ(*sut.back_element(), 'E');
    EXPECT_STREQ(sut.unchecked_access().c_str(), "ABCDE");
}

TEST(StaticString, try_push_back_fails_if_there_is_no_room) {
    constexpr uint64_t const STRING_SIZE = 3;
    iox2::container::StaticString<STRING_SIZE> sut;
    ASSERT_TRUE(sut.try_push_back('A') && sut.try_push_back('B') && sut.try_push_back('C'));
    ASSERT_EQ(sut.size(), sut.capacity());
    EXPECT_FALSE(sut.try_push_back('D'));
    EXPECT_STREQ(sut.unchecked_access().c_str(), "ABC");
}

TEST(StaticString, static_string_with_capacity_0_can_never_be_pushed_into) {
    iox2::container::StaticString<0> sut;
    ASSERT_TRUE(sut.empty());
    ASSERT_EQ(sut.size(), 0);
    ASSERT_FALSE(sut.try_push_back('A'));
    ASSERT_STREQ(sut.unchecked_access().c_str(), "");
}

TEST(StaticString, try_pop_removes_last_element_from_string) {
    constexpr uint64_t const STRING_SIZE = 5;
    auto sut = *iox2::container::StaticString<STRING_SIZE>::from_utf8_null_terminated_unchecked("ABCDE");
    ASSERT_STREQ(sut.unchecked_access().c_str(), "ABCDE");
    ASSERT_TRUE(sut.try_pop_back());
    ASSERT_EQ(sut.size(), 4);
    ASSERT_STREQ(sut.unchecked_access().c_str(), "ABCD");
    ASSERT_TRUE(sut.try_pop_back());
    ASSERT_EQ(sut.size(), 3);
    ASSERT_STREQ(sut.unchecked_access().c_str(), "ABC");
    ASSERT_TRUE(sut.try_pop_back());
    ASSERT_EQ(sut.size(), 2);
    ASSERT_STREQ(sut.unchecked_access().c_str(), "AB");
    ASSERT_TRUE(sut.try_pop_back());
    ASSERT_EQ(sut.size(), 1);
    ASSERT_STREQ(sut.unchecked_access().c_str(), "A");
    ASSERT_TRUE(sut.try_pop_back());
    ASSERT_EQ(sut.size(), 0);
    ASSERT_STREQ(sut.unchecked_access().c_str(), "");
}

TEST(StaticString, try_pop_fails_on_empty_string) {
    constexpr uint64_t const STRING_SIZE = 5;
    auto sut = *iox2::container::StaticString<STRING_SIZE>::from_utf8_null_terminated_unchecked("A");
    ASSERT_TRUE(sut.try_pop_back());
    ASSERT_TRUE(sut.empty());
    ASSERT_FALSE(sut.try_pop_back());
    ASSERT_TRUE(sut.empty());
    ASSERT_FALSE(sut.try_pop_back());
    ASSERT_TRUE(sut.empty());
}

TEST(StaticString, size_returns_number_of_elements_in_string) {
    constexpr uint64_t const STRING_SIZE = 5;
    iox2::container::StaticString<STRING_SIZE> sut;
    ASSERT_EQ(sut.size(), 0);
    ASSERT_TRUE(sut.try_push_back('A'));
    ASSERT_EQ(sut.size(), 1);
    ASSERT_TRUE(sut.try_push_back('A'));
    ASSERT_EQ(sut.size(), 2);
    ASSERT_TRUE(sut.try_pop_back());
    ASSERT_EQ(sut.size(), 1);
    ASSERT_TRUE(sut.try_pop_back());
    ASSERT_EQ(sut.size(), 0);
}

TEST(StaticString, empty_indicates_whether_the_string_is_empty) {
    constexpr uint64_t const STRING_SIZE = 5;
    iox2::container::StaticString<STRING_SIZE> sut;
    ASSERT_TRUE(sut.empty());
    ASSERT_TRUE(sut.try_push_back('A'));
    ASSERT_TRUE(!sut.empty());
    ASSERT_TRUE(sut.try_push_back('A'));
    ASSERT_TRUE(!sut.empty());
    ASSERT_TRUE(sut.try_pop_back());
    ASSERT_TRUE(!sut.empty());
    ASSERT_TRUE(sut.try_pop_back());
    ASSERT_TRUE(sut.empty());
}

} // namespace
