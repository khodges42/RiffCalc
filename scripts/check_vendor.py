"""Verify the offline C++ dependency inventory, separate from Cargo's inventory."""
import hashlib,json
from pathlib import Path
root=Path(__file__).resolve().parents[1]/'third_party/nlohmann'
manifest=json.loads((root/'sources.json').read_text())
assert manifest['version']=='3.12.0' and manifest['license']=='MIT'
for name,record in manifest['files'].items():
    assert hashlib.sha256((root/name).read_bytes()).hexdigest()==record['sha256'],name
assert 'Permission is hereby granted' in (root/'LICENSE.MIT').read_text()
print('Pinned nlohmann/json 3.12.0 header and MIT license hashes match')

base=Path(__file__).resolve().parents[1]/'third_party'
records=json.loads((base/'juce-files.sha256.json').read_text())
actual={p.relative_to(base/'JUCE').as_posix() for p in (base/'JUCE').rglob('*') if p.is_file()}
assert actual==set(records),'JUCE source file inventory changed'
for name,digest in records.items():
    assert hashlib.sha256((base/'JUCE'/name).read_bytes()).hexdigest()==digest,name
assert 'Or: You may also use this code under the terms of the GPL v3' in (base/'JUCE/modules/juce_audio_processors/juce_audio_processors.h').read_text()
print(f'Pinned JUCE 7.0.12: {len(records)} original source/license files match')
