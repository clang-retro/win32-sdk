/* vmmreg.h: The VMM's registry: REG_* value types and errors, the
   HKEY_* roots and the structures _RegQueryMultipleValues and the
   registry services take.

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_VMMREG_H
#define VXD_VMMREG_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
#define MAX_VMM_REG_KEY_LEN 256
#define ERROR_FILE_NOT_FOUND 2
#define ERROR_BADDB 1009
#define ERROR_MORE_DATA 234
#define ERROR_BADKEY 1010
#define ERROR_CANTOPEN 1011
#define ERROR_CANTREAD 1012
#define ERROR_CANTWRITE 1013
#define ERROR_REGISTRY_CORRUPT 1015
#define ERROR_REGISTRY_IO_FAILED 1016
#define ERROR_KEY_DELETED 1018
#define ERROR_OUTOFMEMORY 14
#define ERROR_INVALID_PARAMETER 87
#define ERROR_LOCK_FAILED 167
#define ERROR_NO_MORE_ITEMS 259
#define ERROR_SUCCESS 0
#define PROVIDER_KEEPS_VALUE_LENGTH 0x01
typedef DWORD VMMHKEY;
typedef VMMHKEY *PVMMHKEY;
typedef DWORD VMMREGRET;
struct val_context {
  int valuelen;
  PVOID value_context;
  PVOID val_buff_ptr;
};
typedef struct val_context *PVALCONTEXT;
struct pvalue {
  PCHAR pv_valuename;
  DWORD pv_valuelen;
  PVOID pv_value_context;
  DWORD pv_type;
};
typedef struct pvalue PVALUE;
typedef struct pvalue *PPVALUE;
typedef VMMREGRET (*PQUERYHANDLER)(PVOID, PVALCONTEXT, DWORD, PVOID, DWORD *, DWORD);
struct provider_info {
  PQUERYHANDLER pi_R0_1val;
  PQUERYHANDLER pi_R0_allvals;
  PQUERYHANDLER pi_R3_1val;
  PQUERYHANDLER pi_R3_allvals;
  DWORD pi_flags;
};
typedef struct provider_info PROVIDER;
typedef PROVIDER *PPROVIDER;
struct value_ent {
  PCHAR ve_valuename;
  DWORD ve_valuelen;
  DWORD ve_valueptr;
  DWORD ve_type;
};
typedef struct value_ent VALENT;
typedef VALENT *PVALENT;

#pragma pack(pop)

#endif
