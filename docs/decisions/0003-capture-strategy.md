# 0003 - Capture Strategy

## Status

Accepted

## Context

The plug-in should be recordable and useful in OBS-like tools. A true virtual camera or video-device output is larger than a normal audio plug-in feature and requires a separate system integration layer.

## Decision

For v0.1:

- Record the rendered display to `.mov` on macOS using AVFoundation.
- Provide a separate `Sound Shape Source` window for OBS Window Capture.
- Document true virtual camera, Syphon, or NDI output as a future option.

## Consequences

The v0.1 workflow works with normal macOS tools and OBS without installing a driver, but it is not a first-class camera/video-source device.
