# 0002 - Plug-in Identity

## Status

Accepted

## Context

AU/VST hosts persist plug-in and automation state using plug-in codes, bundle IDs, and parameter IDs. Changing those values after release breaks existing sessions.

## Decision

Use:

- `PLUGIN_MANUFACTURER_CODE=Hzny`
- `PLUGIN_CODE=Sshp`
- `BUNDLE_ID=com.hazaelanaya.oscilinea`
- `PLUGIN_NAME=Oscilinea`

Treat parameter ID strings as immutable once released.

## Consequences

The product/marketing name and host identity should stay stable once Oscilinea is distributed to beta users.
