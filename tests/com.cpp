// The compiler COM support of comdef.h: the dispatch calls that #import's
// dispinterface wrappers make (libcomsupp), _com_error from failed calls,
// and the smart pointers of the system interfaces (comdefsp.h). The object
// is an IDispatch implemented here. Prints ALL PASSED.
#include <windows.h>
#include <comdef.h>
#include <cstdio>
#include <cstring>

static int failures;
#define CHECK(c)                                                  \
  do {                                                            \
    if (!(c)) {                                                   \
      std::printf("FAIL line %d: %s\n", __LINE__, #c);            \
      failures++;                                                 \
    }                                                             \
  } while (0)

// Properties 1 (long), 2 (BSTR) and 5 (IDispatch, set by reference);
// methods 3 (long Add(long), fails for a negative number) and 4 (BSTR
// Concat(BSTR, VARIANT*)).
struct Obj : IDispatch {
  LONG ref = 1;
  long accum = 0;
  BSTR name = SysAllocString(L"obj");
  IDispatch* other = nullptr;
  ~Obj() { SysFreeString(name); }
  STDMETHODIMP QueryInterface(REFIID r, void** pp) override {
    if (r == IID_IUnknown || r == IID_IDispatch) {
      *pp = this;
      AddRef();
      return S_OK;
    }
    *pp = nullptr;
    return E_NOINTERFACE;
  }
  STDMETHODIMP_(ULONG) AddRef() override { return ++ref; }
  STDMETHODIMP_(ULONG) Release() override { return --ref; }
  STDMETHODIMP GetTypeInfoCount(UINT* n) override {
    *n = 0;
    return S_OK;
  }
  STDMETHODIMP GetTypeInfo(UINT, LCID, ITypeInfo**) override { return E_NOTIMPL; }
  STDMETHODIMP GetIDsOfNames(REFIID, LPOLESTR*, UINT, LCID, DISPID*) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP Invoke(DISPID id, REFIID, LCID, WORD f, DISPPARAMS* p, VARIANT* r,
                      EXCEPINFO* ei, UINT*) override {
    bool put = (f & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF)) != 0;
    if (put && (p->cArgs != 1 || p->cNamedArgs != 1 ||
                p->rgdispidNamedArgs[0] != DISPID_PROPERTYPUT))
      return DISP_E_BADPARAMCOUNT;
    switch (id) {
    case 1:
      if (put) {
        if (V_VT(&p->rgvarg[0]) != VT_I4) return DISP_E_TYPEMISMATCH;
        accum = V_I4(&p->rgvarg[0]);
        return S_OK;
      }
      V_VT(r) = VT_I4;
      V_I4(r) = accum;
      return S_OK;
    case 2:
      if (put) {
        SysFreeString(name);
        name = SysAllocString(V_BSTR(&p->rgvarg[0]));
        return S_OK;
      }
      V_VT(r) = VT_BSTR;
      V_BSTR(r) = SysAllocString(name);
      return S_OK;
    case 3:
      if (p->cArgs != 1 || V_VT(&p->rgvarg[0]) != VT_I4) return DISP_E_TYPEMISMATCH;
      if (V_I4(&p->rgvarg[0]) < 0) {
        std::memset(ei, 0, sizeof *ei);
        ei->scode = E_INVALIDARG;
        ei->bstrDescription = SysAllocString(L"negative");
        ei->bstrSource = SysAllocString(L"Obj");
        return DISP_E_EXCEPTION;
      }
      accum += V_I4(&p->rgvarg[0]);
      V_VT(r) = VT_I4;
      V_I4(r) = accum;
      return S_OK;
    case 4: {  // the arguments are in reverse order
      if (p->cArgs != 2 || V_VT(&p->rgvarg[1]) != VT_BSTR ||
          V_VT(&p->rgvarg[0]) != (VT_VARIANT | VT_BYREF))
        return DISP_E_TYPEMISMATCH;
      VARIANT s;
      VariantInit(&s);
      if (FAILED(VariantChangeType(&s, V_VARIANTREF(&p->rgvarg[0]), 0, VT_BSTR)))
        return DISP_E_TYPEMISMATCH;
      UINT a = SysStringLen(V_BSTR(&p->rgvarg[1])), b = SysStringLen(V_BSTR(&s));
      BSTR out = SysAllocStringLen(nullptr, a + b);
      std::memcpy(out, V_BSTR(&p->rgvarg[1]), a * sizeof(WCHAR));
      std::memcpy(out + a, V_BSTR(&s), b * sizeof(WCHAR));
      VariantClear(&s);
      V_VT(r) = VT_BSTR;
      V_BSTR(r) = out;
      return S_OK;
    }
    case 5:
      if (put) {
        if (!(f & DISPATCH_PROPERTYPUTREF) || V_VT(&p->rgvarg[0]) != VT_DISPATCH)
          return DISP_E_TYPEMISMATCH;
        other = V_DISPATCH(&p->rgvarg[0]);
        return S_OK;
      }
      V_VT(r) = VT_DISPATCH;
      V_DISPATCH(r) = other;
      if (other) other->AddRef();
      return S_OK;
    }
    return DISP_E_MEMBERNOTFOUND;
  }
};

int main() {
  CoInitialize(nullptr);
  Obj obj;
  IDispatchPtr disp(&obj);  // from comdefsp.h
  IUnknownPtr unk(disp);
  CHECK(unk != nullptr);

  // What #import writes for a dispinterface's properties and methods.
  _com_dispatch_propput(disp, 1, VT_I4, 5L);
  long n = 0;
  _com_dispatch_propget(disp, 1, VT_I4, &n);
  CHECK(n == 5);
  _com_dispatch_method(disp, 3, DISPATCH_METHOD, VT_I4, &n, L"\x0003", 3L);
  CHECK(n == 8);
  _bstr_t s(L"abc");
  _com_dispatch_propput(disp, 2, VT_BSTR, (BSTR)s);
  BSTR got = nullptr;
  _com_dispatch_propget(disp, 2, VT_BSTR, &got);
  CHECK(_bstr_t(got, false) == _bstr_t(L"abc"));
  _variant_t v(42L);
  got = nullptr;
  _com_dispatch_method(disp, 4, DISPATCH_METHOD, VT_BSTR, &got, L"\x0008\x400c",
                       (BSTR)s, &v);
  CHECK(_bstr_t(got, false) == _bstr_t(L"abc42"));
  // An object property is set by reference.
  Obj other;
  _com_dispatch_propput(disp, 5, VT_DISPATCH, (IDispatch*)&other);
  CHECK(obj.other == &other);
  IDispatch* back = nullptr;
  _com_dispatch_propget(disp, 5, VT_DISPATCH, &back);
  CHECK(back == &other);
  if (back) back->Release();

  // A failure is a _com_error with the object's error information.
  bool caught = false;
  try {
    _com_dispatch_method(disp, 3, DISPATCH_METHOD, VT_I4, &n, L"\x0003", -1L);
  } catch (_com_error& e) {
    caught = true;
    CHECK(e.Error() == E_INVALIDARG);
    CHECK(e.Description() == _bstr_t(L"negative"));
    CHECK(e.Source() == _bstr_t(L"Obj"));
  }
  CHECK(caught);
  caught = false;
  try {
    _com_dispatch_propget(disp, 99, VT_I4, &n);
  } catch (_com_error& e) {
    caught = true;
    CHECK(e.Error() == DISP_E_MEMBERNOTFOUND);
  }
  CHECK(caught);
  // The raw form returns the HRESULT instead.
  CHECK(_com_dispatch_raw_method(disp, 3, DISPATCH_METHOD, VT_I4, &n, L"\x0003", -2L) ==
        E_INVALIDARG);
  // _com_issue_errorex takes the error information from the object.
  caught = false;
  try {
    _com_issue_errorex(E_FAIL, disp, IID_IDispatch);
  } catch (_com_error& e) {
    caught = e.Error() == E_FAIL;
  }
  CHECK(caught);

  disp = nullptr;
  unk = nullptr;
  CoUninitialize();
  if (failures == 0) std::printf("ALL PASSED\n");
  return failures != 0;
}
