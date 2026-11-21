#!/usr/bin/env python3
"""Fails if a PNG screenshot is transparent anywhere.

Usage: tests/check-png-opaque.py <file.png>...
Only the standard library is used: the chunks are parsed by hand. A palette picture must not
have a tRNS entry below 255, a picture with an alpha channel must not have a pixel below 255.
"""
import struct
import sys
import zlib

SIGNATURE = b"\x89PNG\r\n\x1a\n"


def paeth(a, b, c):
    p = a + b - c
    pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    return b if pb <= pc else c


def lowest_alpha(data, width, height, bpp):
    """Lowest value of the last byte of every pixel (8 bit samples, not interlaced)."""
    stride = width * bpp
    lowest = 255
    prev = bytearray(stride)
    pos = 0
    for _ in range(height):
        kind = data[pos]
        line = bytearray(data[pos + 1:pos + 1 + stride])
        pos += 1 + stride
        if kind == 1:
            for i in range(bpp, stride):
                line[i] = (line[i] + line[i - bpp]) & 255
        elif kind == 2:
            for i in range(stride):
                line[i] = (line[i] + prev[i]) & 255
        elif kind == 3:
            for i in range(stride):
                left = line[i - bpp] if i >= bpp else 0
                line[i] = (line[i] + ((left + prev[i]) >> 1)) & 255
        elif kind == 4:
            for i in range(stride):
                left = line[i - bpp] if i >= bpp else 0
                up_left = prev[i - bpp] if i >= bpp else 0
                line[i] = (line[i] + paeth(left, prev[i], up_left)) & 255
        lowest = min(lowest, min(line[bpp - 1::bpp]))
        if lowest == 0:
            break
        prev = line
    return lowest


def check(path):
    with open(path, "rb") as handle:
        data = handle.read()
    if data[:8] != SIGNATURE:
        return "not a PNG file"

    pos = 8
    header = None
    idat = []
    while pos + 8 <= len(data):
        length, kind = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + length]
        pos += 12 + length
        if kind == b"IHDR":
            header = struct.unpack(">IIBBBBB", body)
        elif kind == b"tRNS":
            if header and header[3] == 3:
                if body and min(body) < 255:
                    return "palette with transparent colours (tRNS, lowest alpha %d)" % min(body)
            else:
                return "a colour is marked as transparent (tRNS)"
        elif kind == b"IDAT":
            idat.append(body)
        elif kind == b"IEND":
            break

    if not header:
        return "no IHDR chunk"
    width, height, depth, colour, _, _, interlace = header
    if colour in (4, 6):
        if depth != 8 or interlace:
            return "alpha channel in a layout this check does not read"
        lowest = lowest_alpha(zlib.decompress(b"".join(idat)), width, height, 2 if colour == 4 else 4)
        if lowest < 255:
            return "alpha channel with transparent pixels (lowest alpha %d)" % lowest
    return None


def main():
    failed = 0
    for path in sys.argv[1:]:
        problem = check(path)
        if problem:
            print("FAIL: %s: %s" % (path, problem))
            failed += 1
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
