/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "modules/audio_device/fine_audio_buffer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>

#include "api/array_view.h"
#include "modules/audio_device/audio_device_buffer.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/numerics/safe_conversions.h"

namespace webrtc {

namespace {

// A small callback-size change is normal, but samples separated by a route or
// clock discontinuity must not be assembled into one 10 ms AEC packet.
constexpr int64_t kMaxCaptureTimestampDiscontinuityNs = 100'000'000;

}  // namespace

FineAudioBuffer::FineAudioBuffer(AudioDeviceBuffer* audio_device_buffer)
    : audio_device_buffer_(audio_device_buffer),
      playout_samples_per_channel_10ms_(dchecked_cast<size_t>(
          audio_device_buffer->PlayoutSampleRate() * 10 / 1000)),
      record_samples_per_channel_10ms_(dchecked_cast<size_t>(
          audio_device_buffer->RecordingSampleRate() * 10 / 1000)),
      playout_channels_(audio_device_buffer->PlayoutChannels()),
      record_channels_(audio_device_buffer->RecordingChannels()) {
  RTC_DCHECK(audio_device_buffer_);
  RTC_DLOG(LS_INFO) << __FUNCTION__;
  if (IsReadyForPlayout()) {
    RTC_DLOG(LS_INFO) << "playout_samples_per_channel_10ms: "
                      << playout_samples_per_channel_10ms_;
    RTC_DLOG(LS_INFO) << "playout_channels: " << playout_channels_;
  }
  if (IsReadyForRecord()) {
    RTC_DLOG(LS_INFO) << "record_samples_per_channel_10ms: "
                      << record_samples_per_channel_10ms_;
    RTC_DLOG(LS_INFO) << "record_channels: " << record_channels_;
  }
}

FineAudioBuffer::~FineAudioBuffer() {
  RTC_DLOG(LS_INFO) << __FUNCTION__;
}

void FineAudioBuffer::ResetPlayout() {
  playout_buffer_.Clear();
}

void FineAudioBuffer::ResetRecord() {
  record_buffer_.Clear();
  record_buffer_capture_time_ns_.reset();
}

bool FineAudioBuffer::IsReadyForPlayout() const {
  return playout_samples_per_channel_10ms_ > 0 && playout_channels_ > 0;
}

bool FineAudioBuffer::IsReadyForRecord() const {
  return record_samples_per_channel_10ms_ > 0 && record_channels_ > 0;
}

void FineAudioBuffer::GetPlayoutData(ArrayView<int16_t> audio_buffer,
                                     int playout_delay_ms) {
  RTC_DCHECK(IsReadyForPlayout());
  // Ask WebRTC for new data in chunks of 10ms until we have enough to
  // fulfill the request. It is possible that the buffer already contains
  // enough samples from the last round.
  while (playout_buffer_.size() < audio_buffer.size()) {
    // Get 10ms decoded audio from WebRTC. The ADB knows about number of
    // channels; hence we can ask for number of samples per channel here.
    if (audio_device_buffer_->RequestPlayoutData(
            playout_samples_per_channel_10ms_) ==
        static_cast<int32_t>(playout_samples_per_channel_10ms_)) {
      // Append 10ms to the end of the local buffer taking number of channels
      // into account.
      const size_t num_elements_10ms =
          playout_channels_ * playout_samples_per_channel_10ms_;
      const size_t written_elements = playout_buffer_.AppendData(
          num_elements_10ms, [&](ArrayView<int16_t> buf) {
            const size_t samples_per_channel_10ms =
                audio_device_buffer_->GetPlayoutData(buf.data());
            return playout_channels_ * samples_per_channel_10ms;
          });
      RTC_DCHECK_EQ(num_elements_10ms, written_elements);
    } else {
      // Provide silence if AudioDeviceBuffer::RequestPlayoutData() fails.
      // Can e.g. happen when an AudioTransport has not been registered.
      const size_t num_bytes = audio_buffer.size() * sizeof(int16_t);
      std::memset(audio_buffer.data(), 0, num_bytes);
      return;
    }
  }

  // Provide the requested number of bytes to the consumer.
  const size_t num_bytes = audio_buffer.size() * sizeof(int16_t);
  memcpy(audio_buffer.data(), playout_buffer_.data(), num_bytes);
  // Move remaining samples to start of buffer to prepare for next round.
  memmove(playout_buffer_.data(), playout_buffer_.data() + audio_buffer.size(),
          (playout_buffer_.size() - audio_buffer.size()) * sizeof(int16_t));
  playout_buffer_.SetSize(playout_buffer_.size() - audio_buffer.size());
  // Cache playout latency for usage in DeliverRecordedData();
  playout_delay_ms_ = playout_delay_ms;
}

void FineAudioBuffer::DeliverRecordedData(
    ArrayView<const int16_t> audio_buffer,
    int record_delay_ms,
    std::optional<int64_t> capture_time_ns) {
  RTC_DCHECK(IsReadyForRecord());
  size_t buffered_elements_before_append = record_buffer_.size();
  if (buffered_elements_before_append > 0 && capture_time_ns.has_value() &&
      record_buffer_capture_time_ns_.has_value()) {
    const size_t buffered_frames =
        buffered_elements_before_append / record_channels_;
    const int64_t buffered_duration_ns = static_cast<int64_t>(
        (1'000'000'000.0 * buffered_frames) /
        audio_device_buffer_->RecordingSampleRate());
    const int64_t expected_capture_time_ns =
        *record_buffer_capture_time_ns_ + buffered_duration_ns;
    const int64_t timestamp_delta_ns =
        *capture_time_ns - expected_capture_time_ns;
    if (timestamp_delta_ns > kMaxCaptureTimestampDiscontinuityNs ||
        timestamp_delta_ns < -kMaxCaptureTimestampDiscontinuityNs) {
      RTC_LOG(LS_WARNING)
          << "Discarding partial capture packet after timestamp discontinuity";
      record_buffer_.Clear();
      record_buffer_capture_time_ns_ = capture_time_ns;
      buffered_elements_before_append = 0;
    }
  }
  if (buffered_elements_before_append == 0) {
    record_buffer_capture_time_ns_ = capture_time_ns;
  } else if (!record_buffer_capture_time_ns_.has_value() &&
             capture_time_ns.has_value()) {
    const size_t buffered_frames =
        buffered_elements_before_append / record_channels_;
    const int64_t buffered_duration_ns = static_cast<int64_t>(
        (1'000'000'000.0 * buffered_frames) /
        audio_device_buffer_->RecordingSampleRate());
    record_buffer_capture_time_ns_ =
        *capture_time_ns - buffered_duration_ns;
  }
  // Always append new data and grow the buffer when needed.
  record_buffer_.AppendData(audio_buffer.data(), audio_buffer.size());
  // Consume samples from buffer in chunks of 10ms until there is not
  // enough data left. The number of remaining samples in the cache is given by
  // the new size of the internal `record_buffer_`.
  const size_t num_elements_10ms =
      record_channels_ * record_samples_per_channel_10ms_;
  while (record_buffer_.size() >= num_elements_10ms) {
    double effective_record_delay_ms = record_delay_ms;
    if (capture_time_ns.has_value() &&
        record_buffer_capture_time_ns_.has_value()) {
      effective_record_delay_ms +=
          (*capture_time_ns - *record_buffer_capture_time_ns_) / 1'000'000.0;
    }
    const int bounded_record_delay_ms = static_cast<int>(std::clamp(
        std::round(effective_record_delay_ms), 0.0,
        static_cast<double>(std::numeric_limits<uint16_t>::max())));
    audio_device_buffer_->SetRecordedBuffer(record_buffer_.data(),
                                            record_samples_per_channel_10ms_,
                                            record_buffer_capture_time_ns_);
    audio_device_buffer_->SetVQEData(playout_delay_ms_,
                                     bounded_record_delay_ms);
    audio_device_buffer_->DeliverRecordedData();
    memmove(record_buffer_.data(), record_buffer_.data() + num_elements_10ms,
            (record_buffer_.size() - num_elements_10ms) * sizeof(int16_t));
    record_buffer_.SetSize(record_buffer_.size() - num_elements_10ms);
    if (record_buffer_capture_time_ns_.has_value()) {
      *record_buffer_capture_time_ns_ += 10'000'000;
    }
    if (record_buffer_.size() == 0) {
      record_buffer_capture_time_ns_.reset();
    }
  }
}

}  // namespace webrtc
