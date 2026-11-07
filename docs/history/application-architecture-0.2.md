> Historical PACKRAT record, retained for provenance. [Current RiffCalc documentation](../../README.md). Build paths and milestone status below describe the earlier project.

# PACKRAT native workstation

PACKRAT is a self-contained CMake project. Copy this directory, including its
licenses, resources and vendored dependencies, to extract it later. It neither
links to the Rust plugin family nor changes DAWJ. Core-only builds remain possible.

The existing `packrat-1` chord algorithm and MIDI fixtures remain unchanged.
`packrat-workstation-1` adds related bass/arp/melody parts, selective mutation,
locks, versioned documents and Type 1 arrangement export. Generation, validation,
serialization and filesystem work happen outside the audio callback.

JUCE 7.0.12 (GPLv3 alternative) provides the Windows standalone and VST3 wrappers
and one native beige/grey editor. No browser, curl, analytics, ASIO SDK, sample
library, runtime download or service is used. The preview instrument is a separate
fixed-voice component, replaceable without changing the music engine.

Prepared clips contain bounded arrays of note events. A three-slot SPSC mailbox
transfers ownership from a serialized document producer to the audio consumer.
The callback never touches the document mutex, JSON, filesystem or musical
generator, and never owns containers with heap destructors. It consumes events,
reads atomic playback controls and writes audio. Preview is a local looping
transport; valid host tempo can optionally override its tempo. Starting preview
always starts at tick zero. Host transport itself does not start playback.

MIDI drag creates real temporary SMF files on the message thread, retained for
seven days for asynchronous DAW imports. File export is always available. Native
DAW drag behavior requires host-specific manual verification. The VST3 is a
stereo instrument with MIDI keyboard input; generated MIDI is exported as files,
not streamed to the host. There is no production DAWJ integration.

Risk gates: exact clip/state restoration, malformed input validation, independent
VST3 load/process tests, allocation-free preview, stop/loop behavior and native
window exercise. Preview timbres are audition approximations, not VIXEN patches
or a high fidelity piano. Automated tests do not prove musical quality.

## Source boundaries

| Location | Responsibility |
|---|---|
| include/packrat + src/core, src/style, src/midi | Portable theory, weighted harmony, voicing, profiles, legacy SMF/CLI algorithm |
| src/workstation | Related parts, selective mutation, locks, full documents, Type 1 and batch export |
| src/audio | Replaceable fixed-capacity preview player/synth and triple-buffer mailbox; no JUCE |
| src/app/Processor.* | JUCE instrument buses, host parameters, state envelope and event bridge |
| src/app/Editor.* | Shared standalone/plugin native editor, file dialogs, MIDI drag and bounded history |
| tests | Core invariants, separate MIDI decoders, audio allocation/timing tests and independent VST3 loading |
| scripts | Offline build, source hash verification, complete-source packaging |

The factory profiles and saved documents are product-local. Settings and MIDI drag
cache paths are PACKRAT-specific, without VIXEN/DAWJ brands or path assumptions.
CMake presets default to this directory's ignored out/; explicit build directories
work too. LICENSE/NOTICE are local. The packager includes this entire source tree
and all pinned upstream notices. A relocated core-only build is validated.

Only generation/config/notes are integer-tick deterministic. Audio is a lightweight
floating-point audition renderer; sample-identical output across compilers is not
promised. Related tracks use separate random streams. Mutation takes a previous
candidate as explicit input; exact notes are saved so restoring a variation never
requires reconstructing its historical parents. Fresh generation preserves the
legacy packrat-1 algorithm; application transformations are versioned separately.

Future extraction should move packrat/ intact, then change only the family README
links. No shared Cargo components, helper executables or parent license paths
need to be carried along. Native Linux/macOS packaging remains separate work.
