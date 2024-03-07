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

use std::{
    alloc::Layout,
    sync::atomic::{AtomicBool, Ordering},
};

use iceoryx2_bb_elementary::{
    math::align_to,
    owning_pointer::OwningPointer,
    relocatable_container::RelocatableContainer,
    relocatable_ptr::{PointerTrait, RelocatablePointer},
};
use iceoryx2_bb_log::{fail, fatal_panic};

use crate::shm_allocator::PointerOffset;

pub type UsedChunkList = details::UsedChunkList<OwningPointer<AtomicBool>>;
pub type RelocatableUsedChunkList = details::UsedChunkList<RelocatablePointer<AtomicBool>>;

pub mod details {
    use std::fmt::Debug;

    use iceoryx2_bb_elementary::{math::unaligned_mem_size, owning_pointer::OwningPointer};

    use super::*;

    #[derive(Debug)]
    #[repr(C)]
    pub struct UsedChunkList<PointerType: PointerTrait<AtomicBool>> {
        data_ptr: PointerType,
        capacity: usize,
        is_memory_initialized: AtomicBool,
    }

    unsafe impl<PointerType: PointerTrait<AtomicBool>> Send for UsedChunkList<PointerType> {}
    unsafe impl<PointerType: PointerTrait<AtomicBool>> Sync for UsedChunkList<PointerType> {}

    impl UsedChunkList<OwningPointer<AtomicBool>> {
        pub fn new(capacity: usize) -> Self {
            let mut data_ptr = OwningPointer::<AtomicBool>::new_with_alloc(capacity);

            for i in 0..capacity {
                unsafe { data_ptr.as_mut_ptr().add(i).write(AtomicBool::new(false)) };
            }

            Self {
                data_ptr,
                capacity,
                is_memory_initialized: AtomicBool::new(true),
            }
        }
    }

    impl RelocatableContainer for UsedChunkList<RelocatablePointer<AtomicBool>> {
        unsafe fn new_uninit(capacity: usize) -> Self {
            Self {
                data_ptr: RelocatablePointer::new_uninit(),
                capacity,
                is_memory_initialized: AtomicBool::new(false),
            }
        }

        unsafe fn init<T: iceoryx2_bb_elementary::allocator::BaseAllocator>(
            &self,
            allocator: &T,
        ) -> Result<(), iceoryx2_bb_elementary::allocator::AllocationError> {
            if self.is_memory_initialized.load(Ordering::Relaxed) {
                fatal_panic!(from self,
                "Memory already initialized. Initializing it twice may lead to undefined behavior.");
            }

            let memory = fail!(from self, when allocator
            .allocate(Layout::from_size_align_unchecked(
                    std::mem::size_of::<AtomicBool>() * self.capacity,
                    std::mem::align_of::<AtomicBool>())),
            "Failed to initialize since the allocation of the data memory failed.");

            self.data_ptr.init(memory);

            for i in 0..self.capacity {
                unsafe {
                    (self.data_ptr.as_ptr() as *mut AtomicBool)
                        .add(i)
                        .write(AtomicBool::new(false))
                };
            }

            // relaxed is sufficient since no relocatable container can be used
            // before init was called. Meaning, it is not allowed to send or share
            // the container with other threads when it is in an uninitialized state.
            self.is_memory_initialized.store(true, Ordering::Relaxed);

            Ok(())
        }

        fn memory_size(capacity: usize) -> usize {
            Self::const_memory_size(capacity)
        }

        unsafe fn new(capacity: usize, distance_to_data: isize) -> Self {
            Self {
                data_ptr: RelocatablePointer::new(distance_to_data),
                capacity,
                is_memory_initialized: AtomicBool::new(true),
            }
        }
    }

    impl<PointerType: PointerTrait<AtomicBool> + Debug> UsedChunkList<PointerType> {
        pub const fn const_memory_size(capacity: usize) -> usize {
            unaligned_mem_size::<AtomicBool>(capacity)
        }

        pub fn capacity(&self) -> usize {
            self.capacity
        }

        #[inline(always)]
        fn verify_init(&self, source: &str) {
            debug_assert!(
                self.is_memory_initialized.load(Ordering::Relaxed),
                "Undefined behavior when calling \"{}\" and the object is not initialized.",
                source
            );
        }

        fn set(&self, idx: usize, value: bool) -> bool {
            self.verify_init("set");
            debug_assert!(
                idx < self.capacity,
                "This should never happen. Out of bounds access with index {}.",
                idx
            );

            unsafe { (*self.data_ptr.as_ptr().add(idx)).swap(value, Ordering::Relaxed) }
        }

        pub fn insert(&self, value: usize) -> bool {
            !self.set(value, true)
        }

        pub fn remove(&self, value: usize) -> bool {
            self.set(value, false)
        }

        pub fn remove_all<F: FnMut(usize)>(&self, mut callback: F) {
            self.verify_init("pop");

            for i in 0..self.capacity {
                if unsafe { (*self.data_ptr.as_ptr().add(i)).swap(false, Ordering::Relaxed) } {
                    callback(i);
                }
            }
        }
    }
}

#[derive(Debug)]
#[repr(C)]
pub struct FixedSizeUsedChunkList<const CAPACITY: usize> {
    list: RelocatableUsedChunkList,
    data: [AtomicBool; CAPACITY],
}

impl<const CAPACITY: usize> Default for FixedSizeUsedChunkList<CAPACITY> {
    fn default() -> Self {
        Self {
            list: unsafe {
                RelocatableUsedChunkList::new(
                    CAPACITY,
                    align_to::<AtomicBool>(std::mem::size_of::<RelocatableUsedChunkList>()) as _,
                )
            },
            data: core::array::from_fn(|_| AtomicBool::new(false)),
        }
    }
}

impl<const CAPACITY: usize> FixedSizeUsedChunkList<CAPACITY> {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn insert(&self, value: usize) -> bool {
        self.list.insert(value)
    }

    pub fn remove_all<F: FnMut(usize)>(&mut self, callback: F) {
        self.list.remove_all(callback)
    }

    pub const fn capacity(&self) -> usize {
        CAPACITY
    }

    pub fn remove(&self, value: usize) -> bool {
        self.list.remove(value)
    }
}
