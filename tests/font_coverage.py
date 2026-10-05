"""Read BMP glyph presence from the Microsoft Unicode cmap used by the subsetter."""
import struct

def glyphs(data):
    tables = {}
    for n in range(struct.unpack_from('>H', data, 4)[0]):
        tag, checksum, offset, size = struct.unpack_from('>4sIII', data, 12 + n * 16)
        tables[tag] = (offset, size)
    cmap = tables[b'cmap'][0]
    for n in range(struct.unpack_from('>H', data, cmap + 2)[0]):
        platform, encoding, offset = struct.unpack_from('>HHI', data, cmap + 4 + n * 8)
        sub = cmap + offset
        if platform != 3 or encoding != 1 or struct.unpack_from('>H', data, sub)[0] != 4:
            continue
        count = struct.unpack_from('>H', data, sub + 6)[0] // 2
        ends = sub + 14
        starts = ends + count * 2 + 2
        deltas = starts + count * 2
        offsets = deltas + count * 2
        present = set()
        for i in range(count):
            start = struct.unpack_from('>H', data, starts + i * 2)[0]
            end = struct.unpack_from('>H', data, ends + i * 2)[0]
            delta = struct.unpack_from('>h', data, deltas + i * 2)[0]
            distance = struct.unpack_from('>H', data, offsets + i * 2)[0]
            for c in range(start, end + 1):
                glyph = (c + delta) & 65535
                if distance:
                    glyph = struct.unpack_from('>H', data, offsets + i * 2 + distance + (c-start)*2)[0]
                    if glyph:
                        glyph = (glyph + delta) & 65535
                if glyph:
                    present.add(c)
        return present
    raise AssertionError('Microsoft BMP cmap missing')
