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

use core::mem::MaybeUninit;
use iceoryx2_bb_container::vector::static_vec::*;
use iceoryx2_bb_elementary_traits::placement_default::PlacementDefault;
use iceoryx2_bb_testing::{assert_that, lifetime_tracker::LifetimeTracker};
use serde_test::{assert_tokens, Token};

const SUT_CAPACITY: usize = 10;

#[test]
fn default_created_vec_is_empty() {
    let sut = StaticVec::<LifetimeTracker, SUT_CAPACITY>::default();

    assert_that!(sut.is_empty(), eq true);
    assert_that!(sut.len(), eq 0);
    assert_that!(sut.is_full(), eq false);
}

#[test]
fn two_vectors_with_same_content_are_equal() {
    let mut sut1 = StaticVec::<usize, SUT_CAPACITY>::new();
    let mut sut2 = StaticVec::<usize, SUT_CAPACITY>::new();

    for n in 0..SUT_CAPACITY {
        sut1.push(4 * n + 3);
        sut2.insert(n, 4 * n + 3);
    }

    assert_that!(sut1, eq sut2);
}

#[test]
fn two_vectors_with_different_content_are_not_equal() {
    let mut sut1 = StaticVec::<usize, SUT_CAPACITY>::new();
    let mut sut2 = StaticVec::<usize, SUT_CAPACITY>::new();

    for n in 0..SUT_CAPACITY {
        sut1.push(4 * n + 3);
        sut2.insert(n, 4 * n + 3);
    }

    sut2[5] = 0;

    assert_that!(sut1, ne sut2);
}

#[test]
fn two_vectors_with_different_len_are_not_equal() {
    let mut sut1 = StaticVec::<usize, SUT_CAPACITY>::new();
    let mut sut2 = StaticVec::<usize, SUT_CAPACITY>::new();

    for n in 0..SUT_CAPACITY {
        sut1.push(4 * n + 3);
        sut2.insert(n, 4 * n + 3);
    }

    sut2.pop();

    assert_that!(sut1, ne sut2);
}

#[test]
fn placement_default_works() {
    let mut sut = MaybeUninit::<StaticVec<usize, SUT_CAPACITY>>::uninit();

    unsafe { PlacementDefault::placement_default(sut.as_mut_ptr()) };
    let sut = unsafe { sut.assume_init() };

    assert_that!(sut.len(), eq 0);
    assert_that!(sut.is_empty(), eq true);
}

#[test]
fn serialization_works() {
    let mut sut = StaticVec::<usize, SUT_CAPACITY>::new();
    sut.push(44617);
    sut.push(123123);
    sut.push(89712);
    sut.push(99101);

    assert_tokens(
        &sut,
        &[
            Token::Seq { len: Some(4) },
            Token::U64(44617),
            Token::U64(123123),
            Token::U64(89712),
            Token::U64(99101),
            Token::SeqEnd,
        ],
    );
}

#[test]
fn valid_after_move() {
    let mut sut = StaticVec::<usize, SUT_CAPACITY>::new();

    for i in 0..sut.capacity() {
        let element = i * 2 + 3;
        assert_that!(sut.push(element), eq true);
    }

    let mut sut2 = sut;

    for i in 0..sut2.capacity() {
        let result = sut2.pop();
        assert_that!(result, eq Some((sut2.capacity() - i - 1) * 2 + 3));
    }
}

#[test]
fn clone_clones_empty_vec() {
    let sut1 = StaticVec::<usize, SUT_CAPACITY>::new();

    let sut2 = sut1.clone();

    assert_that!(sut1.len(), eq 0);
    assert_that!(sut2.len(), eq 0);
}

#[test]
fn clone_clones_filled_vec() {
    let mut sut1 = StaticVec::<usize, SUT_CAPACITY>::new();
    for i in 0..sut1.capacity() {
        let element = i * 2 + 3;
        assert_that!(sut1.push(element), eq true);
    }

    let sut2 = sut1.clone();

    assert_that!(sut1.len(), eq SUT_CAPACITY);
    assert_that!(sut2.len(), eq SUT_CAPACITY);

    for i in 0..sut1.capacity() {
        let element = i * 2 + 3;
        assert_that!(sut1[i], eq element);
        assert_that!(sut2[i], eq element);
    }
}
