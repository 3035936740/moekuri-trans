"""Audit all JP 1.10 dialogue lines used by the native line reader."""
from pathlib import Path
import json
from sample_marker import rows

ROOT=Path(__file__).resolve().parent.parent
reference=ROOT.parents[2]/'mods/trans/localization/jp-1.10/resource_original.json'
if not reference.exists():
    raise SystemExit('Local game reference is unavailable; native coverage audit skipped.')
resources=json.loads(reference.read_text(encoding='utf-8-sig'))
catalog=rows(ROOT/'1.10trans.txt')
checked=0;commands=0;files=0;missing=[]
for path,body in resources.items():
    if not path.startswith('data/dat/story/'):continue
    files+=1;dialogue=False
    for line in body.splitlines():
        if line.startswith('#'):
            commands+=1;dialogue=line.startswith('#txt@');continue
        if not dialogue or not line.strip():continue
        checked+=1
        if line not in catalog:missing.append([path,line]);continue
        target=catalog[line][0]
        assert target and '\n' not in target and '\r' not in target and not target.startswith('#'),(path,line,target)
assert not missing,missing
report={'passed':True,'story_files':files,'dialogue_lines_covered':checked,
        'scenario_command_lines':commands,'unmatched_dialogue_lines':len(missing)}
(ROOT/'tests/story-catalog-report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps(report,indent=2))
