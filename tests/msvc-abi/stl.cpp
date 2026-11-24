/* The C++ library of Visual C++ 6.0 (msvcp60.dll) with clang: containers,
 * strings, streams, files, locales and its exceptions. It prints one line
 * for each check and returns the number of checks that failed. */
#include <algorithm>
#include <bitset>
#include <complex>
#include <deque>
#include <exception>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <numeric>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <typeinfo>
#include <utility>
#include <valarray>
#include <vector>
using namespace std;

static int failures = 0;

/* Prints a check and counts it if it failed. */
static void check(const char *what, bool ok) {
  printf("%s %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok)
    failures++;
}

/* Runs every check. */
int main() {
  vector<int> v;
  for (int i = 0; i < 10; i++)
    v.push_back(9 - i);
  sort(v.begin(), v.end());
  check("vector and sort", v[0] == 0 && v[9] == 9);
  check("accumulate", accumulate(v.begin(), v.end(), 0) == 45);
  check("find_if and bind2nd",
        *find_if(v.begin(), v.end(), bind2nd(greater<int>(), 6)) == 7);

  list<string> l;
  l.push_back("b");
  l.push_back("a");
  l.sort();
  check("list<string>::sort", l.front() == "a");

  map<string, int> m;
  m["x"] = 1;
  m["y"] = 2;
  check("map<string, int>", m["y"] == 2 && m.size() == 2);
  set<int> s(v.begin(), v.end());
  check("set", s.size() == 10 && s.count(4) == 1);
  deque<double> d;
  d.push_front(1.5);
  d.push_back(2.5);
  check("deque", d.front() == 1.5 && d.back() == 2.5);
  queue<int> q;
  q.push(1);
  stack<int> st;
  st.push(2);
  priority_queue<int> pq;
  pq.push(3);
  pq.push(5);
  check("queue, stack, priority_queue",
        q.front() == 1 && st.top() == 2 && pq.top() == 5);

  ostringstream os;
  os << "n=" << v[3] << ' ' << 2.5 << ' ' << hex << 255 << dec << ' '
     << setw(4) << setfill('0') << 7;
  check("ostringstream", os.str() == "n=3 2.5 ff 0007");
  istringstream is("12 34 word");
  int a = 0, b = 0;
  string word;
  is >> a >> b >> word;
  check("istringstream", a == 12 && b == 34 && word == "word");

  string str = os.str();
  str += "!";
  string t = str.substr(0, 3) + "x";
  check("string operators", t == "n=3x" && str[str.size() - 1] == '!');
  check("string compare", string("abc") < "abd" && "abc" == string("abc"));
  wstring w = L"wide";
  wostringstream wos;
  wos << w << 5;
  check("wstring and wostringstream", wos.str() == L"wide5");

  complex<double> c(1, 2);
  c *= c;
  check("complex", c.real() == -3 && c.imag() == 4);
  bitset<16> bs(0x1234);
  check("bitset", bs.count() == 5);
  valarray<int> va(3, 4);
  check("valarray", va.sum() == 12);
  auto_ptr<int> ap(new int(5));
  check("auto_ptr", *ap == 5);

  int caught = 0;
  try {
    v.at(100);
  } catch (out_of_range &) {
    caught = 1;
  } catch (exception &) {
    caught = 2;
  }
  check("out_of_range from vector::at", caught == 1);
  caught = 0;
  try {
    throw runtime_error("ours");
  } catch (exception &e) {
    caught = string(e.what()) == "ours";
  }
  check("runtime_error", caught == 1);

  {
    ofstream f("stl-test.txt");
    f << "line one" << endl << 42 << endl;
  }
  ifstream g("stl-test.txt");
  string line;
  int n = 0;
  getline(g, line);
  g >> n;
  check("ofstream, ifstream and getline", line == "line one" && n == 42);

  check("numeric_limits", numeric_limits<int>::max() == 2147483647);
  check("typeid of a library class",
        string(typeid(v).name()).find("vector") != string::npos);
  locale loc;
  // Visual C++ 6.0's use_facet takes the facet as a pointer (_USE).
  check("locale and use_facet", _USE(loc, ctype<char>).toupper('q') == 'Q');

  cout << "cout works" << endl;
  printf("%d failed\n", failures);
  return failures;
}
