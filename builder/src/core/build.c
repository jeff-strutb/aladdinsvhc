/* build.c: checking the player's ROM and assembling the build.
 *
 * Mac: an app bundle
 *   Aladdin.app/Contents/MacOS/Aladdin                 the game
 *              /Contents/Resources/cart.dat            the cartridge's data (aladdin.c)
 *              /Contents/Resources/level_pictures.dat  the level select's pictures
 *              /Contents/Resources/AppIcon.icns        its icon
 * Windows: a folder
 *   Aladdin\Aladdin.exe (its icon built in), cart.dat, level_pictures.dat
 * Linux: a folder
 *   Aladdin/Aladdin             the game
 *          /cart.dat, level_pictures.dat
 *          /icon.png, icon.argb  its icon: the desktop file's, the window's
 *          /Aladdin.desktop      a launcher for the folder as built (copy it to
 *                                ~/.local/share/applications to list the game)
 *
 * level_pictures.dat is made by the build's own game from cart.dat, run once
 * without a window (ALADDIN_MAKE_PICTURES, as native/aladdin/tools/
 * make_pictures.sh runs it), with its save folder pointed at a scratch folder
 * in the staged build, so the player's saves are never touched.
 *
 * The build is made in a hidden folder beside its destination and renamed
 * into place when complete; an earlier build is replaced only when it carries
 * this builder's marker file. */
#include <stdlib.h>
#include <string.h>
#include "rb_int.h"


#ifndef RB_VERSION
#define RB_VERSION "0.0"
#endif

#define MARKER ".aladdin-builder"
#define GAME_SAVE "save"                   /* the game's saves, settings and progress, beside its program (Windows, Linux) */
#define GAME_EXE RB_PRODUCT                /* the Mac app's executable */
#define PICTURES "level_pictures.dat"      /* the level select's pictures (native/aladdin/src/video/level_picture.c) */
#define PICTURES_SAVE ".pictures-save"     /* the picture run's save folder, inside the staged build, removed after */
#define PICTURES_TIMEOUT 300               /* seconds; the run takes about 1 (make_pictures.sh allows the same) */
#define COPIER_HEADER 512u                 /* a copier's header before the ROM (.smc) */

const char *rb_target_name(int t)
{
    return t == RB_MACOS ? "Mac" : t == RB_LINUX ? "Linux" : "Windows";
}

const char *rb_output_name(int t)
{
    return t == RB_MACOS ? RB_PRODUCT ".app" : RB_PRODUCT;
}

void rb_output_path(const rb_job *job, char *out, size_t n)
{
    rb_join(out, n, job->dest_dir, rb_output_name(job->target));
}

/* ---- checks --------------------------------------------------------------------------- */
static void md5_hex(const uint8_t *p, size_t n, char hex[33])
{
    rb_md5 m;
    uint8_t d[16];
    rb_md5_init(&m);
    rb_md5_update(&m, p, n);
    rb_md5_final(&m, d);
    rb_md5_hex(d, hex);
}

/* the ROM, read: the cartridge's bytes (a copier's header left out) when they
 * are No-Intro's; else NULL, with what it is in out */
static uint8_t *load_rom(const char *path, rb_check *out)
{
    size_t n;
    uint8_t *r = rb_read_file(path, &n);
    char md5[33];
    memset(out, 0, sizeof *out);
    if (!r) {
        snprintf(out->note, sizeof out->note, "The file could not be read.");
        return NULL;
    }
    md5_hex(r, n, out->md5);
    if (n == RB_ROM_SIZE && !strcmp(out->md5, RB_ROM_MD5)) {
        out->ok = 1;
        snprintf(out->note, sizeof out->note, RB_ROM_NAME ".");
        return r;
    }
    if (n == RB_ROM_SIZE + COPIER_HEADER) {
        md5_hex(r + COPIER_HEADER, RB_ROM_SIZE, md5);
        if (!strcmp(md5, RB_ROM_MD5)) {
            out->ok = 1;
            memmove(r, r + COPIER_HEADER, RB_ROM_SIZE);
            snprintf(out->note, sizeof out->note, RB_ROM_NAME ", with a copier's 512-byte header: without it, its MD5 is %s.", md5);
            return r;
        }
    }
    free(r);
    snprintf(out->note, sizeof out->note,
             n == RB_ROM_SIZE || n == RB_ROM_SIZE + COPIER_HEADER
                 ? "This is not " RB_ROM_NAME ". The European and Japanese cartridges are not supported."
                 : "This is not " RB_ROM_NAME " (a 1.25 MB SNES ROM).");
    return NULL;
}

int rb_check_rom(const char *path, rb_check *out, rb_progress cb, void *ctx, volatile int *cancel)
{
    uint8_t *r;
    (void)cancel;
    if (cb)
        cb(ctx, 0, NULL);
    r = load_rom(path, out);
    free(r);
    if (cb)
        cb(ctx, 1, NULL);
    return out->ok;
}

/* ---- the output ------------------------------------------------------------------------- */

/* the marker sits in the app's Resources (Mac), or the folder (Windows, Linux) */
static void marker_path(int target, const char *root, char *out, size_t n)
{
    char r[2048];
    if (target == RB_MACOS) {
        rb_join(r, sizeof r, root, "Contents/Resources");
        rb_join(out, n, r, MARKER);
    } else {
        rb_join(out, n, root, MARKER);
    }
}

int rb_output_exists(const rb_job *job, int *ours)
{
    char out[2048], mk[2048];
    rb_output_path(job, out, sizeof out);
    if (ours) {
        marker_path(job->target, out, mk, sizeof mk);
        *ours = rb_exists(mk);
    }
    return rb_exists(out);
}

typedef struct ctx {
    const rb_job *job;
    rb_progress cb;
    void *cctx;
    volatile int *cancel;
    char *err;
    size_t errlen;
    char root[2048];           /* the staged build */
    char res[2048];            /* where the data goes */
    char exe[2048];            /* the game in it */
} ctx;

static void say(ctx *c, double f, const char *s)
{
    if (c->cb)
        c->cb(c->cctx, f, s);
}

static int cancelled(ctx *c)
{
    if (c->cancel && *c->cancel) {
        rb_err(c->err, c->errlen, "the build was cancelled");
        return 1;
    }
    return 0;
}

/* the Mac app's Info.plist */
static int mac_plist(ctx *c)
{
    char path[2048], buf[4096], contents[2048];
    snprintf(buf, sizeof buf,
             "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
             "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
             "<plist version=\"1.0\">\n<dict>\n"
             "    <key>CFBundleName</key><string>%s</string>\n"
             "    <key>CFBundleDisplayName</key><string>%s</string>\n"
             "    <key>CFBundleIdentifier</key><string>com.strutb.aladdin</string>\n"
             "    <key>CFBundleExecutable</key><string>%s</string>\n"
             "    <key>CFBundleIconFile</key><string>AppIcon</string>\n"
             "    <key>CFBundlePackageType</key><string>APPL</string>\n"
             "    <key>CFBundleShortVersionString</key><string>%s</string>\n"
             "    <key>CFBundleVersion</key><string>%s</string>\n"
             "    <key>LSApplicationCategoryType</key><string>public.app-category.action-games</string>\n"
             "    <key>LSMinimumSystemVersion</key><string>12.0</string>\n"
             "    <key>NSHighResolutionCapable</key><true/>\n"
             "    <key>GCSupportsControllerUserInteraction</key><true/>\n"
             "</dict>\n</plist>\n",
             RB_PRODUCT, RB_PRODUCT, GAME_EXE, RB_VERSION, RB_VERSION);
    rb_join(contents, sizeof contents, c->root, "Contents");
    rb_join(path, sizeof path, contents, "Info.plist");
    if (!rb_write_file(path, buf, strlen(buf))) {
        rb_err(c->err, c->errlen, "cannot write the app's Info.plist");
        return 0;
    }
    rb_join(path, sizeof path, contents, "PkgInfo");
    return rb_write_file(path, "APPL????", 8);
}

/* a piece of the payload copied to `to` */
static int payload(ctx *c, const char *name, const char *to, int tree)
{
    char src[2048];
    if (!rb_host_payload(name, src, sizeof src, c->err, c->errlen))
        return 0;
    if (tree ? !rb_copy_tree(src, to) : !rb_copy_file(src, to)) {
        rb_err(c->err, c->errlen, "cannot copy the builder's %s to %s", name, to);
        return 0;
    }
    return 1;
}

/* the staged layout, with the game and its icon in it */
static int stage(ctx *c)
{
    char tmpname[512], path[2048], dir[2048];
    int t = c->job->target;
    snprintf(tmpname, sizeof tmpname, ".%s.building", rb_output_name(t));
    rb_join(c->root, sizeof c->root, c->job->dest_dir, tmpname);
    if (rb_exists(c->root) && !rb_rmtree(c->root)) {        /* a build that stopped partway */
        rb_err(c->err, c->errlen, "cannot clear %s", c->root);
        return 0;
    }
    if (t == RB_MACOS) {
        rb_join(c->res, sizeof c->res, c->root, "Contents/Resources");
        rb_join(dir, sizeof dir, c->root, "Contents/MacOS");
        if (!rb_mkdirs(dir) || !rb_mkdirs(c->res))
            goto nodir;
        rb_join(c->exe, sizeof c->exe, dir, GAME_EXE);
        if (!payload(c, "game", c->exe, 0))
            return 0;
        rb_set_exec(c->exe);
        rb_join(path, sizeof path, c->res, "AppIcon.icns");
        return payload(c, "icon", path, 0);
    }
    snprintf(c->res, sizeof c->res, "%s", c->root);
    if (!rb_mkdirs(c->root))
        goto nodir;
    rb_join(c->exe, sizeof c->exe, c->root, t == RB_LINUX ? RB_PRODUCT : RB_PRODUCT ".exe");
    if (!payload(c, "game", c->exe, 0))
        return 0;
    if (t == RB_LINUX) {
        rb_set_exec(c->exe);
        rb_join(path, sizeof path, c->root, "icon.png");
        if (!payload(c, "icon-png", path, 0))
            return 0;
        rb_join(path, sizeof path, c->root, "icon.argb");
        if (!payload(c, "icon-argb", path, 0))
            return 0;
    }
    return 1;
nodir:
    rb_err(c->err, c->errlen, "cannot make folders in %s (is it writable?)", c->job->dest_dir);
    return 0;
}

/* the game's data, from the ROM */
static int data(ctx *c)
{
    rb_check ck;
    uint8_t *rom, *img;
    size_t n;
    char path[2048];
    int ok;
    say(c, 0.05, "Reading the ROM");
    if (!(rom = load_rom(c->job->rom, &ck))) {
        rb_err(c->err, c->errlen, "%s: %s", c->job->rom, ck.note);
        return 0;
    }
    if (cancelled(c)) {
        free(rom);
        return 0;
    }
    say(c, 0.2, "Taking the game's data from the ROM");
    img = rb_cart(rom, &n, c->err, c->errlen);
    free(rom);
    if (!img)
        return 0;
    say(c, 0.3, "Writing the game's data");
    rb_join(path, sizeof path, c->res, "cart.dat");
    ok = rb_write_file(path, img, n);
    free(img);
    if (!ok)
        rb_err(c->err, c->errlen, "cannot write %s", path);
    return ok && !cancelled(c);
}

/* 32 bits, little endian */
static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

/* level_pictures.dat as the game's reader (native/aladdin/src/video/level_pictures.c) takes it: "LPIC", version 1,
   count, width, height, per picture its level and width x height pixels, all 32-bit little endian, then FNV-1a 32
   of everything before */
static int pictures_valid(const uint8_t *d, size_t n)
{
    uint32_t count, w, h, sum = 2166136261u;
    size_t i;
    if (n < 24 || memcmp(d, "LPIC", 4) || get32(d + 4) != 1)
        return 0;
    count = get32(d + 8), w = get32(d + 12), h = get32(d + 16);
    if (!count || count > 64 || !w || w > 1024 || !h || h > 1024 || n != 20 + (size_t)count * (4 + (size_t)w * h * 4) + 4)
        return 0;
    for (i = 0; i < n - 4; i++)
        sum = (sum ^ d[i]) * 16777619u;
    return sum == get32(d + n - 4);
}

/* the level select's pictures, made by the staged game itself from cart.dat */
static int pictures(ctx *c)
{
    static const char *const args[] = { "--headless", "--updates", "3", NULL };
    char save[2048], out[2048], env_res[2200], env_save[2200], env_out[2200];
    const char *env[] = { env_res, env_save, env_out, NULL };
    uint8_t *d;
    size_t n = 0;
    int r, ok;
    say(c, 0.4, "Making the level select's pictures");
    rb_join(save, sizeof save, c->root, PICTURES_SAVE);
    rb_join(out, sizeof out, c->res, PICTURES);
    if (!rb_mkdirs(save)) {
        rb_err(c->err, c->errlen, "cannot make the folder %s", save);
        return 0;
    }
    snprintf(env_res, sizeof env_res, "PORT_RESDIR=%s", c->res);
    snprintf(env_save, sizeof env_save, "PORT_SAVEDIR=%s", save);
    snprintf(env_out, sizeof env_out, "ALADDIN_MAKE_PICTURES=%s", out);
    r = rb_run(c->exe, args, NULL, env, PICTURES_TIMEOUT, c->cancel);
    rb_rmtree(save);
    if (r == RB_RUN_CANCELLED)
        return !cancelled(c);
    if (r != 0) {
        rb_err(c->err, c->errlen, r == RB_RUN_NOSTART ? "the game could not be started to make the level select's pictures"
                                  : r == RB_RUN_TIMEOUT ? "the game did not finish making the level select's pictures in %d seconds"
                                  : r == RB_RUN_CRASHED ? "the game stopped while making the level select's pictures"
                                                        : "the game could not make the level select's pictures (exit status %d)",
               r == RB_RUN_TIMEOUT ? PICTURES_TIMEOUT : r);
        return 0;
    }
    d = rb_read_file(out, &n);
    ok = d && pictures_valid(d, n);
    free(d);
    if (!ok) {
        rb_err(c->err, c->errlen, "the game's level select pictures (%s) are not valid", out);
        return 0;
    }
    say(c, 0.8, "The level select's pictures are made");
    return !cancelled(c);
}

/* Linux: a launcher for the game where it will be once moved into place */
static int linux_desktop(ctx *c)
{
    char out[2048], path[2048], buf[8192];
    rb_output_path(c->job, out, sizeof out);
    snprintf(buf, sizeof buf,
             "[Desktop Entry]\n"
             "Type=Application\n"
             "Name=%s\n"
             "Comment=Aladdin, built from your cartridge\n"
             "Exec=\"%s/%s\"\n"
             "Path=%s\n"
             "Icon=%s/icon.png\n"
             "Categories=Game;\n"
             "Terminal=false\n",
             RB_PRODUCT, out, RB_PRODUCT, out, out);
    rb_join(path, sizeof path, c->root, RB_PRODUCT ".desktop");
    if (!rb_write_file(path, buf, strlen(buf))) {
        rb_err(c->err, c->errlen, "cannot write the game's desktop file");
        return 0;
    }
    rb_set_exec(path);
    return 1;
}

/* sealed and moved into place */
static int finish(ctx *c)
{
    char out[2048], mk[2048], staged[2048], save[2048];
    int ours = 0, t = c->job->target;
    if (t == RB_MACOS && !mac_plist(c))
        return 0;
    if (t == RB_LINUX && !linux_desktop(c))
        return 0;
    marker_path(t, c->root, mk, sizeof mk);
    if (!rb_write_file(mk, RB_VERSION "\n", strlen(RB_VERSION "\n"))) {
        rb_err(c->err, c->errlen, "cannot write in %s", c->root);
        return 0;
    }
    snprintf(staged, sizeof staged, "%s", c->root);
#ifdef __APPLE__
    if (t == RB_MACOS) {       /* sign the bundle as a whole (ad hoc), so its parts are sealed together */
        const char *const args[] = { "--force", "--sign", "-", c->root, NULL };
        say(c, 0.9, "Signing the app");
        if (rb_run("/usr/bin/codesign", args, NULL, NULL, 120, NULL) != 0) {
            rb_err(c->err, c->errlen, "signing the app failed (codesign)");
            return 0;
        }
    }
#endif
    rb_output_path(c->job, out, sizeof out);
    if (rb_output_exists(c->job, &ours)) {
        if (!ours) {
            rb_err(c->err, c->errlen, "%s already exists and was not made by this builder; move it away or pick another folder", out);
            return 0;
        }
        /* The game keeps its saves, settings and progress in a folder "save" beside
           its program (nshvc_game.save_in_game_folder): on Windows and Linux
           that is inside the folder this build replaces. An earlier build
           that has one is updated in place, the folder "save" (and the
           marker, which keeps it ours should this stop partway) left where
           it is and everything else replaced; it is never deleted. On the
           Mac it is beside the app, outside what is replaced. */
        rb_join(save, sizeof save, out, GAME_SAVE);
        if ((t == RB_WINDOWS || t == RB_LINUX) && rb_is_dir(save)) {
            if (!rb_clear_except(out, GAME_SAVE, MARKER) || !rb_copy_tree(staged, out)) {
                rb_err(c->err, c->errlen, "cannot update the earlier build at %s (your saves and settings in its \"%s\" folder are untouched)", out, GAME_SAVE);
                return 0;
            }
            rb_rmtree(staged);
            return 1;
        }
        if (!rb_rmtree(out)) {
            rb_err(c->err, c->errlen, "cannot replace the earlier build at %s", out);
            return 0;
        }
    }
    if (!rb_rename(staged, out)) {
        rb_err(c->err, c->errlen, "cannot move the build into place at %s", out);
        return 0;
    }
    return 1;
}

int rb_build(const rb_job *job, rb_progress cb, void *cctx, volatile int *cancel, char *err, size_t errlen)
{
    ctx c;
    int ok;
    memset(&c, 0, sizeof c);
    c.job = job;
    c.cb = cb;
    c.cctx = cctx;
    c.cancel = cancel;
    c.err = err;
    c.errlen = errlen;
    if (err && errlen)
        err[0] = 0;
    if (!rb_is_dir(job->dest_dir) && !rb_mkdirs(job->dest_dir)) {
        rb_err(err, errlen, "cannot make the folder %s", job->dest_dir);
        return 0;
    }
    say(&c, 0, "Preparing");
    ok = stage(&c) && data(&c) && pictures(&c) && finish(&c);
    if (!ok) {
        if (c.root[0])
            rb_rmtree(c.root);                         /* leave nothing half-made behind */
        if (err && errlen && !err[0])
            rb_err(err, errlen, "the build failed");
    } else {
        say(&c, 1.0, "Done");
    }
    return ok;
}
