/*
 * sound_compare.c: two sounds (16-bit stereo, 32,040 Hz, counted from the same
 * power-on) compared: the original's (snesbox --sound) and the native's
 * (sound_play --sound).
 *
 *   sound_compare ORIGINAL.RAW NATIVE.RAW FROM
 *
 * From sample FROM (the driver's start) to the shorter's end, prints:
 *   - samples compared, values that differ, and the first difference;
 *   - loudness: for each second in which the original is not near silent
 *     (RMS 64 or more), the native's RMS over the original's; the lowest and
 *     highest of those;
 *   - the native delayed by 0-63 samples: the delay at which the difference's
 *     energy is least, and that energy over the original's. The original's
 *     driver makes its writes over a tick's SPC700 time (its key-ons 20-29
 *     samples after the tick begins); the native's come at the tick's first
 *     sample, so the two can line up only to within that spread.
 */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int16_t *load(const char *path, long *n)
{
    FILE *f = fopen(path, "rb");
    long size;
    int16_t *p;
    if (!f) {
        fprintf(stderr, "sound_compare: cannot read %s\n", path);
        exit(2);
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    p = (int16_t *)malloc((size_t)size + 4);
    if (!p || fread(p, 1, (size_t)size, f) != (size_t)size)
        exit(2);
    fclose(f);
    *n = size / 4;
    return p;
}

int main(int argc, char **argv)
{
    long na, nb, n, from, i, differ = 0, first = -1, sec;
    int16_t *a, *b;
    double lo = 1e9, hi = 0, best = 1e300, eo = 0;
    int best_lag = 0, lag, seconds = 0;

    if (argc != 4) {
        fprintf(stderr, "usage: sound_compare ORIGINAL.RAW NATIVE.RAW FROM\n");
        return 2;
    }
    a = load(argv[1], &na);
    b = load(argv[2], &nb);
    from = atol(argv[3]);
    n = na < nb ? na : nb;
    for (i = from; i < n; i++) {
        if (a[2 * i] != b[2 * i] || a[2 * i + 1] != b[2 * i + 1]) {
            differ++;
            if (first < 0)
                first = i;
        }
        eo += (double)a[2 * i] * a[2 * i] + (double)a[2 * i + 1] * a[2 * i + 1];
    }
    for (sec = from; sec + 32040 <= n; sec += 32040) {
        double x = 0, y = 0, r;
        for (i = sec; i < sec + 32040; i++) {
            x += (double)a[2 * i] * a[2 * i] + (double)a[2 * i + 1] * a[2 * i + 1];
            y += (double)b[2 * i] * b[2 * i] + (double)b[2 * i + 1] * b[2 * i + 1];
        }
        if (sqrt(x / 64080) < 64)
            continue;
        r = sqrt(y / x);
        seconds++;
        if (r < lo) lo = r;
        if (r > hi) hi = r;
    }
    for (lag = 0; lag < 64; lag++) {
        double e = 0;
        for (i = from + 64; i < n; i++) {
            double l = (double)a[2 * i] - b[2 * (i - lag)], r = (double)a[2 * i + 1] - b[2 * (i - lag) + 1];
            e += l * l + r * r;
        }
        if (e < best) {
            best = e;
            best_lag = lag;
        }
    }
    printf("%ld samples compared, %ld differ", n - from, differ);
    if (first >= 0)
        printf(", the first at %ld (original %d,%d, native %d,%d)", first, a[2 * first], a[2 * first + 1], b[2 * first], b[2 * first + 1]);
    printf("; loudness native/original %.3f-%.3f over %d seconds; delayed %d samples the difference is %.1f%% of the original's energy\n",
           seconds ? lo : 0.0, seconds ? hi : 0.0, seconds, best_lag, eo > 0 ? 100.0 * best / eo : 0.0);
    return 0;
}
