> Historical PACKRAT record, retained for provenance. [Current RiffCalc documentation](../../README.md). Build paths and milestone status below describe the earlier project.

# PACKRAT / Generative MIDI Workstation

Milestone 1: offline C++20 chord generator, CLI and Standard MIDI File export.
No audio preview, GUI or VST3 is implemented yet. The musical engine has no JUCE
dependency and does not use the existing Rust plugins at runtime.

## Build and test

Windows x64 requirements: Visual Studio 2022 Desktop C++ tools, Windows SDK,
CMake 3.25+ and Python 3.10+ (preset format 6).
There are no downloads during configure, build, tests or runtime. The JSON header
and license are vendored and verified by hash.

From the VSTs repository root (the helper also finds Visual Studio's bundled CMake):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File packrat/scripts/build.ps1
# Optional checked Debug configuration:
powershell -NoProfile -ExecutionPolicy Bypass -File packrat/scripts/build.ps1 -Configuration Debug
```

Or from `packrat/`, with CMake/CTest on PATH:

```powershell
cmake --preset windows
cmake --build --preset windows-release
ctest --preset windows-release
```

Portable Linux CMake preset, **not executed in this validation**:

```sh
cd packrat
cmake --preset linux
cmake --build --preset linux-release -j
ctest --preset linux-release
```

Core/CLI source and tests were also compiled directly with GCC 11.4 on existing
Ubuntu/WSL, including matching Windows golden hashes. CMake was not installed
there, so the Linux CMake preset remains unverified; see the
[validation record](validation.md).

No system installation is performed. Release executable from the repository root:
`target/packrat/windows/Release/packrat.exe`. CMake also provides a conventional
optional `install` target for a user-chosen prefix; installation is not needed.

## Generate

```powershell
New-Item -ItemType Directory -Force target/packrat/my-midi | Out-Null
./target/packrat/windows/Release/packrat.exe list
./target/packrat/windows/Release/packrat.exe generate --key C --scale minor --genre liquid-dnb --bars 4 --complexity 70 --seed 42 --out target/packrat/my-midi/liquid.mid
./target/packrat/windows/Release/packrat.exe generate --key 'F#' --scale harmonic-minor --genre trance --bars 8 --seed 918 --cadence resolve --out target/packrat/my-midi/trance.mid
./target/packrat/windows/Release/packrat.exe generate --key Eb --scale dorian --genre uk-garage --bpm 132 --humanize 30 --seed 777 --out target/packrat/my-midi/garage.mid
```

Defaults: C natural minor, Liquid Drum & Bass, four bars, complexity 50, seed 1,
48–84 MIDI pitch range, velocity 88, genre-default BPM and no random timing jitter.
Root spellings accept A–G with an optional sharp/flat; all twelve chromatic roots
and enharmonic spellings are available. MIDI 60 is C4. Scales:

`major`, `minor` / `natural-minor`, `harmonic-minor`, `melodic-minor` (ascending
form), `dorian`, `phrygian`, `mixolydian`, `lydian`, `locrian`, `minor-pentatonic`.

Genres: `liquid-dnb`, `jungle`, `uk-garage`, `house`, `trance`, `hard-dance`,
`breakcore`, `synthwave`, `industrial`, `ambient`, `dubstep`, `acid`.

`--help` lists every implemented option. `--low`/`--high` constrain all voicing
notes to 0–127 and must span at least an octave. Bars are 1–16, BPM 20–400,
velocity 1–127, complexity 0–100 and humanization 0–60 integer ticks. Seeds are
unsigned decimal 64-bit integers, including 0 and 18446744073709551615.

The output directory must exist. MIDI and its `.json` sidecar are created
exclusively; existing paths are never overwritten. If creating the sidecar fails,
the newly owned MIDI file is removed and existing files remain intact. This is
ordinary filesystem error handling, not a power-loss-safe two-file transaction.
Unicode output filenames are tested on Windows. No cloud, accounts, telemetry,
network client, AI inference or proprietary MIDI/sample packs are included.

## What affects the phrase

Genre profiles contain weighted progression templates, a seven-degree transition
matrix, cadence weights, preferred qualities/scales/tempo, register/spread,
phrase repetition, rhythmic motifs, swing and velocity articulation. They are
original curated starting data, not transcriptions of a commercial chord pack.
Future preview sound names are descriptive profile hints only; there is no
current sound selector or preview synthesizer.

Generation uses the selected scale, not an automatic scale override from the
genre. Preferred scales and tempos are displayed by `list`. Complexity makes
seventh/sixth/add9/ninth structures eligible, enables denser rhythmic motifs and
increases controlled harmonic substitutions. Low complexity remains structured;
it is not simply a lower random-number multiplier.

Strict mode checks every chord pitch against the chosen scale. Pentatonic mode
uses natural-minor degree numbering while allowing only its five pitches.
Unavailable templates fall back to legal weighted transitions. Locrian's tonic
may be diminished; modal generation does not falsely turn it into major/minor.

`--cadence resolve` ends on a tonic chord. Default `loop` uses the profile's loop
cadence weights; it can finish on tonic, dominant or another weighted degree.
Voicings minimize weighted voice movement across the phrase, including inversion
and register selection. Drop voicings and extended-chord density vary by profile.
The solver optimizes the open phrase, not the loop seam or complete counterpoint.

`--colour` explicitly permits a leading-tone V7 cadence outside the selected
scale when the profile allows it and complexity is at least 30. Such chords are
marked `*` in roman analysis and `chromatic: true` in metadata. The altered 7(b9)
formula requires this option and high complexity, and is used only when its
tones fit the selected scale. There is no uncontrolled random chromatic note
insertion. Other borrowed-chord grammars are future work.

Sixteen explicit chord formulas are available: major/minor/diminished/augmented,
sus2/sus4, dominant7/major7/minor7/minor-major7, minor9/major9, add9,
major6/minor6 and dominant7-flat9. Chord spelling follows diatonic letter roles,
including E#/double accidentals when appropriate; MIDI stores sounding pitches.

## Profiles and reproducibility

`resources/genres.json` is the authoritative profile bank. CMake embeds it in
small source chunks to respect MSVC string-literal limits. The resulting CLI is
relocatable and does not need its source tree at runtime. Edit the JSON and rebuild,
or load a custom bank with `--profiles FILE` using the same schema/validator.

The loader rejects unknown/missing fields, duplicate JSON keys/genre IDs,
unsupported schema, invalid enums, fractional integers, out-of-range weights,
unsorted/duplicate rhythm steps, inconsistent tempo and oversized/deep JSON.
Banks are bounded to 256 KiB, 64 genres and nesting depth 32. Every transition
row and cadence requires a nonzero tonic fallback; one rhythm must be available
at complexity zero. Useful profile-index errors are printed by the CLI.

Algorithm identifier: **packrat-1**; bank schema **1**; each profile has its own
version. Generation uses specified SplitMix64 unsigned arithmetic and unbiased
rejection sampling, with separate harmony/voicing/rhythm/timing/velocity streams.
Iteration order, integer weights and integer voice-leading costs are fixed.
Changing velocity or humanization does not perturb the harmony/voicing streams.
Mutation, selected-stage regeneration and user-visible locks are **not** M1 features.

Metadata records the key/scale, genre/version, algorithm, seed, all config values,
stage seeds, chord analysis/voicings and note events. Seeds are decimal strings
in JSON to preserve all 64 bits in consumers with limited numeric precision.
A canonical-JSON **FNV-1a 64-bit** bank fingerprint identifies profile data across
whitespace/line-ending changes; it is a reproducibility identifier, not a security
hash. The four fixed fixtures additionally freeze SHA-256 hashes of complete
MIDI and metadata files. Preserve old data/algorithm versions when changing rules.

## MIDI format

The encoder accepts the internal clip independently of generation. M1 writes:

- Standard MIDI File **Type 0**, one track; 960 ticks per quarter note.
- Generation in **4/4**, one harmonic chord per bar; genre motifs can retrigger
  that chord. Total length is `bars × 3840` ticks.
- Tick-zero track name (`FF 03`), tempo (`FF 51`, nearest integer microseconds
  per quarter), and time signature (`FF 58`, metronome 24, thirty-seconds 8).
- Explicit note-on/off messages, no running status. At equal ticks, note-offs
  precede note-ons; pitch/channel order is stable.
- Positive note lengths, no same-channel/pitch overlaps, no unclosed notes.
  End-of-track (`FF 2F`) is at the exact clip endpoint, including empty clips.
- Four-byte maximum VLQ deltas and correctly sized big-endian header/track chunks.

The MIDI API can serialize other validated power-of-two meters; M1 generation
and CLI intentionally use 4/4. Type 1 multitrack export belongs with M3's related
parts. DAWs may infer loop lengths differently despite the explicit end marker;
**DAW import and drag-and-drop compatibility have not been tested.**
References: [MIDI Association SMF overview](https://midi.org/standard-midi-files),
[Microsoft's MIDI timing documentation](https://learn.microsoft.com/en-us/windows/win32/multimedia/timing-information).

## Fixed examples and validation

```powershell
python packrat/scripts/examples.py target/packrat/windows/Release/packrat.exe target/packrat/audition
```

This writes the four fixed fixtures plus a small manifest. It refuses existing
outputs; it is a regression/audition helper, not Pack Factory. Import these into
an instrument of your choice for listening. No audio files are supplied because
preview synthesis is outside this milestone.

Tests cover scale/chord spelling, intervals, PRNG golden vectors, reproducibility,
stage separation, profile/config errors, voice-leading movement, all **1,440
root × scale × genre combinations**, all bar lengths, edge registers, complexity,
explicit chromatic colour, empty/one-tick MIDI clips and long VLQs. A separate
Python SMF decoder round-trips the generated files against their note metadata,
including tempo, meter, ordering, Unicode paths, overwrite protection and four
fixed seed hashes. Debug/Release use the same golden files. Tests do not establish
that a phrase sounds good; manual musical review remains necessary.

## Next milestone

M2 should add the shared JUCE standalone/plugin UI, piano roll, immutable prepared
clip publication, lightweight preview, configuration persistence and native MIDI
dragging. Evaluate/pin JUCE licensing before inclusion and independently test
VST3/drag behavior in the target hosts. Bass/arp/melody, independent stage locks,
Type 1 arrangements and Pack Factory remain M3/M4. No production DAWJ integration
or changes to its session formats are included.

Original PACKRAT source and genre recipes: **GPL-3.0-or-later**, under the root
LICENSE/NOTICE. The one vendored dependency is
[nlohmann/json 3.12.0](https://json.nlohmann.me/home/releases/v3.12.0/), MIT,
with the original license and exact-source SHA-256 manifest under
`third_party/nlohmann/`. Builds need no JUCE, VST3 SDK or external test framework.
