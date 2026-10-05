"""Check the JP 1.10 import examples and the generated EXE's PE marker."""
from pathlib import Path
import hashlib,json,re,struct,subprocess,sys,tempfile,zipfile
ROOT=Path(__file__).resolve().parent.parent
TOOL=ROOT/'MoeKuriTools.exe'
sys.stdout.reconfigure(encoding='utf-8')
def run(tool,*args,cwd=ROOT):
    r=subprocess.run([str(tool),*map(str,args)],cwd=cwd,capture_output=True,timeout=30)
    assert r.returncode==0,(args,r.stdout.decode('utf-8'),r.stderr.decode('utf-8'))
    return json.loads(r.stdout) if r.stdout.strip() else None
def rows(path):
    unescape=lambda s:re.sub(r'\\([nrt\\])',lambda m:{'n':'\n','r':'\r','t':'\t','\\':'\\'}[m[1]],s)
    return {unescape(f[0]):(unescape(f[1]),int(f[2]) if len(f)>2 else 0) for line in path.read_text(encoding='utf-8-sig').splitlines() if line and not line.startswith('#') for f in [line.split('\t')]}
def main():
    txt=ROOT/'1.10trans.txt';csv=ROOT/'1.10trans.csv';dictionary=rows(txt)
    overrides=json.loads((ROOT/'catalog-overrides.json').read_text(encoding='utf-8'))
    assert all(dictionary[k][0]==v for k,v in overrides.items())
    assert not any(term in value[0] for value in dictionary.values()
                   for term in ('萌萌物语','萌萌魔物','萌库里２'))
    assert run(TOOL,'--validate',txt)['rows']==len(dictionary)
    assert run(TOOL,'--validate',csv)['rows']==len(dictionary)
    assert (ROOT/'translation.txt').read_bytes()==txt.read_bytes()
    assert csv.read_bytes().startswith(b'\xef\xbb\xbf')
    with tempfile.TemporaryDirectory(dir=ROOT/'build',prefix='sample-') as temp:
        temp=Path(temp)
        run(TOOL,'--convert',txt,temp/'txt-canonical.txt')
        run(TOOL,'--import-csv',csv,temp/'csv-canonical.txt')
        assert (temp/'txt-canonical.txt').read_bytes()==(temp/'csv-canonical.txt').read_bytes()
        with zipfile.ZipFile(ROOT/'dist/moekuri-trans-windows-x86.zip') as archive:archive.extractall(temp)
        assert (temp/'1.10trans.txt').read_bytes()==txt.read_bytes()
        assert not (temp/'translation.txt').exists()
        if '--gui' in sys.argv:
            run(temp/'MoeKuriTools.exe','--gui-test',temp/'gui.json',cwd=temp)
            gui=json.loads((temp/'gui.json').read_text(encoding='utf-8'))
            assert gui['gui_passed']=='true' and int(gui['rows'])==len(dictionary),gui
    report={'passed':True,'sample_rows':len(dictionary),'txt_csv_import_equivalent':True,'packaged_sample_only':True,'isolated_gui_checked':'--gui' in sys.argv}
    # Bind the sample's provenance to the reference when it is locally present.
    reference=ROOT.parents[2]/'mods/trans/localization/jp-1.10/translate.json'
    if reference.exists():
        known=json.loads(reference.read_text(encoding='utf-8-sig'))
        assert all(dictionary[k][0]==overrides.get(k,v) for k,v in known.items())
        report['reference_rows_covered']=len(known)
        report['reference_rows_overridden']=len(overrides)
    patched=ROOT/'build/optimized.exe'
    if patched.exists():
        info=run(TOOL,'--inspect',patched)
        assert info['marked']=='true' and info['signature']=='moekuri_trans' and info['catalog_sha256_matches']=='true',info
        assert info['catalog_sha256']==hashlib.sha256(txt.read_bytes()).hexdigest()
        assert int(info['catalog_rows'])==len(dictionary)
        b=patched.read_bytes();nt=struct.unpack_from('<I',b,60)[0];count=struct.unpack_from('<H',b,nt+6)[0];opt=struct.unpack_from('<H',b,nt+20)[0]
        for i in range(count):
            p=nt+24+opt+i*40
            if b[p:p+8]!=b'.mktrans':continue
            size,va,rawsize,raw=struct.unpack_from('<IIII',b,p+8);flags=struct.unpack_from('<I',b,p+36)[0]
            assert flags & 0x40000000 and not flags & 0xa0000000
            content=b[raw:raw+size]
            assert content.startswith(b'moekuri_trans\0')
            assert json.loads(content[14:].decode('utf-8'))['catalog_sha256']==info['catalog_sha256']
            report['pe_marker_readonly']=True;report['pe_marker_raw_offset']=raw;report['pe_marker_virtual_bytes']=size
            break
        else:raise AssertionError('Missing .mktrans PE section')
        original=next(p for p in ROOT.parent.glob('*.exe') if hashlib.sha256(p.read_bytes()).hexdigest()==info['original_sha256'])
        assert run(TOOL,'--inspect',original)=={'marked':False}
        report['marker']=info
    (ROOT/'tests/sample-marker-report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps(report,ensure_ascii=False,indent=2))
if __name__=='__main__':main()
