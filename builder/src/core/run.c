/* run.c: running a program the build ships (the game, to make its level
 * select's pictures), waited for with a time limit and the build's cancel.
 * POSIX: posix_spawn with the environment given; Windows: CreateProcessW with
 * the variables set in the builder's own environment for the call, then put
 * back. */
#include <stdlib.h>
#include <string.h>
#include "rb_int.h"

#define POLL_MS 20

#ifdef _WIN32
#include <windows.h>

static wchar_t *wide(const char *s)
{
    int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
    wchar_t *w = malloc(sizeof(wchar_t) * (size_t)(n > 0 ? n : 1));
    if (w)
        MultiByteToWideChar(CP_UTF8, 0, s, -1, w, n > 0 ? n : 1);
    return w;
}

/* one argument quoted for CreateProcess's command line (the C runtime's rules:
   backslashes doubled only before a quote) */
static void quote(wchar_t *out, size_t n, size_t *at, const wchar_t *a)
{
    size_t k = *at;
    int bs = 0;
#define PUT(ch) do { if (k + 1 < n) out[k++] = (ch); } while (0)
    PUT(L'"');
    for (; *a; a++) {
        if (*a == L'\\') {
            bs++;
            continue;
        }
        while (bs) {
            PUT(L'\\');
            if (*a == L'"')
                PUT(L'\\');
            bs--;
        }
        if (*a == L'"')
            PUT(L'\\');
        PUT(*a);
    }
    while (bs--) {
        PUT(L'\\');
        PUT(L'\\');
    }
    PUT(L'"');
#undef PUT
    out[k] = 0;
    *at = k;
}

#define MAX_ENV 8

int rb_run(const char *exe, const char *const args[], const char *cwd, const char *const env[], int timeout_s,
           volatile int *cancel)
{
    wchar_t cmd[8192], *wexe = wide(exe), *wcwd = cwd ? wide(cwd) : NULL;
    wchar_t *name[MAX_ENV] = { 0 }, *old[MAX_ENV] = { 0 };
    int had[MAX_ENV] = { 0 }, ne = 0, i, r = RB_RUN_NOSTART;
    size_t at = 0;
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    DWORD waited = 0, code = 0;

    if (!wexe)
        goto out;
    quote(cmd, sizeof cmd / sizeof cmd[0], &at, wexe);
    for (i = 0; args && args[i]; i++) {
        wchar_t *w = wide(args[i]);
        if (!w)
            goto out;
        if (at + 2 < sizeof cmd / sizeof cmd[0])
            cmd[at++] = L' ';
        quote(cmd, sizeof cmd / sizeof cmd[0], &at, w);
        free(w);
    }
    for (ne = 0; env && env[ne] && ne < MAX_ENV; ne++) {
        wchar_t *w = wide(env[ne]), *eq = w ? wcschr(w, L'=') : NULL;
        DWORD n;
        if (!eq) {
            free(w);
            break;
        }
        *eq = 0;
        name[ne] = w;
        n = GetEnvironmentVariableW(w, NULL, 0);
        if (n) {
            old[ne] = malloc(sizeof(wchar_t) * n);
            had[ne] = old[ne] && GetEnvironmentVariableW(w, old[ne], n) > 0;
        }
        SetEnvironmentVariableW(w, eq + 1);
    }
    memset(&si, 0, sizeof si);
    si.cb = sizeof si;
    if (CreateProcessW(wexe, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, wcwd, &si, &pi)) {
        CloseHandle(pi.hThread);
        for (;;) {
            if (WaitForSingleObject(pi.hProcess, POLL_MS) == WAIT_OBJECT_0) {
                r = GetExitCodeProcess(pi.hProcess, &code) ? (int)code : RB_RUN_CRASHED;
                /* an exception ends a process with its NTSTATUS (0xC0000005 and the like) */
                if (code >= 0xC0000000u)
                    r = RB_RUN_CRASHED;
                break;
            }
            waited += POLL_MS;
            if ((cancel && *cancel) || waited >= (DWORD)timeout_s * 1000u) {
                r = cancel && *cancel ? RB_RUN_CANCELLED : RB_RUN_TIMEOUT;
                TerminateProcess(pi.hProcess, 1);
                WaitForSingleObject(pi.hProcess, 5000);
                break;
            }
        }
        CloseHandle(pi.hProcess);
    }
    for (i = 0; i < ne; i++) {                       /* the builder's own environment as it was */
        SetEnvironmentVariableW(name[i], had[i] ? old[i] : NULL);
        free(name[i]);
        free(old[i]);
    }
out:
    free(wexe);
    free(wcwd);
    return r;
}

#else
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
extern char **environ;

/* the variable `e` ("NAME=value") names `n` */
static int same_name(const char *e, const char *n)
{
    size_t k = strcspn(n, "=");
    return !strncmp(e, n, k) && e[k] == '=';
}

int rb_run(const char *exe, const char *const args[], const char *cwd, const char *const env[], int timeout_s,
           volatile int *cancel)
{
    size_t na = 0, ne = 0, nenv = 0, i, j, k = 0;
    char **argv = NULL, **envp = NULL;
    posix_spawn_file_actions_t fa;
    pid_t pid;
    int st = 0, r = RB_RUN_NOSTART, waited = 0, devnull;
    char here[4096];

    while (args && args[na])
        na++;
    while (env && env[ne])
        ne++;
    while (environ[nenv])
        nenv++;
    argv = calloc(na + 2, sizeof *argv);
    envp = calloc(nenv + ne + 1, sizeof *envp);
    if (!argv || !envp)
        goto out;
    argv[0] = (char *)exe;
    for (i = 0; i < na; i++)
        argv[i + 1] = (char *)args[i];
    for (i = 0; i < nenv; i++) {                     /* the builder's environment, less what env sets */
        int over = 0;
        for (j = 0; j < ne && !over; j++)
            over = same_name(environ[i], env[j]);
        if (!over)
            envp[k++] = environ[i];
    }
    for (j = 0; j < ne; j++)
        envp[k++] = (char *)env[j];
    posix_spawn_file_actions_init(&fa);
    devnull = open("/dev/null", O_RDWR);
    if (devnull >= 0) {
        posix_spawn_file_actions_adddup2(&fa, devnull, 0);
        posix_spawn_file_actions_adddup2(&fa, devnull, 1);
        posix_spawn_file_actions_adddup2(&fa, devnull, 2);
    }
    /* posix_spawn has no working folder before POSIX 2024: the builder's is
       changed for the call (the front ends' paths are all absolute) */
    if (cwd && (!getcwd(here, sizeof here) || chdir(cwd) != 0)) {
        posix_spawn_file_actions_destroy(&fa);
        if (devnull >= 0)
            close(devnull);
        goto out;
    }
    if (posix_spawn(&pid, exe, &fa, NULL, argv, envp) == 0) {
        for (;;) {
            struct timespec ts = { 0, POLL_MS * 1000000L };
            pid_t w = waitpid(pid, &st, WNOHANG);
            if (w == pid) {
                r = WIFEXITED(st) ? WEXITSTATUS(st) : RB_RUN_CRASHED;
                break;
            }
            if (w < 0) {
                r = RB_RUN_CRASHED;
                break;
            }
            waited += POLL_MS;
            if ((cancel && *cancel) || waited >= timeout_s * 1000) {
                r = cancel && *cancel ? RB_RUN_CANCELLED : RB_RUN_TIMEOUT;
                kill(pid, SIGKILL);
                waitpid(pid, &st, 0);
                break;
            }
            nanosleep(&ts, NULL);
        }
    }
    if (cwd && chdir(here) != 0) {
        /* the folder the builder started in is gone: nothing it uses is relative to it */
    }
    posix_spawn_file_actions_destroy(&fa);
    if (devnull >= 0)
        close(devnull);
out:
    free(argv);
    free(envp);
    return r;
}
#endif
