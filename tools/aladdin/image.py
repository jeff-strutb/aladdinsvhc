#!/usr/bin/env python3
"""image.py: the image gate. The companion ROM built from source must equal
the reference ROM byte for byte.

    image.py            exit 0 when 0 bytes differ, else list the first and exit 1
"""
import os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
B = os.path.join(ROOT, 'build', 'aladdin')


def main():
    ref = open(os.path.join(B, 'baserom.sfc'), 'rb').read()
    out = os.path.join(B, 'snes', 'aladdin.sfc')
    if not os.path.exists(out):
        print('image: no built ROM (%s)' % os.path.relpath(out, ROOT)); return 1
    got = open(out, 'rb').read()
    bad = [i for i in range(max(len(ref), len(got))) if i >= len(ref) or i >= len(got) or ref[i] != got[i]]
    if len(got) != len(ref):
        print('image: size %d, reference %d' % (len(got), len(ref)))
    if bad:
        print('image: %d bytes differ; first at ROM %06X' % (len(bad), bad[0]))
        for i in bad[:16]:
            print('  %06X built %s reference %s' % (i, '%02X' % got[i] if i < len(got) else '--',
                                                    '%02X' % ref[i] if i < len(ref) else '--'))
        return 1
    print('image: %d bytes, 0 differ' % len(ref))
    return 0


if __name__ == '__main__':
    sys.exit(main())
