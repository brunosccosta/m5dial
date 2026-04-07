#!/usr/bin/env bash
# Convert source PNGs in tools/images/ to LVGL C arrays in src/ui/images/.
# Run from the repo root: bash tools/gen_images.sh
#
# Source PNGs must already be at the intended display size.
# Use sips to resize before adding: sips -z <h> <w> source.png --out tools/images/name.png
#
# To add a new image — see src/ui/images/README.md for the full workflow.

set -e

VENV="tools/venv"
SCRIPT=".pio/libdeps/m5stack-stamps3/lvgl/scripts/LVGLImage.py"
SRC="tools/images"
OUT="src/ui/images"

if [ ! -d "$VENV" ]; then
    echo "Error: venv not found. Run 'bash tools/setup_venv.sh' first."
    exit 1
fi

source "$VENV/bin/activate"
mkdir -p "$OUT"

count=0
for png in "$SRC"/*.png; do
    [ -e "$png" ] || { echo "No PNGs found in $SRC/"; exit 0; }
    name=$(basename "$png" .png)
    echo "  $name"
    python3 "$SCRIPT" --ofmt C --cf ARGB8888 --premultiply --compress NONE \
        -o "$OUT" --name "$name" "$png"
    count=$((count + 1))
done

echo "Done — $count image(s) converted. Declare new ones in src/ui/images/lvgl_images.h"
