"""Independent Type 0/1 MIDI decoding and complete batch export validation."""
import json,struct,sys
from pathlib import Path
root=Path(sys.argv[1])
def decode(path):
 b=path.read_bytes();assert b[:4]==b'MThd';size,kind,tracks,ppq=struct.unpack('>IHHH',b[4:14]);assert size==6 and kind in (0,1) and ppq==960
 pos=14;result=[]
 for _ in range(tracks):
  assert b[pos:pos+4]==b'MTrk';length=int.from_bytes(b[pos+4:pos+8],'big');pos+=8;end=pos+length;tick=0;active={};notes=[];names=[];tempo=[];meter=[];eot=None;last_tick=-1;last_on=-1
  def vlq():
   nonlocal pos
   value=0
   for _ in range(4):
    byte=b[pos];pos+=1;value=(value<<7)|(byte&127)
    if not byte&128:return value
   raise AssertionError('Bad VLQ')
  while pos<end:
   tick+=vlq();status=b[pos];pos+=1
   if status==255:
    tag=b[pos];pos+=1;n=vlq();data=b[pos:pos+n];pos+=n
    if tag==3:names.append(data.decode())
    elif tag==0x51:assert tick==0 and n==3;tempo.append(int.from_bytes(data,'big'))
    elif tag==0x58:assert tick==0;meter.append(tuple(data))
    elif tag==0x2f:assert pos==end and n==0;eot=tick
    else:raise AssertionError('Unexpected meta')
   else:
    assert status>>4 in (8,9);pitch,velocity=b[pos:pos+2];pos+=2;channel=status&15;on=status>>4==9;assert pitch<128 and velocity<128
    if tick==last_tick:assert int(on)>=last_on
    last_tick=tick;last_on=int(on);key=(channel,pitch)
    if on:assert velocity and key not in active;active[key]=(tick,velocity)
    else:
     assert key in active and velocity==0;start,v=active.pop(key);assert tick>start;notes.append(dict(start=start,duration=tick-start,pitch=pitch,velocity=v,channel=channel))
  assert pos==end and not active and eot and len(names)==1
  result.append(dict(notes=notes,tempo=tempo,meter=meter,eot=eot,name=names[0]))
 assert pos==len(b)
 return kind,result
sort=lambda notes:sorted(notes,key=lambda n:(n['start'],n['pitch'],n['channel']))
doc=json.loads((root/'example.packrat.json').read_text());kind,tracks=decode(root/'arrangement.mid');assert kind==1 and len(tracks)==5
assert tracks[0]['notes']==[] and tracks[0]['tempo']==[(60000000+doc['harmony']['config']['bpm']//2)//doc['harmony']['config']['bpm']]
for i,part in enumerate(doc['parts']):
 assert not tracks[i+1]['tempo'] and not tracks[i+1]['meter'];assert sort(tracks[i+1]['notes'])==sort(part['notes']);assert tracks[i+1]['eot']==doc['harmony']['length_ticks']
batch=max(root.glob('pack-*'),key=lambda p:p.stat().st_mtime_ns);seeds=set()
for folder in sorted(batch.iterdir()):
 document=json.loads((folder/'metadata.json').read_text());seeds.add(document['harmony']['config']['seed'])
 for i,name in enumerate(('chords','bass','arp','melody')):
  kind,tracks=decode(folder/(name+'.mid'));assert kind==0 and len(tracks)==1;assert sort(tracks[0]['notes'])==sort(document['parts'][i]['notes'])
assert len(seeds)==8
print('PASS: conductor + four Type 1 tracks and 32 batch SMFs independently decoded against stored note events')
