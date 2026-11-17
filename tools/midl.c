/*
 * midl.c - a MIDL command line for widl.
 *
 * Takes the options of Microsoft's MIDL compiler as Visual C++ projects
 * pass them (/tlb, /h, /iid, /proxy, /dlldata, /out, /I, /D, /env,
 * /mktyplib203 and the rest) and runs widl, which sits next to it, once
 * for each output. The include and library paths of a clang-dos sysroot
 * come first, so imports of the system IDL files (oaidl.idl, ocidl.idl)
 * and importlib("stdole2.tlb") work with no setup.
 *
 * An .odl file, or any file with /mktyplib203, is a MKTYPLIB type library
 * source: it uses BSTR, VARIANT and the other Automation types without
 * importing them, as MKTYPLIB allowed, so midl compiles it with an import
 * of oaidl.idl in front.
 *
 *   midl [options] file.idl|file.odl
 *
 *   MIDL_SYSROOT   the sysroot to take IDL files and type libraries from,
 *                  a name under ../sysroots or a path (default win2k-xp);
 *                  also /sysroot NAME
 *   WIDL           the widl to run (default: widl next to this program)
 *
 * Part of clang-retro win32-sdk.
 * SPDX-License-Identifier: AGPL-3.0-only
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef _WIN32
#include <process.h>
#include <windows.h>
#define PATHSEP '\\'
#define EXE ".exe"
#else
#include <sys/wait.h>
#include <unistd.h>
#define PATHSEP '/'
#define EXE ""
#endif

#define MAXARGS 512

static const char* g_args[MAXARGS];  /* options passed to every widl run */
static int g_nargs;
static int g_verbose;

static void add(const char* s) {
  if (g_nargs >= MAXARGS - 16) {
    fprintf(stderr, "midl: too many options\n");
    exit(2);
  }
  g_args[g_nargs++] = s;
}

static char* cat3(const char* a, const char* b, const char* c) {
  size_t n = strlen(a) + strlen(b) + strlen(c) + 1;
  char* p = malloc(n);
  if (!p) exit(2);
  snprintf(p, n, "%s%s%s", a, b, c);
  return p;
}

static int is_abs(const char* p) {
#ifdef _WIN32
  return p[0] == '\\' || p[0] == '/' || (p[0] && p[1] == ':');
#else
  return p[0] == '/';
#endif
}

/* The directory this program is in. */
static char* self_dir(const char* argv0) {
  char buf[4096];
#ifdef _WIN32
  DWORD n = GetModuleFileNameA(NULL, buf, sizeof buf);
  if (n == 0 || n >= sizeof buf) snprintf(buf, sizeof buf, "%s", argv0);
#else
  ssize_t n = readlink("/proc/self/exe", buf, sizeof buf - 1);
  if (n > 0) buf[n] = 0;
  else snprintf(buf, sizeof buf, "%s", argv0);
#endif
  char* s = strrchr(buf, PATHSEP);
#ifdef _WIN32
  char* s2 = strrchr(buf, '/');
  if (s2 > s) s = s2;
#endif
  if (!s) return strdup(".");
  *s = 0;
  return strdup(buf);
}

static int file_exists(const char* p) {
  FILE* f = fopen(p, "rb");
  if (!f) return 0;
  fclose(f);
  return 1;
}

static int run(const char** argv) {
  if (g_verbose) {
    for (int i = 0; argv[i]; i++) fprintf(stderr, "%s%s", i ? " " : "", argv[i]);
    fprintf(stderr, "\n");
  }
  fflush(stdout);
  fflush(stderr);
#ifdef _WIN32
  intptr_t r = _spawnv(_P_WAIT, argv[0], argv);
  if (r == -1) {
    fprintf(stderr, "midl: cannot run %s: %s\n", argv[0], strerror(errno));
    return 2;
  }
  return (int)r;
#else
  pid_t pid = fork();
  if (pid == 0) {
    execv(argv[0], (char* const*)argv);
    fprintf(stderr, "midl: cannot run %s: %s\n", argv[0], strerror(errno));
    _exit(127);
  }
  int st = 0;
  if (pid < 0 || waitpid(pid, &st, 0) < 0) return 2;
  return WIFEXITED(st) ? WEXITSTATUS(st) : 2;
#endif
}

/* An option's argument: the rest of the word or the next word. */
static const char* optarg_of(const char* rest, int* i, int argc, char** argv, const char* name) {
  if (*rest) return rest;
  if (*i + 1 >= argc) {
    fprintf(stderr, "midl: %s needs an argument\n", name);
    exit(2);
  }
  return argv[++*i];
}

static int opt_is(const char* a, const char* name) { return strcmp(a + 1, name) == 0; }

int main(int argc, char** argv) {
  int win64 = 0; /* MIDL's default environment is win32 */
  const char *tlb = NULL, *header = NULL, *iid = NULL, *proxy = NULL, *dlldata = NULL;
  const char *client = NULL, *server = NULL, *outdir = NULL, *input = NULL;
  const char* sysroot = getenv("MIDL_SYSROOT");
  int mktyplib = 0, notlb = 0, no_client = 0, no_server = 0;
  char* dir = self_dir(argv[0]);

  for (int i = 1; i < argc; i++) {
    const char* a = argv[i];
    if (a[0] != '/' && a[0] != '-') {
      input = a;
      continue;
    }
#ifndef _WIN32
    /* On POSIX systems a path can start with '/': take an existing file
       as the input. */
    if (a[0] == '/' && file_exists(a) && strchr(a + 1, '/')) {
      input = a;
      continue;
    }
#endif
    const char* o = a + 1;
    if (o[0] == '-') o++; /* --option */
    if (opt_is(o - 1, "tlb")) tlb = optarg_of("", &i, argc, argv, a);
    else if (opt_is(o - 1, "h") || opt_is(o - 1, "header")) header = optarg_of("", &i, argc, argv, a);
    else if (opt_is(o - 1, "iid")) iid = optarg_of("", &i, argc, argv, a);
    else if (opt_is(o - 1, "proxy")) proxy = optarg_of("", &i, argc, argv, a);
    else if (opt_is(o - 1, "dlldata")) dlldata = optarg_of("", &i, argc, argv, a);
    else if (opt_is(o - 1, "cstub")) client = optarg_of("", &i, argc, argv, a);
    else if (opt_is(o - 1, "sstub")) server = optarg_of("", &i, argc, argv, a);
    else if (opt_is(o - 1, "out")) outdir = optarg_of("", &i, argc, argv, a);
    else if (opt_is(o - 1, "acf")) add(cat3("--acf=", optarg_of("", &i, argc, argv, a), ""));
    else if (opt_is(o - 1, "sysroot")) sysroot = optarg_of("", &i, argc, argv, a);
    else if (opt_is(o - 1, "mktyplib203")) mktyplib = 1;
    else if (opt_is(o - 1, "notlb")) notlb = 1;
    else if (opt_is(o - 1, "client")) no_client = strcmp(optarg_of("", &i, argc, argv, a), "none") == 0;
    else if (opt_is(o - 1, "server")) no_server = strcmp(optarg_of("", &i, argc, argv, a), "none") == 0;
    else if (opt_is(o - 1, "win32")) win64 = 0;
    else if (opt_is(o - 1, "win64") || opt_is(o - 1, "x64") || opt_is(o - 1, "amd64")) win64 = 1;
    else if (opt_is(o - 1, "env")) {
      const char* e = optarg_of("", &i, argc, argv, a);
      win64 = strstr(e, "64") != NULL;
    } else if (opt_is(o - 1, "Zp")) add(cat3("--packing=", optarg_of("", &i, argc, argv, a), ""));
    else if (!strncmp(o, "Zp", 2)) add(cat3("--packing=", o + 2, ""));
    else if (o[0] == 'I') add(cat3("-I", optarg_of(o + 1, &i, argc, argv, a), ""));
    else if (o[0] == 'D') add(cat3("-D", optarg_of(o + 1, &i, argc, argv, a), ""));
    else if (o[0] == 'U') (void)optarg_of(o + 1, &i, argc, argv, a); /* widl has no -U */
    else if (opt_is(o - 1, "Oicf") || opt_is(o - 1, "Oif")) add("-Oif");
    else if (opt_is(o - 1, "Oi")) add("-Oi");
    else if (opt_is(o - 1, "Os")) add("-Os");
    else if (opt_is(o - 1, "verbose") || opt_is(o - 1, "v")) g_verbose = 1;
    else if (opt_is(o - 1, "help") || opt_is(o - 1, "?")) {
      printf("usage: midl [/tlb file] [/h file] [/iid file] [/proxy file] [/dlldata file]\n"
             "            [/out dir] [/I dir] [/D name[=value]] [/env win32|win64]\n"
             "            [/mktyplib203] [/sysroot name|dir] file.idl|file.odl\n");
      return 0;
    }
    /* Options that change nothing here. */
    else if (opt_is(o - 1, "char") || opt_is(o - 1, "cpp_cmd") || opt_is(o - 1, "cpp_opt") ||
             opt_is(o - 1, "error") || opt_is(o - 1, "target") || opt_is(o - 1, "c_ext") ||
             opt_is(o - 1, "ms_ext") || opt_is(o - 1, "align") || opt_is(o - 1, "pack") ||
             opt_is(o - 1, "lcid") || opt_is(o - 1, "prefix") || opt_is(o - 1, "protocol") ||
             opt_is(o - 1, "robust") || opt_is(o - 1, "no_robust") || opt_is(o - 1, "nologo") ||
             opt_is(o - 1, "app_config") || opt_is(o - 1, "no_warn") || opt_is(o - 1, "WX") ||
             opt_is(o - 1, "oldnames") || opt_is(o - 1, "no_format_opt") ||
             opt_is(o - 1, "no_default_epv") || opt_is(o - 1, "osf") || opt_is(o - 1, "nocpp") ||
             opt_is(o - 1, "confirm") || opt_is(o - 1, "no_def_idir") || o[0] == 'W') {
      /* These few take an argument. */
      if (opt_is(o - 1, "char") || opt_is(o - 1, "cpp_cmd") || opt_is(o - 1, "cpp_opt") ||
          opt_is(o - 1, "error") || opt_is(o - 1, "target") || opt_is(o - 1, "lcid") ||
          opt_is(o - 1, "prefix") || opt_is(o - 1, "protocol") || opt_is(o - 1, "align") ||
          opt_is(o - 1, "pack"))
        if (i + 1 < argc) i++;
      if (opt_is(o - 1, "oldnames")) add("--oldnames");
    } else {
      fprintf(stderr, "midl: warning: ignoring unknown option %s\n", a);
    }
  }
  if (!input) {
    fprintf(stderr, "midl: no input file (midl /help for usage)\n");
    return 2;
  }

  /* The sysroot's IDL files and type libraries. */
  if (!sysroot || !*sysroot) sysroot = "win2k-xp";
  char* sr = (is_abs(sysroot) || strchr(sysroot, PATHSEP) || strchr(sysroot, '/'))
                 ? strdup(sysroot)
                 : cat3(dir, "/../sysroots/", sysroot);
  add(cat3("-I", sr, "/include"));
  add(cat3("-L", sr, "/lib"));
  add("-I.");
  add("-L.");

  /* The base name of the input, for the default output names. */
  const char* base = input;
  for (const char* p = input; *p; p++)
    if (*p == '/' || *p == '\\') base = p + 1;
  char* stem = strdup(base);
  char* dot = strrchr(stem, '.');
  if (dot && (!strcmp(dot, ".odl") || !strcmp(dot, ".ODL") || !strcmp(dot, ".Odl"))) mktyplib = 1;
  if (dot) *dot = 0;

  add(win64 ? "--win64" : "--win32");

  /* MIDL's defaults: a header, interface identifiers, a proxy and its
     dlldata, and a type library when the file has a library. Of those,
     midl writes the header and the type library unless asked for others;
     /mktyplib203 sources get only the type library and their header. */
  int any = tlb || header || iid || proxy || dlldata || client || server;
  if (!any) {
    header = cat3(stem, ".h", "");
    if (!mktyplib) iid = cat3(stem, "_i.c", "");
  }
  if (!tlb && !notlb) tlb = cat3(stem, ".tlb", "");
  (void)no_client;
  (void)no_server;

  /* A MKTYPLIB source, compiled with oaidl.idl imported first. */
  const char* source = input;
  char* tmp = NULL;
  if (mktyplib) {
    char cwd[4096];
#ifdef _WIN32
    if (!GetCurrentDirectoryA(sizeof cwd, cwd)) strcpy(cwd, ".");
#else
    if (!getcwd(cwd, sizeof cwd)) strcpy(cwd, ".");
#endif
    char* abspath = is_abs(input) ? strdup(input) : cat3(cwd, "/", input);
    for (char* p = abspath; *p; p++)
      if (*p == '\\') *p = '/';
    tmp = cat3(outdir ? outdir : ".", "/", cat3(stem, ".midl-odl.idl", ""));
    FILE* f = fopen(tmp, "w");
    if (!f) {
      fprintf(stderr, "midl: cannot write %s: %s\n", tmp, strerror(errno));
      return 2;
    }
    /* widl looks for an include in the -I directories, so the source is
       named by its file name and its directory is added below. */
    const char* fname = strrchr(abspath, '/') ? strrchr(abspath, '/') + 1 : abspath;
    fprintf(f, "import \"oaidl.idl\";\nimport \"ocidl.idl\";\n#include \"%s\"\n", fname);
    fclose(f);
    source = tmp;
    /* The input's own directory, for its #includes and importlibs. */
    char* d = strdup(abspath);
    char* s = strrchr(d, '/');
    if (s) {
      *s = 0;
      add(cat3("-I", d, ""));
      add(cat3("-L", d, ""));
    }
  }

  const char* widl = getenv("WIDL");
  if (!widl || !*widl) widl = cat3(dir, "/widl", EXE);

  struct {
    const char* file;
    const char* flag;
  } outs[] = {{tlb, "-t"}, {header, "-h"}, {iid, "-u"}, {proxy, "-p"},
              {client, "-c"}, {server, "-s"}};
  int rc = 0;
  for (size_t k = 0; k < sizeof outs / sizeof outs[0] && rc == 0; k++) {
    if (!outs[k].file) continue;
    const char* av[MAXARGS + 8];
    int n = 0;
    av[n++] = widl;
    for (int j = 0; j < g_nargs; j++) av[n++] = g_args[j];
    av[n++] = outs[k].flag;
    av[n++] = "-o";
    av[n++] = (outdir && !is_abs(outs[k].file)) ? cat3(outdir, "/", outs[k].file) : outs[k].file;
    av[n++] = source;
    av[n] = NULL;
    rc = run(av);
    /* A file without a library has no type library to write: that is
       not an error unless one was asked for. */
  }
  /* dlldata.c names the proxy by its file name without "_p.c", as
     widl's proxy does. */
  if (rc == 0 && dlldata && proxy) {
    const char* p = strrchr(proxy, '/');
    const char* q = strrchr(proxy, '\\');
    if (q && (!p || q > p)) p = q;
    char* token = strdup(p ? p + 1 : proxy);
    size_t len = strlen(token);
    if (len > 4 && strcmp(token + len - 4, "_p.c") == 0)
      token[len - 4] = 0;
    else if (len > 2 && strcmp(token + len - 2, ".c") == 0)
      token[len - 2] = 0;
    const char* av[5];
    av[0] = widl;
    av[1] = "--dlldata-only";
    av[2] = cat3("--dlldata=", (outdir && !is_abs(dlldata)) ? cat3(outdir, "/", dlldata) : dlldata, "");
    av[3] = token;
    av[4] = NULL;
    rc = run(av);
  }
  if (tmp) remove(tmp);
  return rc;
}
