# Contributing to RiffCalc

Build with the README instructions and run CTest before submitting a focused change.
Report a version, platform/host, seed and configuration or saved idea, expected
result and repeatable steps. Musical taste and a numerical correctness defect are
different reports; both are useful when clearly described.

Keep the portable engine separate from JUCE. Preserve legacy state/plugin IDs and
`packrat-1` reference exports; algorithm changes require explicit versioning and
fixtures. No allocation, blocking, filesystem work or procedural generation belongs
in audio processing. UI changes should use ClassicLook primitives and functional
commands, with real screenshots when practical. Test resizing and keyboard entry.

Retain original copyright/SPDX notices. Original contributions use
GPL-3.0-or-later; dependency grants remain unchanged. Do not add proprietary MIDI
packs, samples, third-party logos, fonts or artwork without compatible licensing.
No signed contributor agreement or authorship transfer is requested.

Use ignored out/ for builds, renders, profiling and temporary packages. Check the
source inventory with `python scripts/check_vendor.py`. Do not regenerate frozen
hashes merely to make a regression test pass. See AGENTS.md and docs/development.md.
