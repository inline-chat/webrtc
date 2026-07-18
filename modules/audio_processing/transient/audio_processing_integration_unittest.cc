/*
 *  Copyright (c) 2026 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <array>

#include "api/audio/audio_processing.h"
#include "api/audio/builtin_audio_processing_builder.h"
#include "api/environment/environment_factory.h"
#include "test/gtest.h"

namespace webrtc {
namespace {

TEST(TypingNoiseSuppressionIntegrationTest,
     AppliesConfigAndProcessesKeyPressedCapture) {
  AudioProcessing::Config config;
  config.transient_suppression.enabled = true;
  scoped_refptr<AudioProcessing> apm =
      BuiltinAudioProcessingBuilder(config).Build(CreateEnvironment());

  ASSERT_TRUE(apm);
  EXPECT_TRUE(apm->GetConfig().transient_suppression.enabled);

  constexpr int kSampleRateHz = 16000;
  constexpr int kNumChannels = 1;
  std::array<float, kSampleRateHz / 100> audio = {};
  float* channels[] = {audio.data()};
  StreamConfig stream_config(kSampleRateHz, kNumChannels);

  apm->set_stream_key_pressed(true);
  EXPECT_EQ(apm->ProcessStream(channels, stream_config, stream_config,
                               channels),
            AudioProcessing::kNoError);
}

}  // namespace
}  // namespace webrtc
