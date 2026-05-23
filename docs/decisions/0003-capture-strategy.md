# 0003 - Capture Strategy

## Status

Accepted; Syphon portion superseded by [0004](0004-syphon-external-source.md)

## Context

The plug-in should be recordable and useful in external streaming or video tools. A true virtual camera or video-device output is larger than a normal audio plug-in feature and requires a separate system integration layer.

## Decision

For v0.1:

- Record the rendered display to `.mov` on macOS using AVFoundation.
- Mux audio passing through the plug-in into those recordings.
- Provide a separate `Sound Shape Source` window for Window Capture/source capture.
- Document true virtual camera, Syphon, or NDI output as a future option.

## Consequences

The v0.1 workflow works with normal macOS capture tooling without installing a driver, but it is not a first-class camera/video-source device.
