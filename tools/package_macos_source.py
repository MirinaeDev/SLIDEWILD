"""Create the self-contained source kit to transfer to an Apple Silicon Mac."""
from pathlib import Path
import hashlib
import json
import re
import zipfile

root = Path(__file__).resolve().parents[1]
name = 'Slidewild-0.35-Mac-AppleSilicon-Sources'
archive = root / 'dist' / (name+'.zip')
archive.parent.mkdir(exist_ok=True)
files = {}
for directory in ('src', 'tests', 'cmake', 'shaders', 'licenses', 'vendor'):
    for file in (root / directory).rglob('*'):
        if file.is_file():
            files[file.relative_to(root).as_posix()] = file
for relative in ('CMakeLists.txt', 'build-macos.sh', 'test-macos.sh',
                 'tools/generate_metal.py', 'tools/package_macos_source.py',
                 'docs/MACOS-BUILD.txt', 'docs/CREDITS.md'):
    files[relative] = root / relative
files['README.txt'] = root / 'docs/MACOS-BUILD.txt'
files['CREDITS.md'] = root / 'docs/CREDITS.md'
array = re.search(r'const char\* names\[\]=\{(.*?)\};', (root/'src/renderer.cpp').read_text()).group(1)
names = re.findall(r'"([^"]+)"', array)
assert len(names) == len(set(names)) == 82
for asset in names:
    files[f'assets/{asset}.rgba'] = root / f'assets/{asset}.rgba'
for track in ('menu', 'course'):
    files[f'assets/music/{track}.wav'] = root / f'assets/music/{track}.wav'
manifest = {}
with zipfile.ZipFile(archive, 'x', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as z:
    for relative, original in sorted(files.items()):
        data = original.read_bytes()
        if relative.endswith('.sh'):
            assert b'\r' not in data and not data.startswith(b'\xef\xbb\xbf'), relative
        manifest[relative] = hashlib.sha256(data).hexdigest()
        item = zipfile.ZipInfo(f'{name}/{relative}')
        item.create_system = 3
        item.external_attr = (0o100755 if relative.endswith('.sh') else 0o100644) << 16
        item.compress_type = zipfile.ZIP_DEFLATED
        z.writestr(item, data)
    z.writestr(f'{name}/manifest-sha256.json', json.dumps(manifest, indent=2)+'\n')
digest = hashlib.sha256(archive.read_bytes()).hexdigest()
archive.with_suffix('.zip.sha256.txt').write_text(f'{digest}  {archive.name}\n')
print(f'{archive}\n{len(files)} files / {archive.stat().st_size/1024**2:.1f} MiB\nSHA256 {digest}')
