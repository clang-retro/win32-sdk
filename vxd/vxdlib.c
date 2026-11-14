/* libvxd.a: the C functions that clang may call on its own in a VxD
   (structure copies and the like), which no VxD can import.

   SPDX-License-Identifier: AGPL-3.0-only
   Copyright (C) clang-retro contributors. */
typedef __SIZE_TYPE__ size_t;

void *memcpy(void *d, const void *s, size_t n) {
  unsigned char *p = d;
  const unsigned char *q = s;
  while (n--)
    *p++ = *q++;
  return d;
}

void *memmove(void *d, const void *s, size_t n) {
  unsigned char *p = d;
  const unsigned char *q = s;
  if (p < q)
    while (n--)
      *p++ = *q++;
  else
    while (n--)
      p[n] = q[n];
  return d;
}

void *memset(void *d, int c, size_t n) {
  unsigned char *p = d;
  while (n--)
    *p++ = (unsigned char)c;
  return d;
}

int memcmp(const void *a, const void *b, size_t n) {
  const unsigned char *p = a, *q = b;
  for (; n; n--, p++, q++)
    if (*p != *q)
      return *p - *q;
  return 0;
}

size_t strlen(const char *s) {
  const char *p = s;
  while (*p)
    p++;
  return (size_t)(p - s);
}
