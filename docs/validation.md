# RiffCalc 0.3.0 validation ? 2026-11-07

Windows x64 development build. This record distinguishes executed checks from
host-specific work still requiring review. Nothing was installed system-wide.

## Build and regression gates

| Check | Result |
|---|---|
| Independent repository build | MSVC 19.37 / VS2022, CMake 4.2.3, Windows SDK; standalone, VST3 and CLI build from RiffCalc alone |
| Release CTest | 8/8 pass: portable core, CLI/MIDI decoder, docs/assets, vendored hashes, workstation, arrangement decoder, native processor/editor and loaded VST3 |
| Debug CTest | 8/8 pass; same core/audio/native-host gates; final record is in ignored out/debug-tests.log |
| Musical regression | Original 1,440 root/scale/genre cases and four frozen MIDI/metadata SHA-256 fixtures pass |
| Workstation regression | 360 genre/scale/length cases, deterministic parts/ranges, mutations, locks, exact documents and batch collision protection pass |
| MIDI decoding | Independent Python reads Type 0 and Type 1 exports against exact notes, including conductor/track metadata and 32 batch companion files |
| PACKRAT saved state | Actual v0.2 fixture restored by native processor, compared for exact document equality |
| VST3 identity | Both old/new loaded bundles report unique/deprecated IDs 5bd65fc7 / 38bf9881 on Windows; host regression now guards these |
| pluginval 1.0.4 | Strictness 5, seed 42: SUCCESS; actual VST3/editor/state/automation checks |
| Existing VST family | 87 Rust tests and the 113-package dependency inventory pass after cleanup |
| Native editor | Menus, event editing/history, shortcut command, generated candidates, play/stop, minimum/large layouts and real desktop About peer tested |
| Actual native screenshots | Real Release standalone with deterministic demo seed 1999, C minor, Liquid DnB, complexity 75; main/events/properties/About captured and inspected |
| Dependency integrity | Original JSON header/license and 3,592 unmodified JUCE source/notice files verified |

The original eleven engine/preview/MIDI/profile implementation/data files match
the migration's pre-change SHA-256 inventory. No factory recipe or audio algorithm
was changed to produce the new screenshots. Explicit velocity edits change only
user-selected note events and create history entries.

The processor continues to produce identical samples across block sizes
1/17/64/511/1024 at 44.1/48/96 kHz. Fixed-array preview tests cover ten sounds,
stop/release, finite bounded output and concurrent mailbox ownership. Native
processor tests publish 64 documents while audio runs and observe zero C++ new
calls in processing, including an ignored long MIDI message. This is not a general
malloc-hook/sanitizer audit of every third-party framework path.

The independent JUCE VST3 host scans and loads the actual bundle, tests stereo
instrument buses, note input at a nonzero sample offset, audible generated preview,
state and native editor construction. Pluginval 1.0.4 strictness 5, seed 42 is run
as a separate external check; see ignored out/pluginval logs for its actual result.
A separate Steinberg validator is not supplied or claimed.

## Visual and interaction review

The screenshot pass identified and corrected title text behind the standalone
Options button, a crowded group caption, and an About dialog missing its desktop
peer. The regression now checks that About has a real desktop window. Shared
primitives provide bevels, checkboxes, toolbars, group frames and paneled status.
Title bars remain gray when inactive, matching normal classic window behavior.

Keyboard command tests cover seed regeneration and existing generation/history/
preview workflows; controls use JUCE focus/Tab navigation with explicit order.
Native injected mouse/key messages were unreliable in this environment, so they
are not reported as proof of a human keyboard session or completed native drag.
The capture helper is capture-only. Demonstration flags select actual application
content and do not load/save preferences. See screenshots/README.md.

## Remaining manual checks and limitations

- FL Studio MIDI drop/import and reopening a real FL project require manual
  testing there. Native drag uses the existing JUCE file-copy mechanism; conventional
  MIDI export remains the fallback. No DAW was manipulated during this migration.
- Verify keyboard/focus and high-DPI appearance with the intended monitor and DAW.
  Native windows were visually inspected at the capture machine's current DPI.
- Preview uses a local looping transport; optional host BPM does not imply PPQ
  alignment. Generated MIDI is file output, not a realtime VST MIDI bus.
- Notes are inspected in the ledger; velocity is editable. Other note columns and
  piano-roll geometry are read-only. Melody remains experimental, preview internally
  mono, arrangement audition uses one sound, history is in memory and has three locks.
- Batch work runs outside audio on the message thread. Slow disks may occupy the
  UI; failed new directories remain available for recovery.
- Linux core-only compilation was validated in the earlier engine milestone.
  Linux GUI/macOS are not certified by this Windows migration, nor are subjective
  listening quality or worst-case realtime performance established by these tests.

The active README links, actual PNG assets, SVG masthead and license/notice presence
are checked by tests/docs.py. Historical records in history/ are labeled prior
milestones and are not current build instructions. Original Git history and origin
are preserved; no commit or push is performed automatically.
