/*
 * Copyright 2024 LiveKit
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MEDIA_AUDIO_MAC_CORE_AUDIO_UTIL_MAC_H_
#define MEDIA_AUDIO_MAC_CORE_AUDIO_UTIL_MAC_H_

#include <AudioUnit/AudioUnit.h>
#include <CoreAudio/CoreAudio.h>

#include <optional>
#include <string>
#include <vector>

namespace webrtc {
namespace mac_audio_utils {

std::vector<AudioObjectID> GetAllAudioDeviceIDs();

std::optional<AudioObjectID> GetDefaultOutputDeviceID();

std::optional<AudioObjectID> GetDefaultInputDeviceID();

std::vector<AudioObjectID> GetRelatedDeviceIDs(AudioObjectID device_id);

std::optional<std::string> GetDeviceUniqueID(AudioObjectID device_id);

std::optional<std::string> GetDeviceName(AudioObjectID device_id);

std::optional<std::string> GetDeviceLabel(AudioObjectID device_id,
                                          bool is_input);

uint32_t GetNumStreams(AudioObjectID device_id, bool is_input);

// Returns the total channel count published by all streams in the requested
// direction. Aggregate subdevices are flattened in subdevice order.
uint32_t GetNumChannels(AudioObjectID device_id, bool is_input);

std::optional<uint32_t> GetDeviceSource(AudioObjectID device_id, bool is_input);

std::optional<uint32_t> GetDeviceTransportType(AudioObjectID device_id);

bool IsInputDevice(AudioObjectID device_id);

bool IsOutputDevice(AudioObjectID device_id);

// Creates a private (process local) aggregate device combining the given
// output and input devices, so a single HAL I/O unit can address both. The
// output device is the clock master and drift compensation is enabled for
// the input sub device. Returns the aggregate AudioObjectID on success.
std::optional<AudioObjectID> CreatePrivateAggregateDevice(
    AudioObjectID output_device_id,
    AudioObjectID input_device_id);

// Verifies that an aggregate contains exactly the requested output and input
// devices in that order, uses the output as its time source, has input drift
// compensation enabled, and publishes streams in both directions.
bool VerifyPrivateAggregateDevice(AudioObjectID aggregate_device_id,
                                  AudioObjectID output_device_id,
                                  AudioObjectID input_device_id);

// Destroys an aggregate device created by CreatePrivateAggregateDevice.
// Core Audio destroys aggregates asynchronously; success means the object has
// also disappeared before the bounded verification deadline.
bool DestroyAggregateDevice(AudioObjectID aggregate_device_id);

}  // namespace mac_audio_utils
}  // namespace webrtc

#endif  // MEDIA_AUDIO_MAC_CORE_AUDIO_UTIL_MAC_H_
