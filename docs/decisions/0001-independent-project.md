# 0001 - Independent Project

## Status

Accepted

## Context

The Oscilinea plug-in is inspired by the shape view inside BasicOsc, but it needs its own code, docs, build products, and plug-in identity.

## Decision

Create Oscilinea as an independent JUCE/CMake project. BasicOsc may be read as reference, but changes for Oscilinea land only in this project folder.

## Consequences

- Oscilinea can ship with a focused feature surface and smaller UI.
- Plug-in identity and parameters are independent from BasicOsc.
- Shared improvements may need to be ported manually unless a shared library is created later.
