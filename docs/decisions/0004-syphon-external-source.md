# 0004 - Syphon External Source

## Status

Accepted

## Context

The source window works with ordinary Window Capture workflows, but users may want each plug-in instance to appear as a direct live visual source in macOS video tools. A true virtual camera is heavier than an audio plug-in feature and requires a separate system device or driver layer.

## Decision

Add a macOS-only Syphon External Source output:

- Pin `Syphon-Framework` as a source submodule.
- Compile only the server-side Syphon OpenGL source into the plug-in instead of depending on a private app-bundled framework.
- Start and stop Syphon per plug-in editor from the Record row.
- Publish frames from the existing editor vblank loop at the selected capture FPS.
- Keep Source Window as a fallback for tools or workflows that do not use Syphon.

## Consequences

- Multiple open plug-in editors can publish separate Syphon sources such as `Oscilinea 1` and `Oscilinea 2`.
- Syphon publishing remains off the audio thread and shares the same rendered display path as recording/source-window capture.
- External Source stops when the plug-in editor closes; keeping a source alive without an editor would require a separate headless render loop owned outside the editor.
- OpenGL is used only as the Syphon bridge because Syphon's OpenGL server can be compiled from source with command-line tools. A future Metal bridge could reduce deprecated API use if the project adopts a Metal render path.
