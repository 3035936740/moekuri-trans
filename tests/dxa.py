"""DXA v4 interoperability, font fallback and tagged catalog regression.
CI runs the portable subset; --native adds the proprietary local game probe.
"""
from pathlib import Path
import csv,hashlib,json,random,re,shutil,subprocess,sys,tempfile,time
ROOT=Path(__file__).resolve().parent.parent
TOOL=ROOT/'MoeKuriTools.exe'

def run(*args,cwd=ROOT,ok=True):
    result=subprocess.run([str(TOOL),*map(str,args)],cwd=cwd,capture_output=True,timeout=120)
    if ok: assert result.returncode==0,(args,result.stdout.decode('utf-8','replace'),result.stderr)
    else: assert result.returncode!=0,(args,'invalid input accepted')
    return result

def game_probe(game,argument,report):
    info=subprocess.STARTUPINFO();info.dwFlags|=subprocess.STARTF_USESHOWWINDOW;info.wShowWindow=0
    result=subprocess.run([str(game),argument],cwd=game.parent,capture_output=True,timeout=35,startupinfo=info)
    assert result.returncode==0,(argument,result.returncode)
    return json.loads((game.parent/report).read_text(encoding='utf-8'))

def main():
    with tempfile.TemporaryDirectory(dir=ROOT/'build',prefix='dxa-') as directory:
        temp=Path(directory).resolve();assert temp.parent== (ROOT/'build').resolve()
        tree=temp/'input';tree.mkdir();(tree/'nested/empty').mkdir(parents=True)
        random_bytes=random.Random(20261008).randbytes(180000)
        files={'zero.bin':b'', 'repeat.bin':b'abc'*12000, 'nested/random.bin':random_bytes,
               'nested/日本語.txt':'UTF-8 日本語文本\t\n'.encode(), 'distance.bin':random_bytes+random_bytes[:20000]}
        for name,content in files.items():(tree/name).write_bytes(content)
        archive=temp/'roundtrip.dxa';run('--dxa-pack',tree,archive);output=temp/'unpacked';run('--dxa-unpack',archive,output)
        assert (output/'nested/empty').is_dir()
        assert all((output/name).read_bytes()==content for name,content in files.items())
        run('--dxa-unpack',archive,output,ok=False)
        run('--dxa-pack',tree,tree/'recursive.dxa',ok=False)
        unsupported=temp/'unsupported';unsupported.mkdir();(unsupported/'😀.txt').write_text('UTF-8',encoding='utf-8')
        run('--dxa-pack',unsupported,temp/'unsupported.dxa',ok=False)
        # A hostile original filename in the table must not escape the destination.
        key=bytes([0x92,0xf6,0xe4,0x68,0x9e,0xc1,0x90,0x32,0xcb,0x1e,0,0xa3])
        raw=bytearray(v^key[i%12] for i,v in enumerate(archive.read_bytes()));table=int.from_bytes(raw[12:16],'little');heads=int.from_bytes(raw[16:20],'little')
        name=int.from_bytes(raw[table+heads:table+heads+4],'little');words=int.from_bytes(raw[table+name:table+name+2],'little');start=table+name+4+words*4
        raw[start:start+7]=b'../bad\0';hostile=temp/'hostile.dxa';hostile.write_bytes(bytes(v^key[i%12] for i,v in enumerate(raw)))
        run('--dxa-unpack',hostile,temp/'unsafe',ok=False);assert not (temp/'bad').exists()
        sample=temp/'scale.txt';sample.write_text('日本語\\r\\n\\t<tab>\t译文\\r\\n\\t<tab>\t230\t75\n',encoding='utf-8')
        tagged=temp/'scale.csv';run('--convert',sample,tagged)
        with tagged.open(encoding='utf-8-sig',newline='') as f: table=list(csv.DictReader(f))
        assert table[0]['Width%']=='75' and '<cr><lf><tab><lt>tab>' in table[0]['Original']
        recovered=temp/'recovered.txt';run('--convert',tagged,recovered)
        assert recovered.read_text(encoding='utf-8').split('\n')[-2].endswith('\t230\t75')
        with tagged.open('w',encoding='utf-8-sig',newline='') as f:
            w=csv.DictWriter(f,fieldnames=table[0].keys());w.writeheader();table[0]['Original']='<broken>';w.writerows(table)
        run('--validate',tagged,ok=False)
        report={'dxa_roundtrip':True,'compression':True,'empty_directories':True,'path_bounds':True,'cp932_names':True,
                'tagged_csv_roundtrip':True,'row_scale_roundtrip':True}
        if '--native' in sys.argv:
            expected='1c79a2d328d8ccd69765ac624b48f2317d80688c3dc7439ee3c23fc1fc05f847'
            original=next(p for p in ROOT.parent.glob('*.exe') if p.stat().st_size==4358144 and hashlib.sha256(p.read_bytes()).hexdigest()==expected)
            native=ROOT/'tests/dxa-native';native.mkdir(exist_ok=True)
            for name in ('data','BGM'):shutil.copytree(ROOT.parent/name,native/name,dirs_exist_ok=True)
            shutil.copyfile(ROOT.parent/'config.ini',native/'config.ini');shutil.copyfile(ROOT/'build/optimized.exe',native/'game.exe')
            unpacked=temp/'csv';run('--dxa-unpack',ROOT.parent/'data/csv.dxa',unpacked)
            run('--dxa-pack',unpacked,native/'data/csv.dxa')
            csv_report=game_probe(native/'game.exe','--cn-csv-test','cn-csv.json');assert csv_report['passed'],csv_report
            run('--dxa-unpack',native/'data/csv.dxa',temp/'verify')
            assert all((temp/'verify'/p.relative_to(unpacked)).read_bytes()==p.read_bytes() for p in unpacked.rglob('*') if p.is_file())
            report['native_game_reads_repacked_dxa']=csv_report
            fontgame=native/'font-game.exe'
            run('--pack',original,ROOT/'1.10trans_omidxa.txt',fontgame,'--font',ROOT/'font.ttf','--font-dxa','1')
            packed=native/'font.dxa';good=packed.read_bytes();font=game_probe(fontgame,'--cn-font-probe','cn-font-probe.json')
            assert font['external_file']==3 and font['font_faces']>0 and font['embedded_bytes']==0,font
            decoded=temp/'font';run('--dxa-unpack',packed,decoded);fontbytes=(decoded/'font.ttf').read_bytes()
            packed.write_bytes(b'bad DXA');(native/'font.ttf').write_bytes(fontbytes)
            fallback=game_probe(fontgame,'--cn-font-probe','cn-font-probe.json');assert fallback['external_file']==2,fallback
            (native/'font.ttf').unlink();system=game_probe(fontgame,'--cn-font-probe','cn-font-probe.json');assert system['external_file']==0,system
            packed.write_bytes(good)
            report.update(font_dxa=font,invalid_dxa_fallback=fallback,system_fallback=system,font_bytes=len(fontbytes),font_dxa_bytes=len(good),external_exe_bytes=fontgame.stat().st_size)
            # A single real EXE entry with a per-row scale reaches draw/measure hooks.
            first=next(line for line in (ROOT/'1.10trans_omidxa.txt').read_text(encoding='utf-8').split('\n') if line and not line.startswith('#') and line.split('\t')[0]!=line.split('\t')[1] and '\\n' not in line and '\\r' not in line)
            scaled=temp/'scaled.txt';scaled.write_text(first+'\t75\n',encoding='utf-8');scaled_game=native/'scale-game.exe'
            run('--pack',original,scaled,scaled_game,'--max-width','0');probe=game_probe(scaled_game,'--cn-probe','cn-probe.json');assert probe['draw_and_width_translation'],probe
            run('--unpack',scaled_game,temp/'scaled-output.txt');assert (temp/'scaled-output.txt').read_text(encoding='utf-8').split('\n')[-2].endswith('\t75')
            report['native_per_row_scale']=True
        report['passed']=True;(ROOT/'build/dxa-report.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
