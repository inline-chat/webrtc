# Inline WebRTC source baseline

The `m144_release` branch is Inline's maintained source mirror for the WebRTC
revision corresponding to `LiveKitWebRTC` `144.7559.11`.

The code tree intentionally matches this standard source revision:

```text
39d2180660d43d2e1630e564e3afe1c1fb72746e
```

Inline currently consumes LiveKit's official `webrtc-xcframework` binary at
version `144.7559.11`; it does not build or link this checkout. This repository
exists so future WebRTC changes can be reviewed and maintained explicitly if a
source fork ever becomes necessary.

## Current policy

- Do not restore the removed transient suppressor or macOS key-state plumbing.
- Do not patch WebRTC for application-owned route selection or Core Audio
  lifecycle coordination.
- Prefer standard WebRTC behavior and narrow LiveKit SDK APIs first.
- If a WebRTC source change becomes unavoidable, document the upstream base,
  keep the delta minimal, add native tests, build every packaged Apple slice,
  and publish a new immutable XCFramework version before changing Inline's
  dependency pin.
