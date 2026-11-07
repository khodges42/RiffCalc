# RiffCalc dependency inventory

- nlohmann/json 3.12.0, MIT: original header/license and exact source URLs/hashes
  in `nlohmann/`. Private implementation dependency; public music APIs use C++ types.
- JUCE 7.0.12: pinned unmodified source in `JUCE/`, GPLv3 alternative selected for
  the application. Its included VST3 SDK also offers GPLv3. ISC/permissive module
  and bundled codec/font/platform notices remain in the original source. No ASIO
  SDK is included by RiffCalc's build; web/curl, FLAC, Ogg, MP3 and Windows Media
  format support are disabled. WAV is used only for the test renderer.

`juce-source.json` identifies the release archive and SHA-256.
`juce-files.sha256.json` inventories every retained original file. Run
`python scripts/check_vendor.py` from RiffCalc. CMake never downloads dependencies.
The native UI uses installed system fonts; no decorative artwork is required.

The combined JUCE7/VST3 build uses GPLv3, without relicensing upstream as later
versions. Original RiffCalc code retains its GPL-3.0-or-later grant. Full
corresponding source and retained notices are included by the packaging script.
The separate Rust plugin dependency inventory is unaffected.

## Why the dependency source is committed

The complete pinned JUCE release is intentionally vendored: 3,592 files, about
63 MiB. Most files in this repository are upstream framework source, examples,
platform support or retained notices, rather than RiffCalc's own implementation.
The examples are not built into RiffCalc; keeping the upstream tree intact makes
the archive/per-file provenance checks straightforward.

Vendoring is a build-policy choice, not a requirement to put all dependency source
in Git. A pinned download or submodule workflow could replace it later, with
corresponding changes to offline builds, verification and source packaging. For
now, a normal clone contains everything the documented build requires. Do not
remove this directory independently of those changes. RiffCalc's generated builds,
packages and local scratch work remain ignored under out/ and .dev-notes/.
