# Inline WebRTC fork

This fork supplies the WebRTC source used by Inline's LiveKitWebRTC binary.
The `m144_release` branch starts from the exact source revision used by
LiveKitWebRTC `144.7559.11`:

```text
39d2180660d43d2e1630e564e3afe1c1fb72746e
```

Inline's source and build history are maintained in:

- https://github.com/inline-chat/webrtc
- https://github.com/inline-chat/webrtc-build
- https://github.com/inline-chat/webrtc-xcframework
- https://github.com/inline-chat/client-sdk-swift

## Native typing-noise suppression

Upstream WebRTC still carries the per-capture-frame typing signal from
`AudioDeviceBuffer::SetTypingStatus()` to
`AudioProcessing::set_stream_key_pressed()`, but the AVAudioEngine audio device
did not populate that signal. Upstream also removed the transient suppressor
implementation while retaining its public config field.

Inline restores the complete transient suppressor module and its
`AudioProcessingImpl` integration, exposes the config through
`RTCAudioProcessingConfig`, and reports live macOS HID key state immediately
before each microphone frame is delivered. The key-state implementation
matches Inline's previous Noor client: it reports `true` while any virtual key
in the macOS `0...0x5D` range is held.

The feature requires all of these pieces:

1. `modules/audio_device/audio_engine_device.mm` supplies key state.
2. `AudioDeviceBuffer` and `AudioTransportImpl` forward the state into APM.
3. `modules/audio_processing/transient/` contains the detector and suppressor.
4. `AudioProcessingImpl` creates and runs the suppressor when enabled.
5. `RTCAudioProcessingConfig.isTransientSuppressionEnabled` exposes the config
   to LiveKit's Swift SDK.
6. The LiveKit client sets that property before starting microphone capture.

Apple Voice Processing I/O is not required or enabled by this feature.

## Validation and upgrades

The build fork's `test-native-typing.yml` workflow builds and runs
`transient_suppression_unittests_runner`, including an APM integration test.
The full Apple build workflow compiles the source for every packaged Apple
slice, reruns that native test target, and produces the prefixed
`LiveKitWebRTC.xcframework.zip`.

For a WebRTC upgrade:

1. Start from the exact source commit used by the intended upstream
   LiveKitWebRTC release.
2. Reapply or port the six required pieces above.
3. Pin the exact resulting commit in `inline-chat/webrtc-build`.
4. Run the focused native test workflow and the full Apple XCFramework build.
5. Publish a new immutable `webrtc-xcframework` release and update
   `client-sdk-swift` to its exact version.
6. Update Inline to the resulting exact LiveKit client commit.
