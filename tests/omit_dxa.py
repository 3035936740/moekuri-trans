"""EXE-only default, catalog filtering and native DXA reader regression."""
from pathlib import Path
import json,subprocess,hashlib,shutil,csv,re
ROOT=Path(__file__).resolve().parent.parent
TOOL=ROOT/'MoeKuriTools.exe'
ORIGINAL=next(p for p in ROOT.parent.glob('*.exe') if hashlib.sha256(p.read_bytes()).hexdigest()=='1c79a2d328d8ccd69765ac624b48f2317d80688c3dc7439ee3c23fc1fc05f847')
TEST=ROOT/'tests/omit-dxa'
TEST.mkdir(exist_ok=True)
def run(*args,cwd=ROOT):
 r=subprocess.run(list(map(str,args)),cwd=cwd,capture_output=True,timeout=45)
 assert r.returncode==0,(args,r.returncode,r.stdout.decode('utf-8'),r.stderr.decode('utf-8'))
 return json.loads(r.stdout) if r.stdout.strip() else None
def rows(path):
 def un(s):return re.sub(r'\\([nrt\\])',lambda m:{'n':'\n','r':'\r','t':'\t','\\':'\\'}[m[1]],s)
 return {un(f[0]):(un(f[1]),int(f[2]) if len(f)>2 else 0) for l in path.read_text(encoding='utf-8-sig').split('\n') if l and not l.startswith('#') for f in [l.split('\t')]}
def main():
 original_rows=TEST/'original-exe.txt'
 run(TOOL,'--extract',ORIGINAL,original_rows)
 full_rows=TEST/'original-full.txt'
 run(TOOL,'--extract',ORIGINAL,full_rows,'--omit-dxa','0')
 exe_sources=set(rows(original_rows));full_sources=set(rows(full_rows))
 assert exe_sources < full_sources
 sample=ROOT/'1.10trans_omidxa.txt';full=rows(ROOT/'1.10trans.txt');filtered=rows(sample)
 assert filtered and set(filtered)==set(full)&exe_sources
 assert all(full[s]==value for s,value in filtered.items())
 assert not any('直線攻撃。対象の地形' in s for s in filtered)
 csv_path=TEST/'export.csv';run(TOOL,'--filter-exe',ORIGINAL,ROOT/'1.10trans.txt',csv_path)
 roundtrip=TEST/'export.txt';run(TOOL,'--import-csv',csv_path,roundtrip)
 assert rows(roundtrip)==filtered
 # Default packing must drop resource rows even if given the complete catalog.
 game=TEST/'game.exe';run(TOOL,'--pack',ORIGINAL,ROOT/'1.10trans.txt',game,'--font',ROOT/'font.ttf')
 info=run(TOOL,'--inspect',game)
 assert info['omit_dxa']=='true' and int(info['catalog_rows'])==len(filtered),info
 unpacked=TEST/'unpacked.txt';settings=TEST/'settings.json'
 run(TOOL,'--unpack',game,unpacked,'--settings-out',settings)
 assert rows(unpacked)==filtered
 assert json.loads(settings.read_text(encoding='utf-8'))['omitDxa']=='1'
 for name in ('data','BGM'):shutil.copytree(ROOT.parent/name,TEST/name,dirs_exist_ok=True)
 shutil.copyfile(ROOT.parent/'config.ini',TEST/'config.ini')
 run(game,'--cn-probe',cwd=TEST)
 assert json.loads((TEST/'cn-probe.json').read_text())['initialized']
 run(game,'--cn-csv-test',cwd=TEST)
 native=json.loads((TEST/'cn-csv.json').read_text())
 assert native['passed'] and native['translated_rows']==0
 for name in ('w_para','n_para','h_para','mj_para','t_para','s_para'):
  assert (TEST/f'cn-{name}-original.csv').read_bytes()==(TEST/f'cn-{name}-translated.csv').read_bytes(),name
 # Existing full-mode fixture keeps the original behavior when unchecked.
 full_info=run(TOOL,'--inspect',ROOT/'build/optimized.exe')
 assert full_info['omit_dxa']=='false'
 report={'passed':True,'default_exe_rows':len(exe_sources),'sample_rows':len(filtered),'complete_sample_rows':len(full),
         'filtered_txt_csv_equivalent':True,'default_pack_filtered':True,'six_native_csv_tables_unchanged':True,'encyclopedia_and_story_unchanged':True,
         'native_csv_rows':native['csv_rows'],'full_mode_restored':True,'game_bytes':game.stat().st_size}
 (TEST/'report.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
