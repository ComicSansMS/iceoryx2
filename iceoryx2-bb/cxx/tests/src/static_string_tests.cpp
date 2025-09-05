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

namespace {

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

TEST(StaticString, static_string_with_capacity_0_is_always_empty) {
    iox2::container::StaticString<0> sut;
    ASSERT_TRUE(sut.empty());
    ASSERT_EQ(sut.size(), 0);
    ASSERT_FALSE(sut.try_push_back('A'));
    ASSERT_STREQ(sut.unchecked_access().c_str(), "");
}

} // namespace
