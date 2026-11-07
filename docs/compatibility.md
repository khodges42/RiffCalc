# Compatibility contract

RiffCalc is the new product name; the following legacy identifiers are deliberately
retained rather than disguised with a global text replacement.

| Interface | Preserved value |
|---|---|
| JUCE manufacturer/plugin codes | `Pkrt` / `Pkr1` |
| Bundle identifier | `audio.packrat.workstation` |
| VST3 component/controller identity | Fixed codes with `JUCE_VST3_CAN_REPLACE_VST2=0`; Windows IDs `5bd65fc7` / `38bf9881` verified against v0.2 |
| Automation parameters | `preview`, `hostTempo`, `output`, `tempo`, `sound`; same ranges/version hints |
| Chord algorithm | `packrat-1` |
| Workstation serialization | Schema 1, `packrat-workstation-1` |
| Portable C++ engine API | `packrat::`, headers under `include/packrat` |
| MIDI timing, notes and metadata | Original Type 0 golden hashes; 960 PPQ; same track names and MIDI channel assignments |

The four original reference MIDI/JSON fixtures remain unchanged. The actual
PACKRAT 0.2 document in tests/fixtures is restored by the native processor test
and compared for exact musical/configuration equality. The rebrand changes no
musical generation or preview DSP source. Velocity edits are explicit user actions.

Open accepts both `.riffcalc.json` and `.packrat.json` ideas. Newly suggested names
use RiffCalc. Their payload uses the same schema; the older application can parse
a compatible RiffCalc document after choosing/renaming its file appropriately.
The full note snapshot, not the current factory bank alone, restores a variation.

Standalone preferences now live in the product's own application-data directory:
`Voidmutt Enterprises/RiffCalc/last-session.riffcalc.json`. If absent, the program
reads the legacy `PACKRAT/last-session.packrat.json` as a fallback. It does not write
back to that old location. Demonstration mode neither loads nor saves preferences.

The new VST3 filename and visible manufacturer/name are intentional. Rescan the
new bundle; hosts with a cached file path may require locating the replacement.
Because both versions have the same IDs, keep the intended replacement version
available to your scanner rather than expecting two distinct plugin identities.
No claim is made that every DAW repairs a missing cached filename automatically.

The deprecated `PACKRAT_BUILD_APP` CMake option maps to `RIFFCALC_BUILD_APP` when
the new option is not supplied. Executable/package names and public branding are
new; no old application binary is silently installed or replaced.
