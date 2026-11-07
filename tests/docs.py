"""Check active documentation links and real screenshot assets; stdlib only."""
from pathlib import Path
import re,struct,xml.etree.ElementTree as ET
root=Path(__file__).resolve().parents[1]
files=[root/'README.md',root/'CONTRIBUTING.md',root/'third_party/README.md',*sorted((root/'docs').glob('*.md')),root/'docs/screenshots/README.md']
checked=0
for file in files:
 text=file.read_text(encoding='utf-8')
 links=re.findall(r'!?\[[^\]]*\]\(([^)]+)\)',text)+re.findall(r'<img[^>]+src="([^"]+)"',text)
 for link in links:
  if link.startswith(('https://','http://','#','mailto:')):continue
  relative=link.split('#')[0].split(' "')[0]
  target=(file.parent/relative).resolve()
  assert target.is_relative_to(root) and target.exists(),(file,link)
  checked+=1
for name in ['workspace.png','event-sheet.png','about.png','riff-machine.png']:
 data=(root/'docs/screenshots'/name).read_bytes();assert data[:8]==b'\x89PNG\r\n\x1a\n'
 width,height=struct.unpack('>II',data[16:24]);assert width>=500 and height>=300,(name,width,height)
ET.parse(root/'docs/branding/masthead.svg')
assert (root/'LICENSE').read_text().find('GNU GENERAL PUBLIC LICENSE')>=0
assert 'GPL-3.0-or-later' in (root/'NOTICE').read_text()
print(f'PASS: {checked} documentation links, four native screenshots, SVG masthead and licensing')
