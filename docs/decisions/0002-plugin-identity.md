# 0002 - Plug-in Identity

## Status

Accepted

## Context

AU/VST hosts persist plug-in and automation state using plug-in codes, bundle IDs, and parameter IDs. Changing those values after release breaks existing sessions.

## Decision

Use:

- `PLUGIN_MANUFACTURER_CODE=Hzny`
- `PLUGIN_CODE=Sshp`
- `BUNDLE_ID=com.hazaelanaya.soundshape`
- `PLUGIN_NAME=Sound Shape`

Treat parameter ID strings as immutable once released.

## Consequences

The product/marketing name can change later, but the host identity values should not.
