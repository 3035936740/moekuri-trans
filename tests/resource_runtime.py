"""Verify native archive line translation and optional fonts in isolated game copies."""
from pathlib import Path
import hashlib
import json
import shutil
import subprocess
import csv
import re

ROOT = Path(__file__).resolve().parent.parent
TEST = ROOT/'tests'
TOOL = ROOT/'MoeKuriTools.exe'
ORIGINAL = next(p for p in ROOT.parent.glob('*.exe') if hashlib.sha256(p.read_bytes()).hexdigest() == '1c79a2d328d8ccd69765ac624b48f2317d80688c3dc7439ee3c23fc1fc05f847')

def run_game(path, argument, report):
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    result = subprocess.run([str(path), argument],cwd=path.parent,capture_output=True,timeout=30,startupinfo=startup)
    assert result.returncode == 0, (argument,result.returncode)
    return json.loads((path.parent/report).read_text(encoding='utf-8'))

def main():
    fixture = TEST/'live-description'
    fixture.mkdir(exist_ok=True)
    for name in ('data','BGM'):
        shutil.copytree(ROOT.parent/name,fixture/name,dirs_exist_ok=True)
    shutil.copyfile(ROOT.parent/'config.ini',fixture/'config.ini')
    shutil.copyfile(ROOT/'build/optimized.exe',fixture/'game.exe')
    descriptions = run_game(fixture/'game.exe','--cn-description-test','cn-descriptions.json')
    assert descriptions['passed'], descriptions
    csv_report = run_game(fixture/'game.exe','--cn-csv-test','cn-csv.json')
    def unescape(text):
        return re.sub(r'\\([nrt\\])',lambda m:{'n':'\n','r':'\r','t':'\t','\\':'\\'}[m[1]],text)
    catalog={unescape(parts[0]):unescape(parts[1]) for line in (ROOT/'translation.txt').read_text(encoding='utf-8-sig').splitlines()
             if line and not line.startswith('#') for parts in [line.split('\t')]}
    columns={'w_para':{1,15},'n_para':{1,5},'h_para':{2},'mj_para':{1},'t_para':{0,19,20,21},'s_para':None}
    checked_fields=0
    for name,display in columns.items():
        with (fixture/f'cn-{name}-original.csv').open(encoding='utf-16',newline='') as stream: original_rows=list(csv.reader(stream))
        with (fixture/f'cn-{name}-translated.csv').open(encoding='utf-16',newline='') as stream: translated_rows=list(csv.reader(stream))
        assert len(original_rows)==len(translated_rows),(name,'row count')
        assert original_rows[0]==translated_rows[0],(name,'header')
        for index,(before,after) in enumerate(zip(original_rows[1:],translated_rows[1:]),1):
            assert len(before)==len(after),(name,index,'column count')
            for column,(source,actual) in enumerate(zip(before,after)):
                selected=display is None or column in display
                target=catalog.get(source,source) if selected else source
                if not target or target.startswith('@') or '\r' in target or '\n' in target: target=source
                assert actual==target,(name,index,column,source,actual,target)
                checked_fields+=1
    # Find the screenshot description in the actual native reader output.
    with (fixture/'cn-w_para-translated.csv').open(encoding='utf-16',newline='') as stream: skills=list(csv.reader(stream))
    snow=next(row for row in skills[1:] if row[1]=='雪道')
    assert snow[15]=='直线攻击。目标地形变为[冰]；若已是[冰]，则变为[雪原]。',snow[15]
    csv_report.update({'fields_checked':checked_fields,'non_display_columns_unchanged':True,'snow_path_translated_before_wrapping':True})
    font_dir=TEST/'external-probe'
    font_dir.mkdir(exist_ok=True)
    game=font_dir/'game.exe'
    result=subprocess.run([str(TOOL),'--pack',str(ORIGINAL),str(ROOT/'translation.txt'),str(game),
                           '--font',str(ROOT/'missing-unused.ttf'),'--external-font','1'],capture_output=True,timeout=30)
    assert result.returncode == 0, result.stdout
    for name in ('font.otf','font.ttf'):
        p=font_dir/name
        if p.exists(): p.unlink()
    missing=run_game(game,'--cn-font-probe','cn-font-probe.json')
    assert missing == {'external_requested':True,'external_file':0,'font_faces':0,'embedded_bytes':0},missing
    shutil.copyfile(ROOT/'font.ttf',font_dir/'font.ttf')
    ttf=run_game(game,'--cn-font-probe','cn-font-probe.json')
    assert ttf['external_file']==2 and ttf['font_faces']>0 and ttf['embedded_bytes']==0,ttf
    (font_dir/'font.otf').write_bytes(b'invalid optional font')
    invalid=run_game(game,'--cn-font-probe','cn-font-probe.json')
    assert invalid==ttf,invalid
    shutil.copyfile(ROOT/'font.ttf',font_dir/'font.otf')
    priority=run_game(game,'--cn-font-probe','cn-font-probe.json')
    assert priority['external_file']==1 and priority['font_faces']>0,priority
    unpacked=font_dir/'font-export.ttf'
    result=subprocess.run([str(TOOL),'--unpack',str(game),str(font_dir/'catalog.txt'),'--font-out',str(unpacked)],capture_output=True,timeout=30)
    assert result.returncode==0 and unpacked.stat().st_size==0
    assert (font_dir/'catalog.txt').read_bytes()==(ROOT/'translation.txt').read_bytes()
    report={'descriptions':descriptions,'csv_resources':csv_report,'external_missing':missing,'external_ttf':ttf,
            'invalid_otf_falls_back':invalid,'otf_priority':priority,'external_exe_bytes':game.stat().st_size,'passed':True}
    (TEST/'resource-runtime-report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps(report,indent=2))

if __name__=='__main__':main()
