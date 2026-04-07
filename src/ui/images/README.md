# LVGL Images

Color images compiled into LVGL C arrays. Used for anything that can't be done with a font — emoji, flags, custom icons.

Generated files live here and are committed. Source PNGs and the conversion script live in `tools/`.

---

## Current images

| File | Variable | Source | Size | Used by |
|---|---|---|---|---|
| `emoji_peach.c` | `emoji_peach` | Twemoji 1F351 | 32×32 | LoveCard (pending) |

---

## How to add a new image

### Step 1 — get the source PNG

For emoji: download from [Twemoji on GitHub](https://github.com/twitter/twemoji) (`assets/72x72/<codepoint>.png`). Apache 2.0 licensed.

For flags: Twemoji also has flag emoji (e.g. `1f1f3-1f1f1.png` for NL).

### Step 2 — resize to the intended display size

Source PNGs must be at the exact pixel dimensions you want on-screen before converting. Use `sips` (built into macOS):

```bash
# Resize to 32×32
sips -z 32 32 source.png --out tools/images/my_image.png
```

The filename (without `.png`) becomes the C variable name. Use `snake_case`.

### Step 3 — run the pipeline

```bash
bash tools/gen_images.sh
```

Output `.c` files land in `src/ui/images/`. The script converts all PNGs in `tools/images/` in one pass.

First-time setup (once per machine):

```bash
bash tools/setup_venv.sh
```

This creates `tools/venv/` (gitignored) with the required Python packages (`pypng`, `lz4`).

### Step 4 — declare the extern

Add one line to `src/ui/images/lvgl_images.h`:

```cpp
extern const lv_image_dsc_t my_image;
```

### Step 5 — use in code

```cpp
#include "../images/lvgl_images.h"

lv_obj_t* img = lv_image_create(parent);
lv_image_set_src(img, &emoji_peach);
lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
```

---

## Technical notes

- **Color format**: `ARGB8888` — 4 bytes/pixel. 32×32 = 4 KB per image. Straightforward; LVGL handles blending.
- **Premultiplied alpha**: enabled (`--premultiply`). Required for correct rendering of semi-transparent edges (emoji have lots of these).
- **No compression**: `--compress NONE` — keeps it simple. Images at these sizes don't need it.
- **Converter**: `LVGLImage.py` from the LVGL library (`--ofmt C`). Generates LVGL v9 `lv_image_dsc_t` structs.
- **Resize first**: the converter does no resizing. Pre-resize with `sips`.

---

## Gotchas

- The generated `.c` file includes `lvgl/lvgl.h` — that path works with PlatformIO's library layout. Don't change it.
- Flash cost: 4 KB per 32×32 ARGB8888 image. Cheap at this scale.
