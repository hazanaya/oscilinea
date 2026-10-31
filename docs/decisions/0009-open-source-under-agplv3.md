# 0009 - Open Source Under AGPLv3; the Pedal Stays Haza's Option

## Status

Accepted (Haza, 2026-10-31). Publication itself is a separate step done at the
desk; see Consequences.

## Context

Oscilinea has no monetization plan. The beta is listed as "price: Free" and had
two downloads in its first three months (ADR 0008). Its risk is obscurity, not
theft. In the free plug-in world, open source is the one lever that reliably
changes that: people can read, trust, fork, package, and talk about it.

Haza's hesitation came from an older idea. Oscilinea was the flagship of a
hardware concept, a guitar pedal with a screen that shows the signal as one
line, several of them along the pedal chain so you can see what each pedal does
to the sound. That idea is called **Ojitos** ("little eyes"); it is one pedal in
a larger, not yet named pedal line (working candidate "Pa' las patas"): analog
recreations of the classic effects with LFO control and patchable control
signal. Both are written down in Notion (Springwood / INTRAVER / Software
Studio / Projects, pages "Ojitos" and "Pedal line (unnamed)"). Haza is not
ready to pursue the hardware and did not want to simply give the idea away.

Licensing facts that shaped the decision:

- JUCE 8.0.12 (`extern/JUCE`) is dual-licensed AGPLv3 or commercial. An open
  release of a JUCE 8 plug-in must therefore be AGPLv3 (or a compatible
  stronger copyleft), not MIT or BSD.
- Syphon-Framework (`extern/Syphon-Framework`) is under a BSD-style license
  that permits redistribution with attribution; it does not allow using the
  Syphon name to endorse derived products.
- Haza is the sole author of every commit (45 commits, one author). No outside
  contributions exist, so Haza alone decides the license and keeps the right to
  relicense his own code later.
- The repository history contains no secrets (scan 2026-10-31: no keys,
  tokens, or credentials; signing keys live in `~/oscilinea-signing/`, outside
  git; `scripts/release.config` is ignored).

## Decision

1. **Oscilinea's source is released under the GNU Affero General Public License,
   version 3** (`LICENSE` holds the verbatim text). Copyright stays with
   Jorge Hazael Anaya.
2. **The name and logo are not licensed.** `TRADEMARKS.md` says so: forks must
   use their own name and icon, and may not present builds as official.
3. **Haza stays the sole copyright holder.** Outside contributions, if any ever
   arrive, are accepted only with a short written agreement that lets Haza
   relicense the combined work (a contributor license agreement). Without that,
   each merged contribution would be AGPL-only and would narrow Haza's freedom
   to reuse the code in a closed product.
4. **The pedal is unaffected and stays Haza's option.** Open-sourcing the plug-in
   does not give the pedal away:
   - The idea is already public: anyone who downloads the plug-in sees exactly
     what it does.
   - A pedal is a rewrite on embedded hardware (small chip, small screen, no
     macOS, no JUCE, no Syphon). What carries over is the period-locking
     trigger approach and the look, both authored by Haza and reusable by him
     in closed firmware under point 3.
   - AGPLv3 is copyleft: anyone who ships a product built from this code must
     publish their source too. Nobody can take Oscilinea's code closed into a
     pedal of their own.
5. **Publishing is a one-way door and happens at the desk**, not over the
   phone, folded into the launch work so that the KVR and Reddit text
   (HANDOFF launch chunk 3) says "free and open source" from the start.

## Consequences

- `LICENSE` is AGPLv3; `TRADEMARKS.md` is new; README gains a License section
  naming AGPLv3, JUCE's AGPLv3 use, Syphon's BSD terms, and the trademark note.
- `HANDOFF.md` gains a "Publish the source" chunk ahead of the listing text:
  reconcile the local history with the public README-only repo
  (`hazanaya/oscilinea`), push, turn on the license display, decide whether
  issues stay enabled, and update the website page.
- Per-file license headers are optional under AGPLv3 and are not added now; the
  top-level `LICENSE` governs the whole tree.
- Precedent noted for comfort: Mutable Instruments ran a decade on fully open
  hardware and firmware, was cloned widely, and sold fine; the clones never had
  the brand.
- Reversal: Haza can stop publishing new versions under AGPLv3 at any time
  (point 3 keeps that door open), but versions already published stay AGPLv3
  forever.
