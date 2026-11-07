"""Independent SMF decoder and CLI tests. Python standard library only."""
import hashlib,json,struct,subprocess,sys,tempfile
from pathlib import Path
exe=Path(sys.argv[1]).resolve();out=Path(sys.argv[2]).resolve();out.mkdir(parents=True,exist_ok=True)
source=Path(__file__).resolve().parents[1]

def run(*args,okay=True):
    p=subprocess.run([str(exe),*map(str,args)],capture_output=True,encoding='utf-8')
    assert (p.returncode==0)==okay,(args,p.stdout,p.stderr)
    return p

def decode(path,meta):
    b=path.read_bytes();assert b[:4]==b'MThd'
    assert struct.unpack('>IHHH',b[4:14])==(6,0,1,960)
    assert b[14:18]==b'MTrk';assert struct.unpack('>I',b[18:22])[0]==len(b)-22
    pos=22;tick=0;active={};notes=[];tempos=[];meters=[];names=[];eot=[];last_order=-1;last_tick=-1
    def vlq():
        nonlocal pos
        value=0
        for _ in range(4):
            x=b[pos];pos+=1;value=(value<<7)|(x&127)
            if not x&128:return value
        raise AssertionError('Oversized VLQ')
    while pos<len(b):
        tick+=vlq();status=b[pos];pos+=1
        if status==255:
            kind=b[pos];pos+=1;n=vlq();data=b[pos:pos+n];assert len(data)==n;pos+=n
            if kind==0x51:assert tick==0 and n==3;tempos.append(int.from_bytes(data,'big'))
            elif kind==0x58:assert tick==0 and n==4;meters.append(tuple(data))
            elif kind==3:assert tick==0;names.append(data.decode('utf-8'))
            elif kind==0x2f:assert n==0 and pos==len(b);eot.append(tick)
            else:raise AssertionError('Unexpected meta event')
        else:
            assert 0x80<=status<=0x9f
            pitch,velocity=b[pos:pos+2];pos+=2;assert pitch<128 and velocity<128
            channel=status&15;on=status>>4==9
            order=int(on)
            if tick==last_tick:assert order>=last_order,'Note-off must precede note-on'
            last_tick=tick;last_order=order;key=(channel,pitch)
            if on:
                assert velocity and key not in active
                active[key]=(tick,velocity)
            else:
                assert velocity==0 and key in active
                start,v=active.pop(key);assert tick>start
                notes.append(dict(start=start,duration=tick-start,pitch=pitch,velocity=v,channel=channel))
    assert not active and eot==[meta['length_ticks']]
    bpm=meta['config']['bpm'];assert tempos==[(60000000+bpm//2)//bpm]
    assert meters==[(4,2,24,8)] and len(names)==1
    sort=lambda x:(x['start'],x['pitch'],x['channel'])
    assert sorted(notes,key=sort)==sorted(meta['notes'],key=sort)
    assert all(meta['config']['low']<=x['pitch']<=meta['config']['high'] for x in notes)
    return b

with tempfile.TemporaryDirectory(prefix='packrat-',dir=out) as temp:
    root=Path(temp).resolve();assert root.is_relative_to(out)
    listing=run('list').stdout
    profiles=json.loads((source/'resources/genres.json').read_text())
    assert all(p['id'] in listing for p in profiles['profiles'])
    # Round-trip each original genre through a parser independent of C++ encoding.
    distinct=set()
    for i,p in enumerate(profiles['profiles']):
        midi=root/f'genre-{i}.mid'
        run('generate','--genre',p['id'],'--scale',p['scales'][0],'--complexity',80,'--humanize',40,'--seed',i+100,'--out',midi)
        meta=json.loads(midi.with_suffix('.json').read_text());blob=decode(midi,meta);distinct.add(hashlib.sha256(blob).hexdigest())
        assert meta['engine']=='packrat-1' and len(meta['chords'])==4
    assert len(distinct)==12
    # Embedded and file-loaded canonical profile data must produce identical bytes.
    a=root/'embedded.mid';b=root/'file.mid'
    base=['generate','--key','C#','--scale','major','--genre','house','--seed','18446744073709551615','--bars',16]
    run(*base,'--out',a);run(*base,'--profiles',source/'resources/genres.json','--out',b)
    assert a.read_bytes()==b.read_bytes()
    assert a.with_suffix('.json').read_bytes()==b.with_suffix('.json').read_bytes()
    decode(a,json.loads(a.with_suffix('.json').read_text()))
    before=a.read_bytes();run(*base,'--out',a,okay=False);assert a.read_bytes()==before
    # A pre-existing sidecar must survive, and an owned MIDI file must roll back.
    orphan=root/'sidecar.mid';orphan.with_suffix('.json').write_text('existing')
    run('generate','--out',orphan,okay=False);assert not orphan.exists() and orphan.with_suffix('.json').read_text()=='existing'
    # Unicode filename support on the primary Windows CLI.
    unicode_path=root/('notes-'+chr(0x266b)+'.mid');run('generate','--out',unicode_path)
    decode(unicode_path,json.loads(unicode_path.with_suffix('.json').read_text()))
    # Malformed inputs fail before creating either output.
    for args in [('--bars','0'),('--bars','17'),('--seed','-1'),('--seed','18446744073709551616'),('--bpm','0'),('--low','80','--high','81'),('--key','H'),('--scale','bogus'),('--genre','bogus'),('--complexity','nan'),('--bars','4','--bars','8'),('--cadence','other')]:
        dest=root/'bad.mid';run('generate',*args,'--out',dest,okay=False);assert not dest.exists() and not dest.with_suffix('.json').exists()
    def malformed(doc):
        p=root/'bad-profiles.json';p.write_text(json.dumps(doc));dest=root/'invalid.mid'
        run('generate','--profiles',p,'--out',dest,okay=False);assert not dest.exists()
    for change in ['schema','unknown','tempo','weight','rhythm','duplicate','float','scale','uint','empty']:
        doc=json.loads(json.dumps(profiles));p=doc['profiles'][0]
        if change=='schema':doc['schema']=0
        elif change=='unknown':p['unexpected']=True
        elif change=='tempo':p['tempo']=[200,100,50]
        elif change=='weight':p['patterns'][0]['weight']=0
        elif change=='rhythm':p['rhythms'][0]['steps']=[0,0]
        elif change=='duplicate':doc['profiles'].append(p)
        elif change=='float':p['center']=64.5
        elif change=='scale':p['scales']=['wrong']
        elif change=='uint':p['center']=2**64-1
        elif change=='empty':p['rhythms']=[]
        malformed(doc)
    # Fixed reference hashes freeze version-1 algorithm/data behavior.
    for fixture in json.loads((source/'tests/fixtures/golden.json').read_text()):
        dest=root/(fixture['id']+'.mid');run('generate',*fixture['args'],'--out',dest)
        assert hashlib.sha256(dest.read_bytes()).hexdigest()==fixture['midi_sha256'],fixture['id']
        assert hashlib.sha256(dest.with_suffix('.json').read_bytes()).hexdigest()==fixture['metadata_sha256'],fixture['id']
        decode(dest,json.loads(dest.with_suffix('.json').read_text()))
print('CLI, 12-genre SMF round trips, state hashes, profile errors, Unicode and overwrite protection passed')
