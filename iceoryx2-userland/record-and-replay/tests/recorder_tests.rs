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

#[cfg(test)]
mod recorder_tests {

    use iceoryx2::service::static_config::message_type_details::{TypeDetail, TypeVariant};
    use iceoryx2_pal_testing::assert_that;
    use iceoryx2_userland_record_and_replay::recorder::{
        RecorderBuilder, RecorderCreateError, ServiceTypes,
    };

    use iceoryx2_bb_posix::{
        file::{CreationMode, FileBuilder},
        testing::generate_file_name,
    };

    #[test]
    fn fails_when_file_already_exists() {
        let file_name = generate_file_name();

        let types = ServiceTypes {
            payload: TypeDetail::new::<u64>(TypeVariant::FixedSize),
            user_header: TypeDetail::new::<u64>(TypeVariant::FixedSize),
            system_header: TypeDetail::new::<u64>(TypeVariant::FixedSize),
        };

        let _file = FileBuilder::new(&file_name)
            .has_ownership(true)
            .creation_mode(CreationMode::PurgeAndCreate)
            .create()
            .unwrap();

        let recorder = RecorderBuilder::new(&types).create(&file_name);
        assert_that!(recorder.err(), eq Some(RecorderCreateError::FileAlreadyExists));
    }
}
