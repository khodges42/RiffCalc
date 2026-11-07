"""Package a tested Windows build with self-contained corresponding source. No installs."""
import argparse
import hashlib
import json
import shutil
import subprocess
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('--build', type=Path, required=True)
parser.add_argument('--out', type=Path, required=True)
parser.add_argument('--ctest', required=True)
args = parser.parse_args()
source = Path(__file__).resolve().parents[1]
build, out = args.build.resolve(), args.out.resolve()
subprocess.run([args.ctest, '--test-dir', str(build), '-C', 'Release', '--output-on-failure'], check=True)
if out.exists():
    raise SystemExit('Choose a new output directory; packages are never overwritten')
if out.is_relative_to(source) and not out.is_relative_to(source / 'out'):
    raise SystemExit('Packages inside the source tree must be under out/')
app = build / 'RiffCalc_artefacts/Release/Standalone/RiffCalc.exe'
plugin = build / 'RiffCalc_artefacts/Release/VST3/RiffCalc 2000 Pro.vst3'
if not app.is_file() or not plugin.is_dir():
    raise SystemExit('Build the Release standalone and VST3 first')
out.mkdir(parents=True)
shutil.copy2(app, out / 'RiffCalc.exe')
shutil.copy2(build / 'Release/riffcalc.exe', out / 'riffcalc-cli.exe')
shutil.copytree(plugin, out / 'RiffCalc 2000 Pro.vst3')
for name in ['LICENSE', 'NOTICE']:
    shutil.copy2(source / name, out / name)
# Complete offline-buildable source, including all original dependency notices.
def ignore(directory, names):
    excluded = {'__pycache__', '.git'}
    if Path(directory).resolve() == source:
        excluded.update({'out', 'build'})
    return [name for name in names if name in excluded or name.endswith('.user')]

shutil.copytree(source, out / 'Source', ignore=ignore)
(out / 'START-HERE.txt').write_text('''RiffCalc 2000 Pro (Workgroup Edition) / 0.3.0 / Windows x64
Voidmutt Enterprises

Run RiffCalc.exe. Use Options > Audio/MIDI Settings to choose your output device.
Choose key, scale, genre, pattern and sound. GENERATE creates a new idea.
PLAY auditions it. MUTATE varies it. DRAG MIDI copies the selected part into a
compatible DAW; EXPORT MIDI is the reliable file fallback. Arrangement exports
four related MIDI tracks. RIFF MACHINE opens locks, save/open and Pack Factory.

Space = play/stop; G = generate; M = mutate; Escape = stop; Ctrl+E = export.
USE SEED regenerates the displayed seed without advancing it.

For VST3, point your host scanner at this directory or manually copy the complete
RiffCalc 2000 Pro.vst3 directory into your chosen plugin directory. Load as an INSTRUMENT.
No system installation is performed. Generated MIDI uses file export/drag,
not real-time VST MIDI output. Preview starts only when requested.

Complete source and offline build instructions are in Source/README.md.
Original source: GPL-3.0-or-later; combined JUCE7/VST3 binary uses the GPLv3 option.
See LICENSE, NOTICE and Source/third_party for original dependency notices.

Independent host and pluginval results are recorded in Source/docs/validation.md.
FL Studio dragging and manual musical listening review remain to be verified.
''', encoding='utf-8')
manifest = {p.relative_to(out).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(out.rglob('*')) if p.is_file()}
(out / 'SHA256.json').write_text(json.dumps(manifest, indent=2) + '\n')
archive = shutil.make_archive(str(out), 'zip', out.parent, out.name)
print('Runnable application:', out / 'RiffCalc.exe')
print('Source + binary archive:', archive)
