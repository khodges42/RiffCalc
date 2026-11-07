> Historical PACKRAT record, retained for provenance. [Current RiffCalc documentation](../../README.md). Build paths and milestone status below describe the earlier project.

# PACKRAT 0.2.0 validation / 2026-11-07

This is a tested Windows development build, not a claim of broad DAW release
qualification. No production DAWJ paths or existing plugin sources changed.

## Executed checks

| Gate | Result |
|---|---|
| MSVC 19.37, VS2022, Windows x64 Release | Standalone, actual VST3 bundle, CLI and tests built; 7/7 CTest suites pass |
| Windows x64 Debug | Same seven suites pass, including native processor/editor and actual VST3 host |
| Independent loaded VST3 | JUCE host scans/loads the binary, verifies stereo instrument buses, sample-offset keyboard MIDI, audible processing, state and editor lifecycle |
| pluginval 1.0.4 level 5, seed 42 | SUCCESS, with editor/state/automation, 44.1/48/96 kHz and 64/128/256/512/1024 frame blocks; separate Steinberg validator not supplied |
| Ubuntu/WSL GCC 11.4 | Final core/workstation/preview source compiles with C++20, optimization, all warnings as errors; complete workstation suite passes |
| Relocated source, Windows | Separate core-only CMake project builds without parent files; five CTest suites pass, including complete vendored source inventory |
| Existing Rust family regression | 87 tests pass; fmt, check, Clippy with warnings denied and 113-package dependency inventory pass |

Main build logs/artifacts are under the parent workspace's ignored
`target/packrat-app/`. The standalone project itself needs no parent path.

The musical core retains the original 1,440 root/scale/genre invariant tests and
four fixed MIDI/metadata SHA-256 fixtures. Workstation tests cover another 360
genre/scale/length combinations, related part ranges, exact document round trips,
recognizable harmony mutation, voicing/rhythm locks and unknown state rejection.

An independent Python decoder verifies Type 1 conductor/part ordering and all
four note tracks, as well as 32 Type 0 companion MIDI files in an eight-idea pack,
against their saved note events. Pack output collision protection is exercised.
The implementation also supports 16/32/64; those counts are not individually
claimed as measured throughput results.

Realtime tests exercise all ten preview sounds at three sample rates, stop/release,
numerical bounds, concurrent triple-buffer ownership and 64 document publications
while audio runs. Actual processor audio matches sample-for-sample across block
sizes 1, 17, 64, 511 and 1024 at 44.1/48/96 kHz. Test allocation counters observe
zero C++ `new` calls in processing, including a long MIDI message that is ignored
without constructing an allocated message. These are not a general malloc-hook
or sanitizer audit of every framework path. Code review confirms no document
locks, generation, file access, JSON or dynamic ownership in the callback.

## Native application and rendering

The actual standalone window was launched, captured, expanded using a native
button event, captured again and closed. Editor callback tests exercise Generate,
Mutate, history, play/stop, expanded/minimum layouts and editor destruction.
See [main view](images/packrat.png) and [Riff Machine](images/riff-machine.png).
The main view has a minimum 880 x 650; expanded minimum height is 800.
Standalone startup working set in this run was about 47 MiB (private about 15 MiB).
That is a single-machine observation, not a memory/CPU budget.

A 16-second 48 kHz stereo arrangement demo is produced at
`<build>/workstation-output/packrat-preview.wav`. An observed optimized offline
render took about 0.27 seconds including WAV writing (~1.7% of one core's realtime
budget for this phrase). It is not a worst-case polyphony measurement. The demo
has finite, nonclipping PCM; measured peak about -26.3 dBFS, RMS -39.8 dBFS at its
conservative test output setting. No subjective listening approval is claimed.

## Remaining checks and limits

- FL Studio end-to-end file dragging/import and GUI operation are not manually
  verified here. The independent VST3 host and pluginval establish a narrower
  boundary. Drag uses JUCE's real native file-copy operation; file export is the fallback.
- Preview follows valid host BPM optionally, with a local looping transport;
  it does not track host PPQ or automatically start with the host.
- MIDI output is SMF/drag only. No realtime VST MIDI generation bus is advertised.
- Preview timbres are lightweight original approximations, internally mono;
  arrangement parts share the selected audition sound. Melody remains experimental.
- Read-only piano roll, three stage locks, in-memory history and file-based idea
  management. See README for deliberately unsupported editing functions.
- Batch generation and filesystem work occur off audio, on the message thread;
  unusually slow disks can briefly pause the interface. Failed packs retain their
  partial new directory. Temporary drag files retain seven days of history.
- Linux GUI, Linux CMake preset and macOS builds are not validated. The Windows
  runtime is linked statically; no Visual C++ runtime installer is invoked.
- Pluginval is an external development tool, not a PACKRAT runtime dependency.
  Source and dependency hashes are checked separately from Cargo's inventory.

Next useful review: load the packaged VST3 in FL Studio, audition the fixed ideas,
drop each part and an arrangement into FL, verify tempo/bar interpretation, save
and reopen a host project, then exercise the standalone's chosen audio device.
No global installation, source control commit or push was performed.
