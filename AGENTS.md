# RiffCalc engineering

- C++20/CMake and pinned vendored JUCE 7.0.12. Build offline.
- Keep musical core and preview independent of JUCE. Preserve packrat-1 golden
  hashes, legacy namespace/state schema, plugin codes Pkrt/Pkr1, automation IDs
  and bundle ID audio.packrat.workstation. See docs/compatibility.md.
- No allocation, locks, generation, JSON, filesystem access or heap destruction
  in processing. Prepare clips outside audio; bounded triple-buffer ownership.
- GUI uses shared ClassicLook primitives. No browser UI, external fonts, mascots
  or unlicensed art. About credits VOIDMUTT; publisher is fictional branding.
- Run CTest, vendor hashes and actual VST3 host tests after relevant changes.
  Capture real native windows for screenshots. Do not claim untested DAW behavior.
- Keep builds/packages/scratch under ignored out/ or .dev-notes/.
- Preserve user changes. Do not install globally, commit, push or alter remote
  settings unless explicitly asked. Do not change sibling DAWJ/VST projects.
