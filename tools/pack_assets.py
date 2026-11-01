"""Lossless PNG -> runtime RGBA serialization. No resizing or artistic edits."""
from pathlib import Path
import struct
from PIL import Image

root = Path(__file__).resolve().parents[1]
for source in (root / "assets").glob("*.png"):
    image = Image.open(source).convert("RGBA")
    source.with_suffix(".rgba").write_bytes(struct.pack("<II", *image.size) + image.tobytes())
    print(source.name, image.size)
