// Copyright (c) 2024 Contributors to the Eclipse Foundation
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

#include "testing/test_utils.hpp"

namespace iox2 {
namespace container {
namespace testing {

void opaque_use(void* /* object */) {
}
void opaque_use(void const* /* object */) {
}

int CustomAddressOperator::s_count_address_operator = 0;

} // namespace testing
} // namespace container
} // namespace iox2
