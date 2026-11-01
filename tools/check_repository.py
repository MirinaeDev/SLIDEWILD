"""Offline pre-publication audit. Run from any directory with Python 3.10+."""
from pathlib import Path
import os
import re
import struct
import subprocess
import sys
import wave

ROOT = Path(__file__).resolve().parents[1]
EXCLUDED = {'.git', '.tools', '.local-archive', 'dist', 'captures', 'media',
            'backups', 'reference-analysis', '__pycache__'}


def main():
    errors = []
    source = (ROOT / 'src/renderer.cpp').read_text(encoding='utf-8')
    array = re.search(r'const char\* names\[\]=\{(.*?)\};', source, re.S)
    names = re.findall(r'"([^"]+)"', array.group(1))
    if len(names) != 82 or len(set(names)) != len(names):
        errors.append('Runtime asset inventory must contain 82 unique names')
    for name in names:
        png = ROOT / f'assets/{name}.png'
        rgba = ROOT / f'assets/{name}.rgba'
        if not png.is_file() or not rgba.is_file():
            errors.append(f'Missing PNG or RGBA: {name}')
            continue
        with png.open('rb') as file:
            header = file.read(24)
        with rgba.open('rb') as file:
            dimensions = struct.unpack('<II', file.read(8))
        if header[:8] != b'\x89PNG\r\n\x1a\n' or struct.unpack('>II', header[16:24]) != dimensions:
            errors.append(f'PNG / RGBA dimension mismatch: {name}')
        if rgba.stat().st_size != 8 + dimensions[0] * dimensions[1] * 4:
            errors.append(f'Invalid RGBA byte count: {name}')

    # Use the actual Git publication inventory when available.
    try:
        result = subprocess.run(['git', '-C', str(ROOT), 'ls-files', '--cached',
                                 '--others', '--exclude-standard', '-z'],
                                capture_output=True, check=True)
        paths = [ROOT / p for p in result.stdout.decode('utf-8').split('\0') if p]
    except (FileNotFoundError, subprocess.CalledProcessError):
        paths = []
        for directory, children, filenames in os.walk(ROOT):
            children[:] = [name for name in children if name not in EXCLUDED and not name.startswith('build')]
            paths.extend(Path(directory) / name for name in filenames if not name.endswith('.log'))
    key_pattern = re.compile(r'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----|'
                             r'\bgh[pousr]_[A-Za-z0-9]{30,}|\bAKIA[0-9A-Z]{16}\b')
    for path in paths:
        if path.stat().st_size >= 95 * 1024 * 1024:
            errors.append(f'File too large for normal Git: {path.relative_to(ROOT)}')
        if path.suffix in {'.sh', '.ps1', '.py', '.cpp', '.hpp', '.inc', '.md', '.txt', '.yml', '.json'}:
            text = path.read_text(encoding='utf-8', errors='replace')
            if key_pattern.search(text):
                errors.append(f'Potential private key/token: {path.relative_to(ROOT)}')
            if re.search(r'[A-Z]:[/\\](?:Users[/\\]|downhil)', text, re.I):
                errors.append(f'Local machine path: {path.relative_to(ROOT)}')
        if path.suffix == '.sh' and b'\r' in path.read_bytes():
            errors.append(f'Shell script needs LF endings: {path.relative_to(ROOT)}')
    for name in ('Bungee-OFL.txt', 'Rajdhani-OFL.txt', 'SDL3.txt'):
        if not (ROOT / 'licenses' / name).is_file():
            errors.append(f'Missing distributed notice: {name}')
    for track in ('menu', 'course'):
        try:
            with wave.open(str(ROOT / f'assets/music/{track}.wav')) as music:
                if (music.getnchannels(), music.getframerate(), music.getsampwidth()) != (2, 48000, 2):
                    errors.append(f'Unexpected music format: {track}')
                if music.getnframes() < 48000:
                    errors.append(f'Music too short: {track}')
        except (OSError, wave.Error) as error:
            errors.append(f'Missing or invalid music {track}: {error}')
    size = sum(p.stat().st_size for p in paths) / 1024**2
    print(f'Publication inventory: {len(paths)} files, {size:.1f} MiB; {len(names)} runtime assets')
    for error in errors:
        print(f'ERROR: {error}', file=sys.stderr)
    if errors:
        return 1
    print('PASS: runtime inventory, sizes, common secret signatures, portable paths, LF scripts, font notices')
    print('This audit does not establish artwork ownership or replace native GPU testing.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
