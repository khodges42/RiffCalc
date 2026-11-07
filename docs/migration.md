# PACKRAT to RiffCalc migration

The original self-contained `packrat/` project was copied out of VSTs into this
independent repository. Source, resources, tests, scripts, licenses and pinned
dependencies were inventoried before modification. No shared VSTs/Rust/DAWJ code
is required. Build outputs, external validation tools and the source repository's
Git metadata were not imported.

The target already had an initial Git commit and an existing origin. Both were
preserved; no remote was created, fetched, changed or pushed. The source project
files were untracked in the VSTs repository, so there was no tracked application
history to transplant. Earlier architecture/validation records are retained under
history/. This rebrand remains an uncommitted working-tree change for user review.

A full independent CMake standalone/VST3 build and all tests passed before source
cleanup. The eleven original engine/preview/MIDI/profile implementation/data files
were compared against their pre-migration SHA-256 inventory and remain identical.
The old source directory was removed only after confirming it had not changed
since that inventory. Relevant historical records moved here; the VSTs README
points to the independent project instead of registering a bundled application.
Unrelated plugin code, build infrastructure and DAWJ production paths are unchanged.

Compatibility is intentional: same plugin/automation IDs, legacy C++ API, document
schema and reference MIDI. Both v0.2 and v0.3 loaded VST3s report the same Windows
unique/deprecated IDs, `5bd65fc7` / `38bf9881`. New visible product names, filenames,
settings and temporary MIDI names are RiffCalc. See compatibility.md.
