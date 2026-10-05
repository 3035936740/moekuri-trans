"""Windows CI checks; no proprietary game, external fonts or Python packages needed."""
from pathlib import Path
import hashlib
import json
import struct
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parent.parent
SYSTEM_DLLS = {'kernel32.dll', 'user32.dll', 'gdi32.dll', 'comctl32.dll',
               'comdlg32.dll', 'shell32.dll', 'bcrypt.dll', 'ole32.dll',
               'oleaut32.dll', 'cabinet.dll', 'fontsub.dll', 'advapi32.dll',
               'ntdll.dll', 'shlwapi.dll', 'version.dll', 'winmm.dll'}
RELEASE_FILES = {'MoeKuriTools.exe', 'README.md', '1.10trans.txt', 'font.ttf',
                 'FONT-LICENSE-OFL.txt', 'FONT-NOTICES.txt'}

def pe_imports(path):
    b = path.read_bytes()
    assert b[:2] == b'MZ'
    nt, = struct.unpack_from('<I', b, 60)
    assert b[nt:nt+4] == b'PE\0\0'
    machine, count = struct.unpack_from('<HH', b, nt+4)
    assert machine == 0x14c, (path, 'Expected x86')
    opt_size, = struct.unpack_from('<H', b, nt+20)
    assert struct.unpack_from('<H', b, nt+24)[0] == 0x10b
    sects = [struct.unpack_from('<IIII', b, nt+24+opt_size+i*40+8) for i in range(count)]
    def offset(rva):
        for vs, va, size, raw in sects:
            if va <= rva < va + size:
                return raw + rva - va
        raise AssertionError(('Unmapped RVA', rva))
    import_rva, import_size = struct.unpack_from('<II', b, nt+24+96+8)
    names = set()
    if import_rva:
        p = offset(import_rva)
        for _ in range(import_size // 20):
            name, = struct.unpack_from('<I', b, p+12)
            if not name:
                break
            n = offset(name)
            names.add(b[n:b.index(0,n)].decode('ascii').lower())
            p += 20
    assert names, ('No imports', path)
    assert not names - SYSTEM_DLLS, ('Unexpected non-system DLL dependencies', path, names - SYSTEM_DLLS)
    return sorted(names)

def run(tool, *args, cwd=ROOT):
    result = subprocess.run([str(tool), *map(str,args)], cwd=cwd, capture_output=True, timeout=30)
    assert result.returncode == 0, (args, result.returncode, result.stdout, result.stderr)
    return result.stdout.decode('utf-8')

def main():
    tool = ROOT / 'MoeKuriTools.exe'
    runtime = ROOT / 'build/runtime.dll'
    imports = {p.name: pe_imports(p) for p in (tool,runtime)}
    assert runtime.read_bytes() in tool.read_bytes(), 'Runtime DLL was not embedded in the GUI'
    assert all(json.loads(run(tool, '--self-test')).values())
    assert len(json.loads(run(tool, '--language-list'))) == 11
    assert json.loads(run(tool, '--validate', ROOT/'translation.txt'))['valid']
    assert json.loads(run(tool, '--validate', ROOT/'1.10trans.txt'))['valid']
    with tempfile.TemporaryDirectory(dir=ROOT/'build', prefix='ci-') as tmp:
        tmp = Path(tmp)
        for ext in ('csv', 'tsv', 'json'):
            out = tmp / ('catalog.'+ext)
            run(tool, '--convert', ROOT/'translation.txt', out)
            assert json.loads(run(tool, '--validate', out))['valid']
        if '--package' in sys.argv:
            archive = ROOT/'dist/moekuri-trans-windows-x86.zip'
            font = ROOT/'dist/font.ttf'
            assert font.read_bytes() == (ROOT/'font.ttf').read_bytes()
            checksums = (ROOT/'dist/SHA256SUMS.txt').read_text(encoding='utf-8').splitlines()
            assert checksums == [hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.name
                                 for p in (archive, font)]
            with zipfile.ZipFile(archive) as zip:
                assert len(zip.namelist()) == len(RELEASE_FILES)
                assert set(zip.namelist()) == RELEASE_FILES
                for name in RELEASE_FILES:
                    assert zip.read(name) == (ROOT/name).read_bytes()
                zip.extractall(tmp)
            # Prove startup from the distributed files without build/runtime.dll.
            assert all(json.loads(run(tmp/'MoeKuriTools.exe','--self-test',cwd=tmp)).values())
    report = {'passed':True,'embedded_runtime_dll':True,'imports':imports,
              'release_allowlist':sorted(RELEASE_FILES),'package_checked':'--package' in sys.argv}
    (ROOT/'build/ci-smoke.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps(report,indent=2))

if __name__ == '__main__':
    main()
