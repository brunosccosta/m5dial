#!/usr/bin/env python3
"""
gen_tag.py — convert a Spotify URL or device ID into the string to write via NFC Tools.

Usage:
  python tools/gen_tag.py <spotify-url>
  python tools/gen_tag.py device <device-id>

Examples:
  python tools/gen_tag.py https://open.spotify.com/album/2WWakvH7foDm8RjWFMDcL7
  → spotify:album:2WWakvH7foDm8RjWFMDcL7

  python tools/gen_tag.py https://open.spotify.com/playlist/37i9dQZF1DX0XUsuxWHRQd?si=abc
  → spotify:playlist:37i9dQZF1DX0XUsuxWHRQd

  python tools/gen_tag.py device acc2c46872b973f91bfad240a11703bc7bbdec06
  → spotify:device:acc2c46872b973f91bfad240a11703bc7bbdec06
"""

import sys
from urllib.parse import urlparse

SUPPORTED_TYPES = {"album", "playlist", "track", "artist", "show", "episode"}

def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)

    # Device puck: gen_tag.py device <id>
    if sys.argv[1] == "device":
        if len(sys.argv) < 3:
            print("error: missing device ID", file=sys.stderr)
            sys.exit(1)
        print(f"spotify:device:{sys.argv[2]}")
        return

    # Spotify URL
    raw = sys.argv[1]
    parsed = urlparse(raw)

    if parsed.netloc not in ("open.spotify.com", "spotify.com"):
        print(f"error: not a Spotify URL: {raw}", file=sys.stderr)
        sys.exit(1)

    parts = parsed.path.strip("/").split("/")
    if len(parts) < 2:
        print(f"error: can't parse path: {parsed.path}", file=sys.stderr)
        sys.exit(1)

    kind, id_ = parts[0], parts[1]
    if kind not in SUPPORTED_TYPES:
        print(f"error: unsupported type '{kind}' (supported: {', '.join(sorted(SUPPORTED_TYPES))})", file=sys.stderr)
        sys.exit(1)

    print(f"spotify:{kind}:{id_}")

if __name__ == "__main__":
    main()
