#!/usr/bin/env python3
"""Rend les sources PlantUML en SVG et PNG via le serveur public PlantUML.

Encodage officiel PlantUML : zlib(deflate) + base64 variante maison.
Utilisation : python3 docs/uml/render.py
"""
import glob
import io
import os
import subprocess
import sys
import urllib.parse
import zlib

SERVER = "https://www.plantuml.com/plantuml"


def _encode6bit(b):
    if b < 10:
        return chr(48 + b)
    b -= 10
    if b < 26:
        return chr(65 + b)
    b -= 26
    if b < 26:
        return chr(97 + b)
    b -= 26
    if b == 0:
        return '-'
    if b == 1:
        return '_'
    return '?'


def _append3bytes(b1, b2, b3):
    c1 = b1 >> 2
    c2 = ((b1 & 0x3) << 4) | (b2 >> 4)
    c3 = ((b2 & 0xF) << 2) | (b3 >> 6)
    c4 = b3 & 0x3F
    return "".join(_encode6bit(c) for c in (c1, c2, c3, c4))


def encode(text):
    data = zlib.compress(text.encode("utf-8"), 9)
    data = data[2:-4]  # retire l'en-tête zlib et le checksum adler32
    res = []
    for i in range(0, len(data), 3):
        chunk = data[i:i + 3]
        res.append(_append3bytes(chunk[0],
                                 chunk[1] if len(chunk) > 1 else 0,
                                 chunk[2] if len(chunk) > 2 else 0))
    return "".join(res)


def render(source, fmt):
    with io.open(source, encoding="utf-8") as fh:
        text = fh.read()
    url = "%s/%s/%s" % (SERVER, fmt, encode(text))
    out = subprocess.run(
        ["curl", "-s", "-m", "60", "-L", "-o", "-", url],
        stdout=subprocess.PIPE, check=True).stdout
    return out


def main():
    files = sorted(glob.glob("docs/uml/[0-9]*.puml"))
    if not files:
        print("aucune source trouvée")
        return 1
    os.makedirs("docs/uml/exports", exist_ok=True)
    fails = []
    for src in files:
        base = os.path.splitext(os.path.basename(src))[0]
        for fmt, ext in (("svg", "svg"), ("png", "png")):
            out = render(src, fmt)
            path = "docs/uml/exports/%s.%s" % (base, ext)
            if len(out) < 500:
                fails.append(path + " (réponse trop courte : %d o)" % len(out))
                continue
            if fmt == "svg" and b"Syntax Error" in out:
                fails.append(path + " (erreur de syntaxe PlantUML)")
                continue
            with open(path, "wb") as fh:
                fh.write(out)
            print("%-46s %8d o" % (path, len(out)))
    print("FAILS:", fails if fails else "aucun")
    return 0


if __name__ == "__main__":
    sys.exit(main())
