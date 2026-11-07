# RiffCalc architecture

This repository is self-contained: C++20/CMake, pinned offline JUCE 7.0.12 and
nlohmann/json. No sibling project, DAWJ playback path or Rust plugin is a dependency.

| Area | Responsibility |
|---|---|
| include/packrat; src/core, style, midi | Portable musical models, weighted harmony, voice-leading, profile bank, MIDI serialization |
| src/workstation | Related parts, deterministic transformations, locks, complete documents and batch exports |
| src/audio | Replaceable fixed-voice preview, event scheduling and triple-buffer ownership; no JUCE |
| src/app/Processor.* | Instrument buses, automation, MIDI keyboard input and session state |
| src/app/Editor.* | Shared standalone/VST3 document workflows and history |
| src/app/EditorCommands.cpp | Single menu/keyboard command routing, dialogs, explicit velocity editing |
| src/app/EditorLayout.cpp | Workspace geometry, etched groups and status panes |
| src/app/ClassicLook.* | Shared native control painting, bevels and original toolbar/logo vectors |
| src/app/NoteSheet.* | Note ledger and validated, queued velocity edits |
| src/app/About.* | Real classic desktop dialogs and selectable help/license text |
| tests, scripts, tools | Core/audio/host tests, offline build/package/inventory, native screenshot capture |

## Thread ownership

Generation, validation, history, JSON and filesystem work occur outside processing.
The GUI and host state producer serialize document access with a non-audio mutex.
They publish fixed arrays of prepared note events through a three-slot SPSC
mailbox. The callback owns its reader slot, reads atomic controls and operates
fixed voices; it never accesses document locks, files or dynamic musical objects.

Note Sheet edits are queued on the message thread, with an epoch check that
rejects an edit if its displayed candidate has changed. A successful edit updates
the selected part (and the harmony clip when necessary), validates and records a
new history entry. It does not mutate the generator algorithm.

## UI lifetime and boundaries

The same native editor is used by standalone and VST3. Its commands are shared
across menus, toolbar and shortcuts. The look-and-feel is instance-local. Only the
standalone's own parent window receives it; it is removed before editor destruction.
Dialog windows are owned by the editor and have real desktop peers. They are
closed before the theme is destroyed. About/license text is embedded at build time.

Preview is a local looping transport. Optional host BPM overrides its tempo when
valid; no claim of host PPQ alignment or real-time generated MIDI output is made.
MIDI file dragging uses JUCE's native file-copy operation. Recent temporary files
remain for seven days to support asynchronous importers.

## Reproducibility and migration

The original musical, preview and MIDI algorithms are retained. Versioned engine
identifiers, legacy schema, namespaced API and VST3 codes are documented in
[compatibility](compatibility.md). The new product version is 0.3.0. A build reports
the base Git revision when available, marked as a local build, or source-archive
for packaged source without Git metadata. No timestamp changes musical generation.

Demonstration startup flags only select a real generated document and native view;
they do not mock the interface or touch preferences. No network/workgroup feature,
web UI, telemetry, mascot or external art service is introduced.
