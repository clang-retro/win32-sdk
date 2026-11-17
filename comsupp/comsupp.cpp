// comsupp.cpp - the compiler COM support library (comsupp.lib and
// comsuppw.lib in Visual C++): the functions comdef.h declares that the
// headers do not define. #import's wrapper methods call these to reach
// dispinterface members through IDispatch::Invoke and to turn failed
// HRESULTs into _com_error exceptions.
//
// _com_dispatch_method's parameter string has one wide character per
// argument, its VARTYPE. Each argument is passed as the C++ type that
// holds a value of that VARTYPE, promoted as variable arguments are
// (float as double, short as int); VARIANT and DECIMAL arguments, and
// VT_BYREF ones, are passed as pointers.
//
// Part of clang-retro win32-sdk.
// SPDX-License-Identifier: AGPL-3.0-only
#include <comdef.h>
#include <stdarg.h>
#include <string.h>
#include <new>

// The property setters' signatures come from comdef.h, with the last
// named parameter a VARTYPE; every argument takes a whole stack slot, so
// va_start after it finds the next one.
#pragma clang diagnostic ignored "-Wvarargs"

namespace {

// GUID_NULL, here so that the library needs no libuuid.
const GUID kNullGuid = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};

// Puts one variable argument of type vt into a VARIANTARG.
bool TakeArg(VARIANTARG* pv, VARTYPE vt, va_list* pap) {
  VariantInit(pv);
  if (vt & VT_BYREF) {
    pv->vt = vt;
    pv->byref = va_arg(*pap, void*);
    return true;
  }
  if (vt & VT_ARRAY) {
    pv->vt = vt;
    pv->parray = va_arg(*pap, SAFEARRAY*);
    return true;
  }
  switch (vt) {
  case VT_I1: pv->cVal = (CHAR)va_arg(*pap, int); break;
  case VT_UI1: pv->bVal = (BYTE)va_arg(*pap, int); break;
  case VT_I2: pv->iVal = (SHORT)va_arg(*pap, int); break;
  case VT_UI2: pv->uiVal = (USHORT)va_arg(*pap, int); break;
  case VT_BOOL: pv->boolVal = (VARIANT_BOOL)va_arg(*pap, int); break;
  case VT_I4: pv->lVal = va_arg(*pap, LONG); break;
  case VT_UI4: pv->ulVal = va_arg(*pap, ULONG); break;
  case VT_INT: pv->intVal = va_arg(*pap, INT); break;
  case VT_UINT: pv->uintVal = va_arg(*pap, UINT); break;
  case VT_ERROR: pv->scode = va_arg(*pap, SCODE); break;
  case VT_HRESULT: vt = VT_ERROR; pv->scode = va_arg(*pap, HRESULT); break;
  case VT_I8: pv->llVal = va_arg(*pap, LONGLONG); break;
  case VT_UI8: pv->ullVal = va_arg(*pap, ULONGLONG); break;
  case VT_R4: pv->fltVal = (FLOAT)va_arg(*pap, double); break;
  case VT_R8: pv->dblVal = va_arg(*pap, double); break;
  case VT_DATE: pv->date = va_arg(*pap, DATE); break;
  case VT_CY: pv->cyVal = va_arg(*pap, CY); break;
  case VT_BSTR: pv->bstrVal = va_arg(*pap, BSTR); break;
  case VT_DISPATCH: pv->pdispVal = va_arg(*pap, IDispatch*); break;
  case VT_UNKNOWN: pv->punkVal = va_arg(*pap, IUnknown*); break;
  case VT_VARIANT: {
    // The VARIANT itself, not a copy of what it holds: Invoke only reads
    // its arguments.
    const VARIANT* pSrc = va_arg(*pap, const VARIANT*);
    if (pSrc != NULL) *pv = *pSrc;
    return true;
  }
  case VT_DECIMAL: {
    const DECIMAL* pSrc = va_arg(*pap, const DECIMAL*);
    if (pSrc != NULL) pv->decVal = *pSrc;
    break;
  }
  default: return false;
  }
  pv->vt = vt;
  return true;
}

// Stores Invoke's result as the caller's variable of type vtRet.
HRESULT StoreResult(VARIANT* pResult, VARTYPE vtRet, void* pvRet) {
  if (vtRet == VT_EMPTY || pvRet == NULL) return S_OK;
  if (vtRet == VT_VARIANT) {
    // The caller's VARIANT is empty: it takes the result as it is.
    *(VARIANT*)pvRet = *pResult;
    VariantInit(pResult);
    return S_OK;
  }
  VARTYPE vt = vtRet == VT_HRESULT ? VT_ERROR : vtRet;
  if (pResult->vt != vt) {
    HRESULT hr = VariantChangeType(pResult, pResult, 0, vt);
    if (FAILED(hr)) return hr;
  }
  if (vt & (VT_BYREF | VT_ARRAY)) {
    *(void**)pvRet = pResult->byref;
    VariantInit(pResult);  // the caller owns an array now
    return S_OK;
  }
  switch (vt) {
  case VT_I1: case VT_UI1: *(BYTE*)pvRet = pResult->bVal; break;
  case VT_I2: case VT_UI2: case VT_BOOL: *(SHORT*)pvRet = pResult->iVal; break;
  case VT_I4: case VT_UI4: case VT_INT: case VT_UINT: case VT_ERROR:
    *(LONG*)pvRet = pResult->lVal;
    break;
  case VT_I8: case VT_UI8: *(LONGLONG*)pvRet = pResult->llVal; break;
  case VT_R4: *(FLOAT*)pvRet = pResult->fltVal; break;
  case VT_R8: *(DOUBLE*)pvRet = pResult->dblVal; break;
  case VT_DATE: *(DATE*)pvRet = pResult->date; break;
  case VT_CY: *(CY*)pvRet = pResult->cyVal; break;
  case VT_DECIMAL: *(DECIMAL*)pvRet = pResult->decVal; break;
  case VT_BSTR: case VT_DISPATCH: case VT_UNKNOWN:
    // The caller takes the reference.
    *(void**)pvRet = pResult->byref;
    VariantInit(pResult);
    break;
  default: return DISP_E_BADVARTYPE;
  }
  return S_OK;
}

// An error object made from Invoke's exception information.
IErrorInfo* ErrorInfoFromExcepInfo(EXCEPINFO* pei) {
  if (pei->pfnDeferredFillIn != NULL) pei->pfnDeferredFillIn(pei);
  ICreateErrorInfo* pCreate = NULL;
  IErrorInfo* pInfo = NULL;
  if (SUCCEEDED(CreateErrorInfo(&pCreate))) {
    pCreate->SetGUID(kNullGuid);
    if (pei->bstrSource != NULL) pCreate->SetSource(pei->bstrSource);
    if (pei->bstrDescription != NULL) pCreate->SetDescription(pei->bstrDescription);
    if (pei->bstrHelpFile != NULL) pCreate->SetHelpFile(pei->bstrHelpFile);
    pCreate->SetHelpContext(pei->dwHelpContext);
    if (FAILED(pCreate->QueryInterface(__uuidof(IErrorInfo), (void**)&pInfo))) pInfo = NULL;
    pCreate->Release();
  }
  return pInfo;
}

// Calls IDispatch::Invoke with the arguments described by pwParamInfo.
// On failure *ppInfo may receive an error object.
HRESULT InvokeV(IDispatch* pDisp, DISPID dispid, WORD wFlags, VARTYPE vtRet, void* pvRet,
                const wchar_t* pwParamInfo, va_list ap, IErrorInfo** ppInfo) {
  *ppInfo = NULL;
  if (pDisp == NULL) return E_POINTER;
  UINT cArgs = pwParamInfo != NULL ? (UINT)wcslen(pwParamInfo) : 0;
  VARIANTARG argsSmall[8];
  VARIANTARG* pArgs = cArgs <= 8 ? argsSmall : new (std::nothrow) VARIANTARG[cArgs];
  if (pArgs == NULL) return E_OUTOFMEMORY;
  va_list aq;
  va_copy(aq, ap);
  HRESULT hr = S_OK;
  // Invoke takes the arguments last first.
  for (UINT i = 0; i < cArgs; i++) {
    if (!TakeArg(&pArgs[cArgs - 1 - i], (VARTYPE)pwParamInfo[i], &aq)) {
      hr = DISP_E_BADVARTYPE;
      break;
    }
  }
  va_end(aq);
  if (SUCCEEDED(hr)) {
    DISPID dispidNamed = DISPID_PROPERTYPUT;
    DISPPARAMS params = {pArgs, NULL, cArgs, 0};
    if (wFlags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF)) {
      params.rgdispidNamedArgs = &dispidNamed;
      params.cNamedArgs = 1;
    }
    VARIANT result;
    VariantInit(&result);
    EXCEPINFO excep;
    memset(&excep, 0, sizeof(excep));
    UINT uArgErr = (UINT)-1;
    hr = pDisp->Invoke(dispid, kNullGuid, LOCALE_USER_DEFAULT, wFlags, &params,
                       vtRet != VT_EMPTY ? &result : NULL, &excep, &uArgErr);
    if (hr == DISP_E_EXCEPTION) {
      if (excep.pfnDeferredFillIn != NULL) {
        excep.pfnDeferredFillIn(&excep);
        excep.pfnDeferredFillIn = NULL;
      }
      hr = excep.scode != 0 ? excep.scode : _com_error::WCodeToHRESULT(excep.wCode);
      *ppInfo = ErrorInfoFromExcepInfo(&excep);
      SysFreeString(excep.bstrSource);
      SysFreeString(excep.bstrDescription);
      SysFreeString(excep.bstrHelpFile);
    }
    if (SUCCEEDED(hr)) hr = StoreResult(&result, vtRet, pvRet);
    VariantClear(&result);
  }
  if (pArgs != argsSmall) delete[] pArgs;
  return hr;
}

// A property is put by reference when it is an object.
WORD PutFlags(VARTYPE vt) {
  return (vt == VT_DISPATCH || vt == VT_UNKNOWN) ? DISPATCH_PROPERTYPUTREF : DISPATCH_PROPERTYPUT;
}

HRESULT Raise(HRESULT hr, IErrorInfo* pInfo) {
  if (FAILED(hr)) __mingw_com_error_handler(hr, pInfo);  // takes pInfo
  else if (pInfo != NULL) pInfo->Release();
  return hr;
}

}  // namespace

void WINAPI _com_issue_errorex(HRESULT hr, IUnknown* punk, REFIID riid) {
  IErrorInfo* pInfo = NULL;
  ISupportErrorInfo* pSupport = NULL;
  if (punk != NULL &&
      SUCCEEDED(punk->QueryInterface(__uuidof(ISupportErrorInfo), (void**)&pSupport))) {
    if (pSupport->InterfaceSupportsErrorInfo(riid) == S_OK && GetErrorInfo(0, &pInfo) != S_OK)
      pInfo = NULL;
    pSupport->Release();
  }
  __mingw_com_error_handler(hr, pInfo);
}

HRESULT __cdecl _com_dispatch_raw_method(IDispatch* pDisp, DISPID dispid, WORD wFlags,
                                         VARTYPE vtRet, void* pvRet, const wchar_t* pwParamInfo,
                                         ...) throw() {
  va_list ap;
  va_start(ap, pwParamInfo);
  IErrorInfo* pInfo = NULL;
  HRESULT hr = InvokeV(pDisp, dispid, wFlags, vtRet, pvRet, pwParamInfo, ap, &pInfo);
  va_end(ap);
  // The raw forms leave the error object where GetErrorInfo finds it.
  if (pInfo != NULL) {
    SetErrorInfo(0, pInfo);
    pInfo->Release();
  }
  return hr;
}

HRESULT __cdecl _com_dispatch_method(IDispatch* pDisp, DISPID dispid, WORD wFlags, VARTYPE vtRet,
                                     void* pvRet, const wchar_t* pwParamInfo, ...) {
  va_list ap;
  va_start(ap, pwParamInfo);
  IErrorInfo* pInfo = NULL;
  HRESULT hr = InvokeV(pDisp, dispid, wFlags, vtRet, pvRet, pwParamInfo, ap, &pInfo);
  va_end(ap);
  return Raise(hr, pInfo);
}

HRESULT WINAPI _com_dispatch_raw_propget(IDispatch* pDisp, DISPID dispid, VARTYPE vtProp,
                                         void* pvProp) throw() {
  return _com_dispatch_raw_method(pDisp, dispid, DISPATCH_PROPERTYGET, vtProp, pvProp, NULL);
}

HRESULT WINAPI _com_dispatch_propget(IDispatch* pDisp, DISPID dispid, VARTYPE vtProp,
                                     void* pvProp) {
  return _com_dispatch_method(pDisp, dispid, DISPATCH_PROPERTYGET, vtProp, pvProp, NULL);
}

namespace {

HRESULT PropPutV(IDispatch* pDisp, DISPID dispid, VARTYPE vtProp, va_list ap,
                 IErrorInfo** ppInfo) {
  wchar_t info[2] = {(wchar_t)vtProp, 0};
  return InvokeV(pDisp, dispid, PutFlags(vtProp), VT_EMPTY, NULL, info, ap, ppInfo);
}
}  // namespace

HRESULT __cdecl _com_dispatch_raw_propput(IDispatch* pDisp, DISPID dispid, VARTYPE vtProp,
                                          ...) throw() {
  va_list ap;
  va_start(ap, vtProp);
  IErrorInfo* pInfo = NULL;
  HRESULT hr = PropPutV(pDisp, dispid, vtProp, ap, &pInfo);
  va_end(ap);
  if (pInfo != NULL) {
    SetErrorInfo(0, pInfo);
    pInfo->Release();
  }
  return hr;
}

HRESULT __cdecl _com_dispatch_propput(IDispatch* pDisp, DISPID dispid, VARTYPE vtProp, ...) {
  va_list ap;
  va_start(ap, vtProp);
  IErrorInfo* pInfo = NULL;
  HRESULT hr = PropPutV(pDisp, dispid, vtProp, ap, &pInfo);
  va_end(ap);
  return Raise(hr, pInfo);
}
