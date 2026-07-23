# Inline WebRTC source fork

The `m144_release` branch is Inline's maintained WebRTC source fork for the
revision corresponding to `LiveKitWebRTC` `144.7559.11`. Its upstream code
baseline is:

```text
39d2180660d43d2e1630e564e3afe1c1fb72746e
```

Unlike the stock baseline, the head of this branch is intentionally built and
linked by Inline's owned Apple XCFramework releases. The fork keeps WebRTC's
AudioEngine ADM and software audio-processing pipeline, while hardening the
macOS VPIO-off path for physical split routing and truthful runtime evidence.

## Supported delta

- When the selected physical input and output differ, create a process-private
  Core Audio aggregate whose output is the clock master and whose input uses
  drift compensation.
- Route the aggregate's flattened channels explicitly so duplex endpoints do
  not silently replace the selected input or output direction.
- Recreate the shared HAL graph transactionally, force local quiescence after a
  failed transition, and retain rollback or terminal cleanup ownership until
  native capture and playout are stopped.
- Reset WebRTC audio processing only after callbacks quiesce and before a new
  acoustic route resumes.
- Preserve physical capture timestamps and measured capture/playout delay
  through `FineAudioBuffer`, including cached capture-packet age.
- Export per-direction callback freshness, delay, and configured graph-format
  diagnostics through the Objective-C SDK.
- Enforce the process-wide no-VPIO policy and expose requested and effective
  software/platform processing state.

Historical tags `m144.7559.11-inline.4` through
`m144.7559.11-inline.7` retain an abandoned transient/typing-suppression
experiment for auditability. They are unsupported and must not be used for a
release. The maintained fork deliberately reverts that experiment; typing
suppression is a separate product gate and is not provided by this source line.

## Owned artifact chain

Releases must be built from an exact commit in this repository by an exact
commit in `inline-chat/webrtc-build`, then published immutably through
`inline-chat/webrtc-xcframework`. Release evidence must record both commits,
the hosted build run, the archive checksum, all packaged Apple slices, required
SDK symbols and headers, and the focused native audio tests. Inline and its
LiveKit fork must not change pins until that remote artifact resolves and
passes clean dependency builds.

## Current policy

- Do not restore the abandoned transient suppressor or macOS key-state
  plumbing without a new, independently measured design and validation gate.
- Keep the fork limited to behavior that cannot be implemented truthfully at
  the LiveKit or application layer.
- Preserve WebRTC ownership of transport, audio buffering, delay delivery, and
  software AEC/NS/AGC.
- Treat built-in microphone plus Bluetooth output quality, repeated physical
  route changes, two-process coexistence, AEC integrity, and terminal shutdown
  as physical release gates. If the aggregate architecture fails its explicit
  kill gates, replace it with a proper AUHAL WebRTC ADM rather than layering a
  second audio pipeline around WebRTC.
