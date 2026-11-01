"""Package only runtime files; execute after build and native validation."""
from pathlib import Path
import hashlib
import json
import re
import shutil
import zipfile

root = Path(__file__).resolve().parents[1]
name = 'Slidewild-0.35-Windows-x64'
destination = root / 'dist' / name
# Never mix a release with stale files or replace someone's extracted game.
destination.mkdir(parents=True, exist_ok=False)
files = {'Slidewild.exe': root / 'build/Slidewild.exe',
         'SDL3.dll': root / 'build/SDL3.dll',
         'README.txt': root / 'docs/PLAYER-GUIDE.txt'}
source = (root / 'src/renderer.cpp').read_text()
array = re.search(r'const char\* names\[\]=\{(.*?)\};', source).group(1)
names = re.findall(r'"([^"]+)"', array)
assert len(names) == 82 and len(set(names)) == 82
for asset in names:
    files[f'assets/{asset}.rgba'] = root / f'assets/{asset}.rgba'
for track in ('menu', 'course'):
    files[f'assets/music/{track}.wav'] = root / f'assets/music/{track}.wav'
files['CREDITS.md'] = root / 'docs/CREDITS.md'
for stage in ('vs', 'ps', 'shadow'):
    files[f'shaders/world.{stage}.dxil'] = root / f'build/shaders/world.{stage}.dxil'
for license_file in (root / 'licenses').glob('*.txt'):
    files[f'licenses/{license_file.name}'] = license_file
manifest = {}
for relative, original in sorted(files.items()):
    target = destination / relative
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(original, target)
    manifest[relative] = hashlib.sha256(target.read_bytes()).hexdigest()
(destination / 'manifest-sha256.json').write_text(json.dumps(manifest, indent=2)+'\n')
archive = destination.parent / (destination.name + '.zip')
with zipfile.ZipFile(archive, 'x', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as package:
    for file in sorted(destination.rglob('*')):
        if file.is_file():
            package.write(file, f'{name}/{file.relative_to(destination).as_posix()}')
checksum = hashlib.sha256(archive.read_bytes()).hexdigest()
archive.with_suffix('.zip.sha256.txt').write_text(f'{checksum}  {archive.name}\n')
print(f'{archive}\n{archive.stat().st_size / 1024**2:.1f} MiB / {len(files)} runtime files\nSHA256 {checksum}')
