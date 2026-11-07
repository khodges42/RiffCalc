> Historical PACKRAT record, retained for provenance. [Current RiffCalc documentation](../../README.md). Build paths and milestone status below describe the earlier project.

# PACKRAT / Milestone 1 architecture

Historical Milestone 1 record. The native workstation is described in
[PACKRAT application docs](../../README.md).

## Repository inspection and scope

Reviewed VSTs/AGENTS, README, architecture, patch conventions, Cargo workspace,
standalone DSP cores, independent hosts and the recent FL Studio crash follow-up.
The repository is an existing GPL-3.0-or-later Rust plugin family, not an empty
JUCE project. Existing Rust/Cargo builds and DAWJ playback remain separate.

PACKRAT lives in `packrat/`, an isolated C++20/CMake subproject. This honors the
requested C++ direction without adding a C++ dependency to the existing audio
plugins. Windows/MSVC is the measured target. Portable standard-library code and
Linux CMake presets are included. Source/CLI tests also passed using GCC on
Ubuntu/WSL; Linux CMake and macOS execution require separate evidence.

**Stop after Milestone 1:** a real command-line chord generator, selectable key,
ten scales, twelve original genre profiles, 1–16 bars, deterministic seeds,
bounded voicings, SMF Type 0 export and automated tests. No GUI, audio preview,
VST3 wrapper, MIDI dragging, mutation/locking UI or batch pack interface is claimed.
JUCE is unnecessary for this milestone and will be evaluated/pinned before M2.

## Modules

| Module | Responsibility |
| --- | --- |
| `include/packrat/music.hpp`, `src/core/music.cpp` | Pitch/spelling, scale and chord formulas, integer musical time, clips and validation |
| `include/packrat/generator.hpp`, `src/core/harmony.cpp` | Config, explicit SplitMix64 sampling, stage seeds, weighted harmonic grammar and cadence |
| `src/core/voicing.cpp` | Candidate inversions/registers and deterministic dynamic-programming voice leading |
| `src/core/generator.cpp` | Rhythmic phrase construction, bounded humanization and velocity |
| `include/packrat/profiles.hpp`, `src/style/profiles.cpp` | Strict bounded JSON loading and declarative profile validation |
| `include/packrat/midi.hpp`, `src/midi/midi.cpp` | SMF serialization and safe create-new export, independent of generation |
| `src/cli/main.cpp` | Offline argument parsing, summaries, MIDI/metadata output |
| `resources/genres.json` | Original harmonic/rhythmic/style data, versioned separately from algorithms |

There is no JUCE/platform/audio dependency in generation or MIDI encoding. The
JSON library is private to profile/metadata I/O, not exposed by public engine
types. The only vendored library is pinned nlohmann/json 3.12.0 (MIT), retaining
its header, license and checksums. Builds do not fetch anything. Bundled profiles
are embedded from the same JSON file by CMake; custom profiles use the same loader.

## Musical and deterministic contracts

- 960 integer ticks per quarter, 4/4 generation, exactly `bars * 3840` clip ticks.
  All notes have valid MIDI pitches/velocities/channels and positive durations;
  repeated notes on a channel never overlap. Event validation precedes export.
- Weighted phrase templates plus weighted scale-degree transitions provide
  recognizable repetition and controlled harmonic changes. Cadence can resolve
  to tonic or favor a loop ending. Modal profiles reduce functional cadence.
- Chord formulas are explicit. Strict mode filters every chord tone against the
  selected scale. Pentatonic harmony uses natural-minor degree names but restricts
  actual pitches to the five selected classes; if a template is unavailable,
  the legal weighted transition graph supplies the phrase. Chromatic dominant
  color is an explicit opt-in, labeled in results; accidental chromaticism is not
  used to increase complexity.
- Complexity changes extension eligibility, harmonic variation and rhythmic
  vocabulary. Genre data changes harmony, voicing and rhythm, not preview sound.
- Voice-leading enumerates bounded close/drop voicings and minimizes movement
  across the phrase using integer costs. This is practical open-phrase voice
  leading, not full contrapuntal or cyclic optimization.
- SplitMix64, unsigned wrap, rejection-based bounded sampling, canonical iteration
  order and integer weights are specified in code. Separate harmony, voicing,
  rhythm, timing and velocity streams prevent random draw-count coupling.
  Output identity requires the same algorithm version, exact profile data,
  config and seed. Exported metadata includes these inputs and stage seeds.
- Generate only on the caller's non-audio thread. M1 intentionally allocates for
  candidates, phrases and serialization; no real-time safety claim is made for
  generation. Future preview will receive immutable prepared clips with explicit
  lifetime management, rather than calling generation inside an audio callback.

## Export and risks

Type 0 SMF, one named track, no running status, 960 PPQ, tempo/time-signature
metadata, note-off before note-on at equal ticks, and end-of-track at the exact
clip length. Tempo is encoded in integer microseconds per quarter. The CLI also
writes a reproducibility JSON sidecar. Create-new writes protect existing files;
bad configs/profiles must fail before creating an output. No DAW import/drag
compatibility claim follows from byte-level MIDI validation.

Primary risks: subjective genre quality; sparse scale/chord compatibility;
extended-chord register feasibility; accidental random-stream coupling; MIDI
event ordering and final length; portable export ownership/overwrite behavior;
algorithm/profile changes breaking old seeds. Tests and fixed-seed fixtures cover
objective properties. Manual MIDI audition remains necessary. M2/M3 work must
preserve versioned inputs or introduce an explicit algorithm migration.
