import re
from PIL import Image

with open("output.txt") as f:
    log = f.read()

match = re.search(r"===PIXELS_START===\s*(.*?)\s*===PIXELS_END===", log, re.DOTALL)
if not match:
    raise RuntimeError("Couldn't find pixel block.")

hex_data = re.sub(r"[^0-9A-Fa-f]", "", match.group(1))
pixel_bytes = bytes.fromhex(hex_data)

WIDTH = 336
HEIGHT = 300
MODE = "RGB"

expected_len = WIDTH * HEIGHT * 3
if len(pixel_bytes) != expected_len:
    raise RuntimeError(
        f"Mismatched image size: got {len(pixel_bytes)} bytes, expected {expected_len}"
    )

img = Image.frombytes(MODE, (WIDTH, HEIGHT), pixel_bytes)
img.save("output.png")
print("Image saved as output.png")
