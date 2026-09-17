#!/usr/bin/env python3
"""Assemble the deployable site from web/, with content-hashed file names.

GitHub Pages serves everything with Cache-Control: max-age=600, so for ten
minutes after a deploy a browser can still use the previous app.js or
solver-core.js. Putting the content hash in the file name makes every build a
new URL, so a page only ever loads assets that match it.

    python3 web/make-dist.py web dist

index.html and sw.js keep their plain names: index.html is the entry point,
and the service worker is registered by URL, so a stable one keeps a single
registration across deploys. Both are rewritten to point at the hashed files.
"""

import hashlib
import shutil
import sys
from pathlib import Path


def hashed_name(rel: str, data: bytes) -> str:
    """'board.js' + content -> 'board.<hash>.js' (suffix kept last)."""
    p = Path(rel)
    # as_posix(): the name goes into URLs, so it must not pick up backslashes
    return p.with_name(f"{p.stem}.{hashlib.sha256(data).hexdigest()[:8]}{p.suffix}").as_posix()


def main(src_dir: str, out_dir: str) -> int:
    src = Path(src_dir)
    out = Path(out_dir)

    # Assets that other files reference, in dependency order: every file is
    # rewritten to use the names already published before it.
    hashed_assets = [
        'icons/icon-192.png',
        'icons/icon.svg',
        'styles.css',
        'solver-core.js',
        'board.js',
        'solver-worker.js',   # imports solver-core.js
        'app.js',             # imports board.js / solver-core.js, starts the worker
        'manifest.webmanifest',
    ]
    # Rewritten but kept under their own name
    stable_assets = ['sw.js', 'index.html']

    missing = [f for f in hashed_assets + stable_assets if not (src / f).is_file()]
    if missing:
        print(f"error: missing source file: {', '.join(missing)}", file=sys.stderr)
        return 1

    if out.exists():
        shutil.rmtree(out)
    (out / 'icons').mkdir(parents=True)

    published: dict[str, str] = {}   # original path -> published path

    def rewrite(text: str) -> str:
        for original, new in published.items():
            text = text.replace(original, new)
        return text

    for rel in hashed_assets:
        data = (src / rel).read_bytes()
        if rel.endswith(('.js', '.css', '.webmanifest')):
            data = rewrite(data.decode('utf-8')).encode('utf-8')
        name = hashed_name(rel, data)
        (out / name).write_bytes(data)
        published[rel] = name

    for rel in stable_assets:
        text = rewrite((src / rel).read_text(encoding='utf-8'))
        if rel == 'sw.js':
            # A cache name per build, so an old deploy's entries are dropped
            build_id = hashlib.sha256(
                ''.join(sorted(published.values())).encode('utf-8')).hexdigest()[:8]
            text = text.replace("'puzzlesolver-v1'", f"'puzzlesolver-{build_id}'")
        (out / rel).write_text(text, encoding='utf-8')

    for f in sorted(out.rglob('*')):
        if f.is_file():
            print(f"{f.stat().st_size / 1024:9.1f} KiB  {f.relative_to(out).as_posix()}")
    return 0


if __name__ == '__main__':
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        sys.exit(2)
    sys.exit(main(sys.argv[1], sys.argv[2]))
