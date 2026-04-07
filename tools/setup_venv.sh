#!/usr/bin/env bash
# One-time setup: creates the Python venv used by gen_images.sh.
# Run from the repo root: bash tools/setup_venv.sh
#
# The venv lives at tools/venv/ (gitignored).
# Re-run any time you update tools/requirements.txt.

set -e

VENV="tools/venv"

if [ -d "$VENV" ]; then
    echo "Venv already exists at $VENV."
    echo "To recreate: rm -rf $VENV && bash tools/setup_venv.sh"
    exit 0
fi

python3 -m venv "$VENV"
source "$VENV/bin/activate"
pip install --quiet -r tools/requirements.txt
echo "Done. Run 'bash tools/gen_images.sh' to convert images."
