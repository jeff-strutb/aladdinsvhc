#!/usr/bin/env python3
"""extract_assets.py ROM OUTDIR: the game's data from the player's own copy of
the cartridge. Checks the ROM, then writes OUTDIR/cart.dat: an image of the
cartridge with the data the game reads at its place and nothing where the
original's code was. The list of what is data is assets.csv beside this
(made by make_assets_csv.py from the decompilation's code map)."""
import csv, hashlib, os, sys

SHA1 = 'bd5fea18ce7456cc0231a39473927d8ea9c6589b'


def main():
    rom = open(sys.argv[1], 'rb').read()
    if len(rom) & 0x200:
        rom = rom[0x200:]
    if hashlib.sha1(rom).hexdigest() != SHA1:
        sys.exit('extract_assets: this is not the expected Aladdin (USA) cartridge')
    image = bytearray(len(rom))
    kept = {}
    here = os.path.dirname(os.path.abspath(__file__))
    with open(os.path.join(here, 'assets.csv')) as f:
        for row in csv.DictReader(f):
            start, end = int(row['start'], 16), int(row['end'], 16)
            image[start:end] = rom[start:end]
            kept[row['reader']] = kept.get(row['reader'], 0) + end - start
    os.makedirs(sys.argv[2], exist_ok=True)
    with open(os.path.join(sys.argv[2], 'cart.dat'), 'wb') as f:
        f.write(image)
    print('cart.dat %d B: %s; %d B left out as code' % (
        len(image), ', '.join('%s %d B' % kv for kv in sorted(kept.items())), len(image) - sum(kept.values())))


main()
