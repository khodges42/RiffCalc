# Screenshot provenance and capture

These PNGs are captures of the actual Windows x64 Release standalone, not mockups.
`workspace.png` shows the generated C-minor/Liquid Drum & Bass four-bar document,
seed 1999, complexity 75. `event-sheet.png` is that same document's real event ledger.
`riff-machine.png` exposes generation properties. `about.png` is the actual desktop
About dialog. No scene or control is painted over the application's output.
The `generation-detail.png` image is only a crop of the real workspace capture.

Reproduce from the built executable, with Python and optional Pillow available:

```powershell
$app = Start-Process -FilePath './out/windows/RiffCalc_artefacts/Release/Standalone/RiffCalc.exe' -ArgumentList '--demo' -PassThru
python tools/capture_windows.py --pid $app.Id --out docs/screenshots/workspace.png
```

Close that demonstration. Repeat with `--demo --events`, `--demo --advanced`, or
`--demo --about`. About capture uses `--title 'About RiffCalc 2000 Pro'`. Close the
About dialog before exiting its parent. Demonstration mode skips preference load
and save, so normal user work is not replaced. The real generator, processor and
editor are used. No GUI-only approximation is involved.

The capture helper matches exact process ID and title, then captures that window
through Pillow's Windows window API. It does not capture the entire desktop or
inject keyboard/mouse input. Window sizes are logical native sizes at the host's
current DPI. Keep an unobscured, readable native window and inspect the result.

The README embeds full-resolution PNGs with relative paths. Their inactive gray
OS title bar is authentic window state; the application masthead remains navy.
No personal paths, unrelated desktop windows or debug overlays are included.
