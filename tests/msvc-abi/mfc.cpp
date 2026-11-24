/* A program that uses mfc42.dll, the MFC of Visual C++ 6.0, built with
 * clang. It prints one line for each check and returns the number of
 * checks that failed. */
#include <afx.h>
#include <afxwin.h>
#include <afxcoll.h>
#include <afxtempl.h>
#include <stdio.h>

static int failures = 0;

/* Prints a check and counts it if it failed. */
static void check(const char *what, BOOL ok) {
  printf("%s %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok)
    failures++;
}

/* A class that MFC makes, serializes and identifies at run time. */
class CPoint3 : public CObject {
  DECLARE_SERIAL(CPoint3)
public:
  /* Makes a point at the origin. */
  CPoint3() : x(0), y(0), z(0) {}
  /* Makes a point. */
  CPoint3(int a, int b, int c) : x(a), y(b), z(c) {}
  /* Writes or reads the point. */
  virtual void Serialize(CArchive &ar) {
    CObject::Serialize(ar);
    if (ar.IsStoring())
      ar << (LONG)x << (LONG)y << (LONG)z;
    else {
      LONG a, b, c;
      ar >> a >> b >> c;
      x = a; y = b; z = c;
    }
  }
  int x, y, z;
};
IMPLEMENT_SERIAL(CPoint3, CObject, 1)

/* An application object, as each MFC program has. */
class CAbiApp : public CWinApp {
public:
  /* Makes the application object. */
  CAbiApp() : CWinApp("mfcabi") {}
};
CAbiApp theApp;

/* Throws a CFileException from MFC, for TRY and CATCH. */
static void open_missing() {
  CFile f;
  CFileException *e = new CFileException;
  if (!f.Open("Z:\\no\\such\\file.txt", CFile::modeRead, e))
    THROW(e);
  e->Delete();
}

/* Runs every check. */
int main() {
  if (!AfxWinInit(GetModuleHandle(NULL), NULL, GetCommandLine(), 0)) {
    printf("FAIL AfxWinInit\n");
    return 1;
  }
  check("AfxGetApp", AfxGetApp() == &theApp);

  CString s("Hello");
  s += ", MFC";
  CString t;
  t.Format("%s %d", (LPCTSTR)s, 42);
  check("CString", t == "Hello, MFC 42" && t.GetLength() == 13);
  check("CString::Find and Mid", t.Mid(t.Find("MFC"), 3) == "MFC");
  t.MakeUpper();
  check("CString::MakeUpper", t.Left(5) == "HELLO");

  CStringArray arr;
  arr.Add("one");
  arr.Add("two");
  arr.InsertAt(0, "zero");
  check("CStringArray", arr.GetSize() == 3 && arr[2] == "two");

  CMap<int, int, CString, LPCTSTR> map;
  map[1] = "a";
  map[2] = "b";
  CString v;
  check("CMap", map.Lookup(2, v) && v == "b" && map.GetCount() == 2);

  CPoint3 *p = new CPoint3(1, 2, 3);
  check("IsKindOf", p->IsKindOf(RUNTIME_CLASS(CObject)));
  check("GetRuntimeClass", strcmp(p->GetRuntimeClass()->m_lpszClassName,
                                  "CPoint3") == 0);
  CMemFile mem;
  {
    CArchive ar(&mem, CArchive::store);
    ar << p;
  }
  mem.SeekToBegin();
  CObject *q = NULL;
  {
    CArchive ar(&mem, CArchive::load);
    ar >> q;
  }
  CPoint3 *r = DYNAMIC_DOWNCAST(CPoint3, q);
  check("CArchive round trip", r && r->x == 1 && r->y == 2 && r->z == 3);
  delete p;
  delete q;

  int caught = 0;
  TRY {
    open_missing();
  }
  CATCH(CFileException, e) {
    caught = e->m_cause == CFileException::fileNotFound ||
             e->m_cause == CFileException::badPath;
  }
  END_CATCH
  check("TRY and CATCH of a CFileException", caught);

  caught = 0;
  try {
    AfxThrowMemoryException();
  } catch (CException *e) {
    caught = e->IsKindOf(RUNTIME_CLASS(CMemoryException));
    e->Delete();
  }
  check("C++ catch of an MFC exception", caught);

  CRect rc(0, 0, 10, 20);
  rc.InflateRect(1, 1);
  check("CRect", rc.Width() == 12 && rc.Height() == 22);

  printf("%d failed\n", failures);
  return failures;
}
