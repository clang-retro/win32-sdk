/* tls9x.c: crtbegin.o of the mingw-w64 sysroots for Windows before Vista.
 * Written for clang-retro; see the win32-sdk LICENSE.
 *
 * Windows 95, 98 and ME do not run the TLS callbacks of a program. The
 * mingw-w64 runtime sets up its thread_local destructor list and its
 * TlsAlloc key destructors in its callbacks for DLL_PROCESS_ATTACH, so on
 * 9x __cxa_thread_atexit failed and no thread_local destructor ran. clang
 * links crtbegin.o into each program and DLL, and this one runs the
 * program's callbacks for DLL_PROCESS_ATTACH on 9x, before the C and C++
 * initializers. A DLL needs nothing: the runtime's DllMainCRTStartup runs
 * its callbacks on 9x as well.
 *
 * When a thread ends, the libc++ of clang-retro runs the callbacks for
 * DLL_THREAD_DETACH on 9x, for the threads that std::thread starts. */
#include <windows.h>

extern IMAGE_DOS_HEADER __ImageBase;

/* The TLS directory of a module, from its PE header, or NULL. The header
 * gives it, and not the symbol _tls_used: a reference to that symbol
 * would bring a TLS directory into each program, also into the programs
 * that have no TLS. */
static const IMAGE_TLS_DIRECTORY *tls_directory(HMODULE module)
{
  const BYTE *base = (const BYTE *)module;
  const IMAGE_NT_HEADERS *nt =
      (const IMAGE_NT_HEADERS *)(base + ((const IMAGE_DOS_HEADER *)base)->e_lfanew);
  const IMAGE_DATA_DIRECTORY *dir =
      &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];
  if (nt->OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_TLS ||
      !dir->VirtualAddress)
    return NULL;
  return (const IMAGE_TLS_DIRECTORY *)(base + dir->VirtualAddress);
}

/* Runs the TLS callbacks of the program for DLL_PROCESS_ATTACH, on
 * Windows 9x only (Windows NT runs them itself), and in a program only. */
static void __cdecl tls9x_process_attach(void)
{
  if (!(GetVersion() & 0x80000000))
    return;
  HMODULE module = (HMODULE)&__ImageBase;
  if (module != GetModuleHandleA(NULL))
    return;
  const IMAGE_TLS_DIRECTORY *tls = tls_directory(module);
  if (!tls)
    return;
  PIMAGE_TLS_CALLBACK *cb = (PIMAGE_TLS_CALLBACK *)tls->AddressOfCallBacks;
  for (; cb && *cb; cb++)
    (*cb)(module, DLL_PROCESS_ATTACH, NULL);
}

/* The runtime's _initterm runs the pointers from __xc_a (.CRT$XCA) to
 * __xc_z (.CRT$XCZ) in the order of their section names: this one runs
 * after the runtime's own (.CRT$XCAA) and before the C++ constructors. */
__attribute__((section(".CRT$XCAB"), used))
static void (__cdecl *const tls9x_init)(void) = tls9x_process_attach;
