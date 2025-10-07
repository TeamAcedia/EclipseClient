from PIL import Image, ImageDraw, ImageFont
from matplotlib import font_manager
import string, math, os

FONT_NAME = "Unispace"
FONT_SIZE = 64
TILE_SIZE = 128
CHARS = string.ascii_uppercase + string.ascii_lowercase + string.digits + "._-"

CUSTOM_ICONS = {
    "*": "textures/base/pack/logo.png"
}

# Combine all characters and icon keys for atlas layout
ALL_KEYS = list(CHARS) + list(CUSTOM_ICONS.keys())

# Compute atlas layout
num_items = len(ALL_KEYS)
atlas_cols = math.ceil(math.sqrt(num_items))
atlas_rows = math.ceil(num_items / atlas_cols)

# Load font
FONT_PATH = font_manager.findfont(FONT_NAME, fallback_to_default=True)
font = ImageFont.truetype(FONT_PATH, FONT_SIZE)

# Get font metrics
ascent, descent = font.getmetrics()
baseline_y = (TILE_SIZE - ascent + descent) // 2 + ascent  # vertical centering of baseline

# Create atlas
atlas = Image.new("RGBA", (TILE_SIZE * atlas_cols, TILE_SIZE * atlas_rows), (0, 0, 0, 0))
draw = ImageDraw.Draw(atlas)
char_map = {}

for i, key in enumerate(ALL_KEYS):
    col = i % atlas_cols
    row = i // atlas_cols
    tile_x = col * TILE_SIZE
    tile_y = row * TILE_SIZE

    if key in CUSTOM_ICONS:
        # Load icon
        icon = Image.open(CUSTOM_ICONS[key]).convert("RGBA")
        # Resize if needed to fit TILE_SIZE (optional)
        scale = min((TILE_SIZE / 1.5) / icon.width, (TILE_SIZE / 1.5) / icon.height)
        icon = icon.resize((int(icon.width * scale), int(icon.height * scale)), resample=Image.LANCZOS)
        # Center icon horizontally
        x = tile_x + (TILE_SIZE - icon.width) // 2
        # Align vertically on the same baseline as text
        y = tile_y + baseline_y - int(icon.height // 1.25)  # center roughly like text
        atlas.paste(icon, (x, y), icon)
        char_map[key] = {"x": col, "y": row, "width": int(icon.width // 1.25)}
    else:
        bbox = draw.textbbox((0, 0), key, font=font)
        w = bbox[2] - bbox[0]
        x = tile_x + (TILE_SIZE - w) // 2
        y = tile_y + baseline_y - ascent
        draw.text((x, y), key, font=font, fill=(255, 255, 255, 255))
        char_map[key] = {"x": col, "y": row, "width": w}

# Save atlas
os.makedirs("textures/custom_assets", exist_ok=True)
atlas.save("textures/custom_assets/font_atlas.png")

# Save Lua map
lua_lines = [
    "-- Auto-generated square font map\nreturn {",
    f"\tcols = {atlas_cols},",
    f"\trows = {atlas_rows},",
    f"\ttile_size = {{x={TILE_SIZE},y={TILE_SIZE}}},",
    "\tchars = {"
]
for key, info in char_map.items():
    escaped = key.replace("\\", "\\\\").replace('"', '\\"')
    lua_lines.append(f'\t\t["{escaped}"]={{x={info["x"]},y={info["y"]},width={info["width"]}}},')
lua_lines.append("\t}\n}")

with open("builtin/eclipse/font_map.lua", "w", encoding="utf-8") as f:
    f.write("\n".join(lua_lines))

print("Font Atlas and Font Map generated successfully and written to disk.")
