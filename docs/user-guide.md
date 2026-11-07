# RiffCalc operator's guide

The [README walkthrough](../README.md#your-first-pattern) covers the normal workflow.

## Working specifications

Root, scale, genre, length, complexity, chord register, velocity, humanization,
resolve/colour and arp order apply on Generate. Tempo and output are immediate.
Genre suggests tempo/sound but does not force a scale. Pattern selects the current
related clip; arrangement combines the four parts for preview/export.

New Candidate advances the uint64 seed. Use Seed/F5 leaves the seed unchanged.
Fresh unlocked results are repeatable. Mutate preserves recognizable material;
locked/mutated operations also require the previous candidate. Save Idea preserves
exact notes and settings. Opening an idea or host session restores playback stopped.

Harmony lock retains root/quality movement. Voicing additionally locks harmony
and exact chord pitches. Rhythm preserves part note onsets/durations. Unlock before
changing key/scale/length; voicing locks also protect chord register.

## Event Sheet

Rows list actual exported notes. Velocity is editable on double-click, range 1–127.
Enter commits and Escape cancels. Invalid values revert. Selection reports note
information in the status bar. A valid edit adds candidate history. Previous/Next
restores earlier documents, including note velocities. Other columns are read-only.
Chords above the piano roll describe generated harmony; this is not a full editor.

## Keyboard reference

| Key | Command |
|---|---|
| Ctrl+O | Open Idea |
| Ctrl+Shift+S | Save Idea As |
| Ctrl+E | Export MIDI |
| G / M | Generate / Mutate |
| F5 | Regenerate using displayed seed |
| Space / Escape | Preview toggle / Stop |
| Alt+Left / Alt+Right | Previous / Next candidate |
| F1 | Embedded help |
| Tab / Shift+Tab | Navigate controls |

Typing in a text editor retains ordinary text behavior. Numeric entry and dropdown
arrows belong to their focused controls. Menus, buttons and tooltips expose the
same supported actions. Preview settings may also be automated in the host.

## Files and packs

Save/open use full versioned documents; existing outputs are protected. Both new
`.riffcalc.json` and legacy `.packrat.json` are accepted. Exporting MIDI does not
save an editable idea document. Save the idea too when you want exact restoration.
Packs write all four companions plus restorable metadata per numbered idea. Locks
carry into batch generation. Failed new packs retain their partial folder.

Generated MIDI uses SMF files, not a live VST MIDI bus. DAW drop behavior differs;
file export is always available. Preview notes are audition material, not a claim
of exact commercial instrument emulation. No preset is a finished composition.
