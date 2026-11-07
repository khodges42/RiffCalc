# SPDX-License-Identifier: GPL-3.0-or-later
"""Export the four fixed reference phrases for manual audition; not Pack Factory."""
import argparse,hashlib,json,subprocess
from pathlib import Path
parser=argparse.ArgumentParser()
parser.add_argument('executable',type=Path)
parser.add_argument('output',type=Path)
args=parser.parse_args();executable=args.executable.resolve();output=args.output.resolve()
fixtures=json.loads((Path(__file__).resolve().parents[1]/'tests/fixtures/golden.json').read_text())
targets=[output/(f['id']+suffix) for f in fixtures for suffix in ['.mid','.json']]
targets.append(output/'examples.json')
if any(p.exists() for p in targets):raise SystemExit('Example outputs already exist; choose a new directory.')
output.mkdir(parents=True,exist_ok=True);manifest=[]
for f in fixtures:
    path=output/(f['id']+'.mid')
    subprocess.run([str(executable),'generate',*f['args'],'--out',str(path)],check=True)
    for p,expected in [(path,f['midi_sha256']),(path.with_suffix('.json'),f['metadata_sha256'])]:
        assert hashlib.sha256(p.read_bytes()).hexdigest()==expected,p
    manifest.append({'id':f['id'],'midi':path.name,'parameters':f['args'],'seed':json.loads(path.with_suffix('.json').read_text())['config']['seed']})
manifest_path=output/'examples.json'
with manifest_path.open('x',encoding='utf-8') as file:json.dump(manifest,file,indent=2);file.write('\n')
