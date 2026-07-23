/*
 *  Copyright (c) 2026 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "audio/audio_transport_impl.h"

#include "api/make_ref_counted.h"
#include "api/test/mock_audio_mixer.h"
#include "modules/audio_processing/include/mock_audio_processing.h"
#include "test/gmock.h"
#include "test/gtest.h"

namespace webrtc {
namespace {

using test::MockAudioMixer;
using test::MockAudioProcessing;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::StrictMock;

TEST(AudioTransportImplTest, RouteChangeReinitializesAudioProcessing) {
  auto mixer = make_ref_counted<NiceMock<MockAudioMixer>>();
  auto audio_processing =
      make_ref_counted<StrictMock<MockAudioProcessing>>();
  AudioTransportImpl transport(mixer.get(), audio_processing.get(), nullptr);

  EXPECT_CALL(*audio_processing, Initialize())
      .WillOnce(Return(AudioProcessing::kNoError));

  EXPECT_EQ(AudioProcessing::kNoError, transport.OnAudioRouteChanged());
}

TEST(AudioTransportImplTest, RouteChangePropagatesAudioProcessingFailure) {
  auto mixer = make_ref_counted<NiceMock<MockAudioMixer>>();
  auto audio_processing =
      make_ref_counted<StrictMock<MockAudioProcessing>>();
  AudioTransportImpl transport(mixer.get(), audio_processing.get(), nullptr);

  EXPECT_CALL(*audio_processing, Initialize())
      .WillOnce(Return(AudioProcessing::kCreationFailedError));

  EXPECT_EQ(AudioProcessing::kCreationFailedError,
            transport.OnAudioRouteChanged());
}

TEST(AudioTransportImplTest, RouteChangeSucceedsWithoutAudioProcessing) {
  auto mixer = make_ref_counted<NiceMock<MockAudioMixer>>();
  AudioTransportImpl transport(mixer.get(), nullptr, nullptr);

  EXPECT_EQ(AudioProcessing::kNoError, transport.OnAudioRouteChanged());
}

}  // namespace
}  // namespace webrtc
