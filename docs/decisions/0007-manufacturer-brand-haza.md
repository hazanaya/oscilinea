# 0007 - Manufacturer Brand "Haza"

## Status

Accepted

## Context

DAWs group plug-ins by their manufacturer display name (Logic's AU menu folder,
the VST3 vendor column). The project originally used the full personal name
"Hazael Anaya". No beta has ever been distributed, so the display name was
still free to change; once a build reaches users it should stay fixed, because
every future plug-in must carry the identical string to land in the same menu
folder.

## Decision

Set `COMPANY_NAME` to "Haza" for Oscilinea and all future plug-ins. It is
short, distinctive, and one word, like established plug-in brands (Moog,
Arturia, Valhalla).

The display name is branding only. These identifiers are unchanged and remain
frozen per [0002](0002-plugin-identity.md):

- `PLUGIN_MANUFACTURER_CODE Hzny` and `PLUGIN_CODE Sshp`
- `BUNDLE_ID com.hazaelanaya.oscilinea` (matches the hazaelanaya.com domain)

The Apple Developer ID certificate still reads "Jorge Hazael Anaya"; installer
trust dialogs show the legal cert name and cannot be rebranded. That is normal
for individual accounts.

## Consequences

- Hosts list the plug-in as "Haza: Oscilinea" / vendor "Haza".
- Every future plug-in must use `COMPANY_NAME "Haza"` exactly.
- After the first beta ships, this string is frozen alongside the identity
  codes.
