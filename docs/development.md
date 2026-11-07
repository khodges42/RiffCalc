# Development and release checks

See README for Windows and core-only commands. This is an independent repository;
no VSTs/DAWJ build products are needed. All upstream code required by CMake is
vendored. Original engine APIs/test names retain their compatibility namespace.

Run Release and Debug CTest, vendor hashes and the independent VST3 host. The
optional external pluginval tool can validate the built bundle at strictness 5:

```powershell
pluginval --strictness-level 5 --random-seed 42 --validate "out/windows/RiffCalc_artefacts/Release/VST3/RiffCalc 2000 Pro.vst3"
```

Pluginval is a development tool, not an application or repository build dependency.
Standalone and plugin contain the same processor/editor. Test native dialogs with
real desktop peers, file export/import, keyboard focus and minimum layouts.

Package with scripts/package.py after building Release. It reruns tests, rejects an
existing output, includes full corresponding source/licenses and hashes every file.
No commit, remote operation, global plugin installation or publication is automated.
The package is unsigned. See validation.md for the checks actually performed.

## Screenshots

Launch `RiffCalc.exe --demo` for a real four-bar C-minor/Liquid DnB document,
seed 1999, complexity 75. Add `--events`, `--advanced` or `--about` for that native
view. Demonstration mode neither reads nor writes saved preferences. Instructions
and provenance are in screenshots/README.md. Pillow is an optional capture-tool
dependency, not a build/runtime requirement.

## Future work

Host-specific FL Studio drag/import tests and listening review remain useful.
Linux core is portable; no Linux GUI/macOS release is certified. Keep later
features and musical algorithm changes separate from visual refinements.
