"""Optional authoring tool: decode supplied music to portable 48 kHz stereo PCM WAV.
Requires PyAV and NumPy; runtime and builds do not need either package.
Usage: python tools/prepare_music.py source.mp3 assets/music/menu.wav
"""
import argparse
from pathlib import Path
import wave
import av
import numpy as np


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    chunks = []
    resampler = av.AudioResampler(format='fltp', layout='stereo', rate=48000)
    with av.open(str(args.source)) as container:
        for frame in container.decode(audio=0):
            chunks.extend(output.to_ndarray() for output in resampler.resample(frame))
        chunks.extend(output.to_ndarray() for output in resampler.resample(None))
    audio = np.concatenate(chunks, axis=1).T
    # Trim only digital silence, then blend the loop seam over 120 ms.
    audible = np.flatnonzero(np.max(np.abs(audio), axis=1) > .0005)
    if not len(audible):
        raise ValueError('Source is silent')
    audio = audio[max(0, audible[0]-240):min(len(audio), audible[-1]+241)]
    overlap = min(5760, len(audio)//8)
    fade = np.linspace(0, 1, overlap, dtype=np.float32)[:, None]
    seam = audio[-overlap:] * (1-fade) + audio[:overlap] * fade
    audio = np.concatenate((audio[overlap:-overlap], seam), axis=0)
    peak = float(np.max(np.abs(audio)))
    audio *= .90/max(peak, .0001)
    pcm = np.rint(np.clip(audio, -1, 1) * 32767).astype('<i2')
    args.destination.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(args.destination), 'wb') as output:
        output.setnchannels(2)
        output.setsampwidth(2)
        output.setframerate(48000)
        output.writeframes(pcm.tobytes())
    print(f'{args.destination}: {len(pcm)/48000:.2f}s, 48 kHz stereo PCM, peak 0.90')


if __name__ == '__main__':
    main()
