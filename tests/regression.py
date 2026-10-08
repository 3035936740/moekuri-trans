"""Run from any directory: python tests/regression.py. Uses only its own test processes."""
from pathlib import Path
import hashlib
import json
import shutil
import struct
import subprocess
import time
import sys
import csv as csvmodule
from font_coverage import glyphs

sys.stdout.reconfigure(encoding='utf-8')

ROOT = Path(__file__).resolve().parent.parent
TEST = ROOT / 'tests'
TOOL = ROOT / 'MoeKuriTools.exe'
EXPECTED = '1c79a2d328d8ccd69765ac624b48f2317d80688c3dc7439ee3c23fc1fc05f847'
REPORT = {}

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def run(*args, timeout=30):
    start = time.perf_counter()
    result = subprocess.run([str(TOOL), *map(str, args)], capture_output=True, timeout=timeout, cwd=ROOT)
    if result.returncode:
        raise AssertionError(result.stdout.decode('utf-8') + result.stderr.decode('utf-8'))
    return result.stdout.decode('utf-8'), round((time.perf_counter() - start) * 1000)

def sections(data):
    nt = struct.unpack_from('<I', data, 60)[0]
    count, = struct.unpack_from('<H', data, nt + 6)
    opt, = struct.unpack_from('<H', data, nt + 20)
    result = {}
    for n in range(count):
        off = nt + 24 + opt + n * 40
        name, vs, va, size, raw = struct.unpack_from('<8sIIII', data, off)
        flags, = struct.unpack_from('<I', data, off + 36)
        result[name.rstrip(b'\0').decode('ascii')] = (data[raw:raw+size], va, vs, flags)
    return result

def main():
    TEST.mkdir(exist_ok=True)
    original = next(p for p in ROOT.parent.glob('*.exe') if p.stat().st_size == 4358144 and digest(p) == EXPECTED)
    REPORT['self_test'] = json.loads(run('--self-test')[0])
    REPORT['languages'] = json.loads(run('--language-list')[0])
    REPORT['validation'] = json.loads(run('--validate', ROOT / 'translation.txt')[0])
    csv = TEST / 'catalog-roundtrip.CsV'
    txt = TEST / 'catalog-roundtrip.txt'
    REPORT['csv_export_ms'] = run('--export-csv', ROOT / 'translation.txt', csv)[1]
    assert csv.read_bytes().startswith(b'\xef\xbb\xbf')
    REPORT['csv_import_ms'] = run('--import-csv', csv, txt)[1]
    canonical = TEST / 'canonical.txt'
    run('--convert', ROOT / 'translation.txt', canonical)
    assert txt.read_bytes() == canonical.read_bytes()
    for ext in ('tsv', 'json'):
        output = TEST / ('format.' + ext)
        run('--convert', txt, output)
        assert json.loads(run('--validate', output)[0])['rows'] == REPORT['validation']['rows']
    REPORT['csv_txt_tsv_json'] = True
    run('--gui-test', TEST / 'gui-pagination.json', timeout=30)
    REPORT['gui'] = json.loads((TEST / 'gui-pagination.json').read_text(encoding='utf-8'))
    assert REPORT['gui']['gui_passed'] == 'true', REPORT['gui']
    patched = ROOT / 'build' / 'optimized.exe'
    REPORT['patch'] = json.loads(run('--pack', original, ROOT / 'translation.txt', patched, '--omit-dxa', '0',
        '--font', ROOT / 'font.ttf', '--height', '0', '--width', '0', '--scale', '100',
        '--max-width', '620', '--min-scale', '70')[0])
    recovered = TEST / 'exact-roundtrip.txt'
    font = TEST / 'exact-roundtrip.ttf'
    run('--unpack', patched, recovered, '--font-out', font, '--settings-out', TEST / 'roundtrip-settings.json')
    assert recovered.read_bytes() == (ROOT / 'translation.txt').read_bytes()
    original_font = (ROOT / 'font.ttf').read_bytes()
    reduced_font = font.read_bytes()
    assert len(reduced_font) < len(original_font)
    used = set(range(256)) | set(range(0x3000,0x3100))
    sources = TEST / 'font-sources.csv'
    run('--extract', original, sources)
    for catalog in (csv, sources):
        with catalog.open(encoding='utf-8-sig',newline='') as file:
            for row in csvmodule.DictReader(file):
                used.update(map(ord,row['Original'] + row['Translation']))
    assert used & glyphs(original_font) <= glyphs(reduced_font)
    REPORT['exact_txt_roundtrip'] = True
    REPORT['used_font_glyphs_preserved'] = True
    REPORT['font_original_bytes'] = len(original_font)
    REPORT['font_subset_bytes'] = len(reduced_font)
    full = TEST / 'full-font.exe'
    run('--pack', original, ROOT / 'translation.txt', full, '--omit-dxa', '0', '--font', ROOT / 'font.ttf', '--subset-font', '0')
    full_font = TEST / 'full-font.ttf'
    run('--unpack', full, TEST / 'full-font.txt', '--font-out', full_font)
    assert full_font.read_bytes() == original_font
    REPORT['full_font_option_exact_roundtrip'] = True
    # The old raw format remains readable by the new tool.
    old = ROOT.parent / 'MoeKuri110_CN.exe'
    if old.exists() and old.stat().st_size == 29322752:
        run('--unpack', old, TEST / 'legacy.txt')
        assert (TEST / 'legacy.txt').read_bytes() == (ROOT / 'translation.txt').read_bytes()
        REPORT['legacy_uncompressed_compatible'] = True
    native_bytes, packed_bytes = original.read_bytes(), patched.read_bytes()
    before, after = sections(native_bytes), sections(packed_bytes)
    # Header growth moves file offsets, so PE debug-directory raw pointers move too.
    # Normalize only those documented metadata fields; code, strings and resources must match.
    nt = struct.unpack_from('<I', native_bytes, 60)[0]
    debug_rva, debug_size = struct.unpack_from('<II', native_bytes, nt + 24 + 96 + 6 * 8)
    for name, section in before.items():
        expected, actual = bytearray(section[0]), bytearray(after[name][0])
        assert section[1:3] == after[name][1:3]
        if section[1] <= debug_rva < section[1] + len(expected):
            for n in range(0, debug_size, 28):
                off = debug_rva - section[1] + n + 24
                actual[off:off+4] = expected[off:off+4]
        assert expected == actual, name
    assert after['.cnro'][3] & 0x40000000 and not after['.cnro'][3] & 0xa0000000
    assert all(not flags & 0x80000000 or not flags & 0x20000000 for _, _, _, flags in after.values())
    REPORT['original_code_strings_resources_unchanged'] = True
    REPORT['debug_raw_offsets_adjusted_for_header_growth'] = True
    REPORT['readonly_translation_section'] = True
    REPORT['no_writable_executable_sections'] = True
    probe = TEST / 'probe'
    probe.mkdir(exist_ok=True)
    game = probe / 'game.exe'
    shutil.copyfile(patched, game)
    result = subprocess.run([str(game), '--cn-probe'], cwd=probe, capture_output=True, timeout=15)
    assert result.returncode == 0, result.returncode
    REPORT['runtime_probe'] = json.loads((probe / 'cn-probe.json').read_text(encoding='utf-8'))
    assert all(REPORT['runtime_probe'].values())
    assert digest(original) == EXPECTED
    REPORT['original_exe_sha256'] = EXPECTED
    REPORT['gameplay_verified'] = False
    REPORT['all_passed'] = True
    print(json.dumps(REPORT, ensure_ascii=False, indent=2))

try:
    main()
except Exception as error:
    REPORT['all_passed'] = False
    REPORT['error'] = str(error)
    raise
finally:
    (TEST / 'regression-report.json').write_text(json.dumps(REPORT, ensure_ascii=False, indent=2), encoding='utf-8')
