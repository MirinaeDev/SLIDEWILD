"""Offline HLSL -> SPIR-V -> Metal, with explicit SDL binding preservation.
Only developers changing world.hlsl need DXC and SPIRV-Cross. Mac builds use
the generated MSL shipped in shaders/metal and verify its source fingerprint.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
REVISION = '300681bbcb8365a8d1eacdacdbdd0249bbf1d6b1'
STAGES = {'vs': ('vs_6_0', 'vs_main', 1), 'ps': ('ps_6_0', 'ps_main', 3),
          'shadow': ('ps_6_0', 'shadow_ps', None)}
OFFSETS = dict(mvp=0, eye=64, weather=80, camera=96, sunMvp=112,
               dynamics=176, raster=192, rayX=208, rayY=224, rayZ=240,
               riderState=256, cloth=320)

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def audit(stage, reflection, metal):
    profile, entry, uniform_set = STAGES[stage]
    assert re.search(r'\b'+entry+r'\(', metal), f'{stage}: missing entrypoint'
    buffers = reflection.get('ubos', [])
    assert len(buffers) == (0 if uniform_set is None else 1)
    if buffers:
        uniform = buffers[0]
        assert (uniform['set'], uniform['binding'], uniform['block_size']) == (uniform_set, 0, 384)
        members = reflection['types'][uniform['type']]['members']
        assert {member['name']: member['offset'] for member in members} == OFFSETS
        assert '[[buffer(0)]]' in metal
    for category, table in [('separate_images', 'texture'), ('separate_samplers', 'sampler')]:
        for resource in reflection.get(category, []):
            assert resource['set'] == 2
            assert re.search(r'\b'+resource['name']+r'\s+\[\['+table+r'\('+str(resource['binding'])+r'\)\]\]', metal), resource
    if stage == 'ps':
        assert sorted(r['binding'] for r in reflection['separate_images']) == list(range(16))
        assert 'center_no_perspective' in metal and 'flat' in metal
    if stage == 'vs':
        assert sorted(r['location'] for r in reflection['inputs']) == [0, 1, 2, 3]
        for index in range(4):
            assert f'[[attribute({index})]]' in metal

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dxc', required=True)
    parser.add_argument('--spirv-cross', required=True)
    args = parser.parse_args()
    output = ROOT / 'shaders/metal'
    intermediate = ROOT / 'build/metal-intermediate'
    output.mkdir(exist_ok=True)
    intermediate.mkdir(parents=True, exist_ok=True)
    manifest = {'source_sha256': digest(ROOT / 'shaders/world.hlsl'),
                'spirv_cross_revision': REVISION, 'msl_version': '2.1', 'files': {}}
    for stage, (profile, entry, _) in STAGES.items():
        spv = intermediate / f'world.{stage}.spv'
        msl = output / f'world.{stage}.msl'
        reflected = output / f'world.{stage}.json'
        subprocess.run([args.dxc, '-spirv', '-fspv-preserve-interface', '-T', profile,
                        '-E', entry, '-O3', '-Fo', str(spv), str(ROOT / 'shaders/world.hlsl')], check=True)
        # Never renumber sparse shadow bindings: texture 8 must remain texture 8.
        subprocess.run([args.spirv_cross, str(spv), '--msl', '--msl-version', '20100',
                        '--msl-decoration-binding', '--output', str(msl)], check=True)
        subprocess.run([args.spirv_cross, str(spv), '--reflect', '--output', str(reflected)], check=True)
        audit(stage, json.loads(reflected.read_text()), msl.read_text())
        manifest['files'][msl.name] = digest(msl)
        manifest['files'][reflected.name] = digest(reflected)
        print(f'PASS {stage}: entrypoint, SDL bindings, uniform ABI and vertex interface')
    (output / 'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')

if __name__ == '__main__':
    main()
