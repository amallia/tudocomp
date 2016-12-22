/*
 * This file integrates customized parts of divsufsort into tudocomp.
 * divsufsort is licensed under the MIT License, which follows.
 *
 * Copyright (c) 2003-2008 Yuta Mori All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <tudocomp/util/divsufsort_def.hpp>
#include <tudocomp/util/divsufsort_private.hpp>

namespace tdc {
namespace libdivsufsort {

// all below is adapted from ssort.c

#if SS_BLOCKSIZE != 0

const saint_t sqq_table[256] = {
  0,  16,  22,  27,  32,  35,  39,  42,  45,  48,  50,  53,  55,  57,  59,  61,
 64,  65,  67,  69,  71,  73,  75,  76,  78,  80,  81,  83,  84,  86,  87,  89,
 90,  91,  93,  94,  96,  97,  98,  99, 101, 102, 103, 104, 106, 107, 108, 109,
110, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126,
128, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142,
143, 144, 144, 145, 146, 147, 148, 149, 150, 150, 151, 152, 153, 154, 155, 155,
156, 157, 158, 159, 160, 160, 161, 162, 163, 163, 164, 165, 166, 167, 167, 168,
169, 170, 170, 171, 172, 173, 173, 174, 175, 176, 176, 177, 178, 178, 179, 180,
181, 181, 182, 183, 183, 184, 185, 185, 186, 187, 187, 188, 189, 189, 190, 191,
192, 192, 193, 193, 194, 195, 195, 196, 197, 197, 198, 199, 199, 200, 201, 201,
202, 203, 203, 204, 204, 205, 206, 206, 207, 208, 208, 209, 209, 210, 211, 211,
212, 212, 213, 214, 214, 215, 215, 216, 217, 217, 218, 218, 219, 219, 220, 221,
221, 222, 222, 223, 224, 224, 225, 225, 226, 226, 227, 227, 228, 229, 229, 230,
230, 231, 231, 232, 232, 233, 234, 234, 235, 235, 236, 236, 237, 237, 238, 238,
239, 240, 240, 241, 241, 242, 242, 243, 243, 244, 244, 245, 245, 246, 246, 247,
247, 248, 248, 249, 249, 250, 250, 251, 251, 252, 252, 253, 253, 254, 254, 255
};

inline
saidx_t
ss_isqrt(saidx_t x) {
  saidx_t y, e;

  if(x >= (SS_BLOCKSIZE * SS_BLOCKSIZE)) { return SS_BLOCKSIZE; }
  e = (x & 0xffff0000) ?
        ((x & 0xff000000) ?
          24 + lg_table[(x >> 24) & 0xff] :
          16 + lg_table[(x >> 16) & 0xff]) :
        ((x & 0x0000ff00) ?
           8 + lg_table[(x >>  8) & 0xff] :
           0 + lg_table[(x >>  0) & 0xff]);

  if(e >= 16) {
    y = sqq_table[x >> ((e - 6) - (e & 1))] << ((e >> 1) - 7);
    if(e >= 24) { y = (y + 1 + x / y) >> 1; }
    y = (y + 1 + x / y) >> 1;
  } else if(e >= 8) {
    y = (sqq_table[x >> ((e - 6) - (e & 1))] >> (7 - (e >> 1))) + 1;
  } else {
    return sqq_table[x] >> 4;
  }

  return (x < (y * y)) ? y - 1 : y;
}

#endif /* SS_BLOCKSIZE != 0 */


/*---------------------------------------------------------------------------*/

/* Compares two suffixes. */
template<typename buffer_t1, typename buffer_t2>
inline saint_t ss_compare(const sauchar_t *T,
           buffer_t1& B1, buffer_t2& B2, const saidx_t p1, const saidx_t p2,
           saidx_t depth) {
  const sauchar_t *U1, *U2, *U1n, *U2n;

  for(U1 = T + depth + B1[p1],
      U2 = T + depth + B2[p2],
      U1n = T + B1[p1 + 1] + 2,
      U2n = T + B2[p2 + 1] + 2;
      (U1 < U1n) && (U2 < U2n) && (*U1 == *U2);
      ++U1, ++U2) {
  }

  return U1 < U1n ?
        (U2 < U2n ? *U1 - *U2 : 1) :
        (U2 < U2n ? -1 : 0);
}

template<typename buffer_t>
inline saint_t ss_compare(const sauchar_t *T,
           buffer_t& B, const saidx_t p1, const saidx_t p2,
           saidx_t depth) {
    return ss_compare(T, B, B, p1, p2, depth);
}

/*---------------------------------------------------------------------------*/

#if (SS_BLOCKSIZE != 1) && (SS_INSERTIONSORT_THRESHOLD != 1)

/* Insertionsort for small size groups */
template<typename buffer_t>
inline void ss_insertionsort(const sauchar_t *T, buffer_t& B, const saidx_t PA,
                 saidx_t first, saidx_t last, saidx_t depth) {
  saidx_t i, j;
  saidx_t t;
  saint_t r;

  for(i = last - 2; first <= i; --i) {
    for(t = B[i], j = i + 1; 0 < (r = ss_compare(T, B, PA + t, PA + B[j], depth));) {
      do { B[j - 1] = B[j]; } while((++j < last) && (B[j] < 0));
      if(last <= j) { break; }
    }
    if(r == 0) { B[j] = ~B[j]; }
    B[j - 1] = t;
  }
}

#endif /* (SS_BLOCKSIZE != 1) && (SS_INSERTIONSORT_THRESHOLD != 1) */


/*---------------------------------------------------------------------------*/

#if (SS_BLOCKSIZE == 0) || (SS_INSERTIONSORT_THRESHOLD < SS_BLOCKSIZE)

template<typename buffer_t>
inline void ss_fixdown(const sauchar_t *Td, buffer_t& B, const saidx_t PA,
           saidx_t SA, saidx_t i, saidx_t size) {
  saidx_t j, k;
  saidx_t v;
  saint_t c, d, e;

  for(v = B[SA + i], c = Td[B[PA + v]]; (j = 2 * i + 1) < size; B[SA + i] = B[SA + k], i = k) {
    k = j++;
    d = Td[B[PA + B[SA + k]]];
    if(d < (e = Td[B[PA + B[SA + j]]])) { k = j; d = e; }
    if(d <= c) { break; }
  }
  B[SA + i] = v;
}

/* Simple top-down heapsort. */
template<typename buffer_t>
inline void ss_heapsort(const sauchar_t *Td, buffer_t& B, const saidx_t PA, saidx_t SA, saidx_t size) {
  saidx_t i, m;
  saidx_t t;

  m = size;
  if((size % 2) == 0) {
    m--;
    if(Td[B[PA + B[SA + m / 2]]] < Td[B[PA + B[SA + m]]]) { SWAP(B[SA + m], B[SA + m / 2]); }
  }

  for(i = m / 2 - 1; 0 <= i; --i) { ss_fixdown(Td, B, PA, SA, i, m); }
  if((size % 2) == 0) { SWAP(B[SA + 0], B[SA + m]); ss_fixdown(Td, B, PA, SA, 0, m); }
  for(i = m - 1; 0 < i; --i) {
    t = B[SA + 0], B[SA + 0] = B[SA + i];
    ss_fixdown(Td, B, PA, SA, 0, i);
    B[SA + i] = t;
  }
}


/*---------------------------------------------------------------------------*/

/* Returns the median of three elements. */
template<typename buffer_t>
inline saidx_t ss_median3(const sauchar_t *Td, buffer_t& B, const saidx_t PA,
           saidx_t v1, saidx_t v2, saidx_t v3) {
  saidx_t t;
  if(Td[B[PA + B[v1]]] > Td[B[PA + B[v2]]]) { SWAP(v1, v2); }
  if(Td[B[PA + B[v2]]] > Td[B[PA + B[v3]]]) {
    if(Td[B[PA + B[v1]]] > Td[B[PA + B[v3]]]) { return v1; }
    else { return v3; }
  }
  return v2;
}

/* Returns the median of five elements. */
template<typename buffer_t>
inline saidx_t ss_median5(const sauchar_t *Td, buffer_t& B, const saidx_t PA,
           saidx_t v1, saidx_t v2, saidx_t v3, saidx_t v4, saidx_t v5) {
  saidx_t t;
  if(Td[B[PA + B[v2]]] > Td[B[PA + B[v3]]]) { SWAP(v2, v3); }
  if(Td[B[PA + B[v4]]] > Td[B[PA + B[v5]]]) { SWAP(v4, v5); }
  if(Td[B[PA + B[v2]]] > Td[B[PA + B[v4]]]) { SWAP(v2, v4); SWAP(v3, v5); }
  if(Td[B[PA + B[v1]]] > Td[B[PA + B[v3]]]) { SWAP(v1, v3); }
  if(Td[B[PA + B[v1]]] > Td[B[PA + B[v4]]]) { SWAP(v1, v4); SWAP(v3, v5); }
  if(Td[B[PA + B[v3]]] > Td[B[PA + B[v4]]]) { return v4; }
  return v3;
}

/* Returns the pivot element. */
template<typename buffer_t>
inline saidx_t ss_pivot(const sauchar_t *Td, buffer_t& B, const saidx_t PA, saidx_t first, saidx_t last) {
  saidx_t middle;
  saidx_t t;

  t = last - first;
  middle = first + t / 2;

  if(t <= 512) {
    if(t <= 32) {
      return ss_median3(Td, B, PA, first, middle, last - 1);
    } else {
      t >>= 2;
      return ss_median5(Td, B, PA, first, first + t, middle, last - 1 - t, last - 1);
    }
  }
  t >>= 3;
  first  = ss_median3(Td, B, PA, first, first + t, first + (t << 1));
  middle = ss_median3(Td, B, PA, middle - t, middle, middle + t);
  last   = ss_median3(Td, B, PA, last - 1 - (t << 1), last - 1 - t, last - 1);
  return ss_median3(Td, B, PA, first, middle, last);
}


/*---------------------------------------------------------------------------*/

/* Binary partition for substrings. */
template<typename buffer_t>
inline saidx_t ss_partition(buffer_t& B, const saidx_t PA,
                    saidx_t first, saidx_t last, saidx_t depth) {
  saidx_t a, b;
  saidx_t t;
  for(a = first - 1, b = last;;) {
    for(; (++a < b) && ((B[PA + B[a]] + depth) >= (B[PA + B[a] + 1] + 1));) { B[a] = ~B[a]; }
    for(; (a < --b) && ((B[PA + B[b]] + depth) <  (B[PA + B[b] + 1] + 1));) { }
    if(b <= a) { break; }
    t = ~B[b];
    B[b] = B[a];
    B[a] = t;
  }
  if(first < a) { B[first] = ~B[first]; }
  return a;
}

/* Multikey introsort for medium size groups. */
template<typename buffer_t>
inline void ss_mintrosort(const sauchar_t *T, buffer_t& B, const saidx_t PA,
              saidx_t first, saidx_t last,
              saidx_t depth) {
#define STACK_SIZE SS_MISORT_STACKSIZE
  struct { saidx_t a, b, c; saint_t d; } stack[STACK_SIZE];
  const sauchar_t *Td;
  saidx_t a, b, c, d, e, f;
  saidx_t s, t;
  saint_t ssize;
  saint_t limit;
  saint_t v, x = 0;

  for(ssize = 0, limit = ilg<saidx_t>(last - first);;) {

    if((last - first) <= SS_INSERTIONSORT_THRESHOLD) {
#if 1 < SS_INSERTIONSORT_THRESHOLD
      if(1 < (last - first)) { ss_insertionsort(T, B, PA, first, last, depth); }
#endif
      STACK_POP(first, last, depth, limit);
      continue;
    }

    Td = T + depth;
    if(limit-- == 0) {
        ss_heapsort(Td, B, PA, first, last - first);
    }
    if(limit < 0) {
      for(a = first + 1, v = Td[B[PA + B[first]]]; a < last; ++a) {
        if((x = Td[B[PA + B[a]]]) != v) {
          if(1 < (a - first)) { break; }
          v = x;
          first = a;
        }
      }
      if(Td[B[PA + B[first]] - 1] < v) {
        first = ss_partition(B, PA, first, a, depth);
      }
      if((a - first) <= (last - a)) {
        if(1 < (a - first)) {
          STACK_PUSH(a, last, depth, -1);
          last = a, depth += 1, limit = ilg<saidx_t>(a - first);
        } else {
          first = a, limit = -1;
        }
      } else {
        if(1 < (last - a)) {
          STACK_PUSH(first, a, depth + 1, ilg<saidx_t>(a - first));
          first = a, limit = -1;
        } else {
          last = a, depth += 1, limit = ilg<saidx_t>(a - first);
        }
      }
      continue;
    }

    /* choose pivot */
    a = ss_pivot(Td, B, PA, first, last);
    v = Td[B[PA + B[a]]];
    SWAP(B[first], B[a]);

    /* partition */
    for(b = first; (++b < last) && ((x = Td[B[PA + B[b]]]) == v);) { }
    if(((a = b) < last) && (x < v)) {
      for(; (++b < last) && ((x = Td[B[PA + B[b]]]) <= v);) {
        if(x == v) { SWAP(B[b], B[a]); ++a; }
      }
    }
    for(c = last; (b < --c) && ((x = Td[B[PA + B[c]]]) == v);) { }
    if((b < (d = c)) && (x > v)) {
      for(; (b < --c) && ((x = Td[B[PA + B[c]]]) >= v);) {
        if(x == v) { SWAP(B[c], B[d]); --d; }
      }
    }
    for(; b < c;) {
      SWAP(B[b], B[c]);
      for(; (++b < c) && ((x = Td[B[PA + B[b]]]) <= v);) {
        if(x == v) { SWAP(B[b], B[a]); ++a; }
      }
      for(; (b < --c) && ((x = Td[B[PA + B[c]]]) >= v);) {
        if(x == v) { SWAP(B[c], B[d]); --d; }
      }
    }

    if(a <= d) {
      c = b - 1;

      if((s = a - first) > (t = b - a)) { s = t; }
      for(e = first, f = b - s; 0 < s; --s, ++e, ++f) { SWAP(B[e], B[f]); }
      if((s = d - c) > (t = last - d - 1)) { s = t; }
      for(e = b, f = last - s; 0 < s; --s, ++e, ++f) { SWAP(B[e], B[f]); }

      a = first + (b - a), c = last - (d - c);
      b = (v <= Td[B[PA + B[a]] - 1]) ? a : ss_partition(B, PA, a, c, depth);

      if((a - first) <= (last - c)) {
        if((last - c) <= (c - b)) {
          STACK_PUSH(b, c, depth + 1, ilg<saidx_t>(c - b));
          STACK_PUSH(c, last, depth, limit);
          last = a;
        } else if((a - first) <= (c - b)) {
          STACK_PUSH(c, last, depth, limit);
          STACK_PUSH(b, c, depth + 1, ilg<saidx_t>(c - b));
          last = a;
        } else {
          STACK_PUSH(c, last, depth, limit);
          STACK_PUSH(first, a, depth, limit);
          first = b, last = c, depth += 1, limit = ilg<saidx_t>(c - b);
        }
      } else {
        if((a - first) <= (c - b)) {
          STACK_PUSH(b, c, depth + 1, ilg<saidx_t>(c - b));
          STACK_PUSH(first, a, depth, limit);
          first = c;
        } else if((last - c) <= (c - b)) {
          STACK_PUSH(first, a, depth, limit);
          STACK_PUSH(b, c, depth + 1, ilg<saidx_t>(c - b));
          first = c;
        } else {
          STACK_PUSH(first, a, depth, limit);
          STACK_PUSH(c, last, depth, limit);
          first = b, last = c, depth += 1, limit = ilg<saidx_t>(c - b);
        }
      }
    } else {
      limit += 1;
      if(Td[B[PA + B[first]] - 1] < v) {
        first = ss_partition(B, PA, first, last, depth);
        limit = ilg<saidx_t>(last - first);
      }
      depth += 1;
    }
  }
#undef STACK_SIZE
}

#endif /* (SS_BLOCKSIZE == 0) || (SS_INSERTIONSORT_THRESHOLD < SS_BLOCKSIZE) */


/*---------------------------------------------------------------------------*/

#if SS_BLOCKSIZE != 0

template<typename buffer_t>
inline void ss_blockswap(buffer_t& B, saidx_t a, saidx_t b, saidx_t n) {
  saidx_t t;
  for(; 0 < n; --n, ++a, ++b) {
    t = B[a], B[a] = B[b], B[b] = t;
  }
}

template<typename buffer_t>
inline void ss_rotate(buffer_t& B, saidx_t first, saidx_t middle, saidx_t last) {
  saidx_t a, b;
  saidx_t t, l, r;
  l = middle - first, r = last - middle;
  for(; (0 < l) && (0 < r);) {
    if(l == r) { ss_blockswap(B, first, middle, l); break; }
    if(l < r) {
      a = last - 1, b = middle - 1;
      t = B[a];
      do {
        B[a--] = B[b], B[b--] = B[a];
        if(b < first) {
          B[a] = t;
          last = a;
          if((r -= l + 1) <= l) { break; }
          a -= 1, b = middle - 1;
          t = B[a];
        }
      } while(1);
    } else {
      a = first, b = middle;
      t = B[a];
      do {
        B[a++] = B[b], B[b++] = B[a];
        if(last <= b) {
          B[a] = t;
          first = a + 1;
          if((l -= r + 1) <= r) { break; }
          a += 1, b = middle;
          t = B[a];
        }
      } while(1);
    }
  }
}


/*---------------------------------------------------------------------------*/

template<typename buffer_t>
inline void ss_inplacemerge(const sauchar_t *T, buffer_t& B, const saidx_t PA,
                saidx_t first, saidx_t middle, saidx_t last,
                saidx_t depth) {
  saidx_t p;
  saidx_t a, b;
  saidx_t len, half;
  saint_t q, r;
  saint_t x;

  for(;;) {
    if(B[last - 1] < 0) { x = 1; p = PA + ~B[last - 1]; }
    else                { x = 0; p = PA +  B[last - 1]; }
    for(a = first, len = middle - first, half = len >> 1, r = -1;
        0 < len;
        len = half, half >>= 1) {
      b = a + half;
      q = ss_compare(T, B, PA + ((0 <= B[b]) ? B[b] : ~B[b]), p, depth);
      if(q < 0) {
        a = b + 1;
        half -= (len & 1) ^ 1;
      } else {
        r = q;
      }
    }
    if(a < middle) {
      if(r == 0) { B[a] = ~B[a]; }
      ss_rotate(B, a, middle, last);
      last -= middle - a;
      middle = a;
      if(first == middle) { break; }
    }
    --last;
    if(x != 0) { while(B[--last] < 0) { } }
    if(middle == last) { break; }
  }
}


/*---------------------------------------------------------------------------*/

/* Merge-forward with internal buffer. */
template<typename buffer_t>
inline void ss_mergeforward(const sauchar_t *T, buffer_t& B, const saidx_t PA,
                saidx_t first, saidx_t middle, saidx_t last,
                saidx_t buf, saidx_t depth) {
  saidx_t a, b, c, bufend;
  saidx_t t;
  saint_t r;

  bufend = buf + (middle - first) - 1;

  ss_blockswap(B, buf, first, middle - first);

  for(t = B[a = first], b = buf, c = middle;;) {
    r = ss_compare(T, B, PA + B[b], PA + B[c], depth);
    if(r < 0) {
      do {
        B[a++] = B[b];
        if(bufend <= b) { B[bufend] = t; return; }
        B[b++] = B[a];
      } while(B[b] < 0);
    } else if(r > 0) {
      do {
        B[a++] = B[c], B[c++] = B[a];
        if(last <= c) {
          while(b < bufend) { B[a++] = B[b], B[b++] = B[a]; }
          B[a] = B[b], B[b] = t;
          return;
        }
      } while(B[c] < 0);
    } else {
      B[c] = ~B[c];
      do {
        B[a++] = B[b];
        if(bufend <= b) { B[bufend] = t; return; }
        B[b++] = B[a];
      } while(B[b] < 0);

      do {
        B[a++] = B[c], B[c++] = B[a];
        if(last <= c) {
          while(b < bufend) { B[a++] = B[b], B[b++] = B[a]; }
          B[a] = B[b], B[b] = t;
          return;
        }
      } while(B[c] < 0);
    }
  }
}

/* Merge-backward with internal buffer. */
template<typename buffer_t>
inline void ss_mergebackward(const sauchar_t *T, buffer_t& B, const saidx_t PA,
                 saidx_t first, saidx_t middle, saidx_t last,
                 saidx_t buf, saidx_t depth) {
  saidx_t p1, p2;
  saidx_t a, b, c, bufend;
  saidx_t t;
  saint_t r;
  saint_t x;

  bufend = buf + (last - middle) - 1;

  ss_blockswap(B, buf, middle, last - middle);

  x = 0;
  if(B[bufend] < 0)     { p1 = PA + ~B[bufend]; x |= 1; }
  else                  { p1 = PA +  B[bufend]; }
  if(B[middle - 1] < 0) { p2 = PA + ~B[middle - 1]; x |= 2; }
  else                  { p2 = PA +  B[middle - 1]; }
  for(t = B[a = last - 1], b = bufend, c = middle - 1;;) {
    r = ss_compare(T, B, p1, p2, depth);
    if(0 < r) {
      if(x & 1) { do { B[a--] = B[b], B[b--] = B[a]; } while(B[b] < 0); x ^= 1; }
      B[a--] = B[b];
      if(b <= buf) { B[buf] = t; break; }
      B[b--] = B[a];
      if(B[b] < 0) { p1 = PA + ~B[b]; x |= 1; }
      else         { p1 = PA +  B[b]; }
    } else if(r < 0) {
      if(x & 2) { do { B[a--] = B[c], B[c--] = B[a]; } while(B[c] < 0); x ^= 2; }
      B[a--] = B[c], B[c--] = B[a];
      if(c < first) {
        while(buf < b) { B[a--] = B[b], B[b--] = B[a]; }
        B[a] = B[b], B[b] = t;
        break;
      }
      if(B[c] < 0) { p2 = PA + ~B[c]; x |= 2; }
      else       { p2 = PA +  B[c]; }
    } else {
      if(x & 1) { do { B[a--] = B[b], B[b--] = B[a]; } while(B[b] < 0); x ^= 1; }
      B[a--] = ~B[b];
      if(b <= buf) { B[buf] = t; break; }
      B[b--] = B[a];
      if(x & 2) { do { B[a--] = B[c], B[c--] = B[a]; } while(B[c] < 0); x ^= 2; }
      B[a--] = B[c], B[c--] = B[a];
      if(c < first) {
        while(buf < b) { B[a--] = B[b], B[b--] = B[a]; }
        B[a] = B[b], B[b] = t;
        break;
      }
      if(B[b] < 0) { p1 = PA + ~B[b]; x |= 1; }
      else       { p1 = PA +  B[b]; }
      if(B[c] < 0) { p2 = PA + ~B[c]; x |= 2; }
      else       { p2 = PA +  B[c]; }
    }
  }
}

/* D&C based merge. */
template<typename buffer_t>
inline void ss_swapmerge(const sauchar_t *T, buffer_t& B, const saidx_t PA,
             saidx_t first, saidx_t middle, saidx_t last,
             saidx_t buf, saidx_t bufsize, saidx_t depth) {
#define STACK_SIZE SS_SMERGE_STACKSIZE
#define GETIDX(a) ((0 <= (a)) ? (a) : (~(a)))
#define MERGE_CHECK(a, b, c)\
  do {\
    if(((c) & 1) ||\
       (((c) & 2) && (ss_compare(T, B, PA + GETIDX(B[(a) - 1]), PA + B[a], depth) == 0))) {\
      B[a] = ~B[a];\
    }\
    if(((c) & 4) && ((ss_compare(T, B, PA + GETIDX(B[(b) - 1]), PA + B[b], depth) == 0))) {\
      B[b] = ~B[b];\
    }\
  } while(0)
  struct { saidx_t a, b, c; saint_t d; } stack[STACK_SIZE];
  saidx_t l, r, lm, rm;
  saidx_t m, len, half;
  saint_t ssize;
  saint_t check, next;

  for(check = 0, ssize = 0;;) {
    if((last - middle) <= bufsize) {
      if((first < middle) && (middle < last)) {
        ss_mergebackward(T, B, PA, first, middle, last, buf, depth);
      }
      MERGE_CHECK(first, last, check);
      STACK_POP(first, middle, last, check);
      continue;
    }

    if((middle - first) <= bufsize) {
      if(first < middle) {
        ss_mergeforward(T, B, PA, first, middle, last, buf, depth);
      }
      MERGE_CHECK(first, last, check);
      STACK_POP(first, middle, last, check);
      continue;
    }

    for(m = 0, len = MIN(middle - first, last - middle), half = len >> 1;
        0 < len;
        len = half, half >>= 1) {
      if(ss_compare(T, B, PA + GETIDX(B[middle + m + half]),
                          PA + GETIDX(B[middle - m - half - 1]), depth) < 0) {
        m += half + 1;
        half -= (len & 1) ^ 1;
      }
    }

    if(0 < m) {
      lm = middle - m, rm = middle + m;
      ss_blockswap(B, lm, middle, m);
      l = r = middle, next = 0;
      if(rm < last) {
        if(B[rm] < 0) {
          B[rm] = ~B[rm];
          if(first < lm) { for(; B[--l] < 0;) { } next |= 4; }
          next |= 1;
        } else if(first < lm) {
          for(; B[r] < 0; ++r) { }
          next |= 2;
        }
      }

      if((l - first) <= (last - r)) {
        STACK_PUSH(r, rm, last, (next & 3) | (check & 4));
        middle = lm, last = l, check = (check & 3) | (next & 4);
      } else {
        if((next & 2) && (r == middle)) { next ^= 6; }
        STACK_PUSH(first, lm, l, (check & 3) | (next & 4));
        first = r, middle = rm, check = (next & 3) | (check & 4);
      }
    } else {
      if(ss_compare(T, B, PA + GETIDX(B[middle - 1]), PA + B[middle], depth) == 0) {
        B[middle] = ~B[middle];
      }
      MERGE_CHECK(first, last, check);
      STACK_POP(first, middle, last, check);
    }
  }
#undef STACK_SIZE
}

#endif /* SS_BLOCKSIZE != 0 */


/*---------------------------------------------------------------------------*/

/*- Function -*/

/* Substring sort */
template<typename buffer_t>
inline void sssort(const sauchar_t *T, buffer_t& B, const saidx_t PA,
       saidx_t first, saidx_t last,
       saidx_t buf, saidx_t bufsize,
       saidx_t depth, saidx_t n, saint_t lastsuffix) {
  saidx_t a;
#if SS_BLOCKSIZE != 0
  saidx_t b, middle, curbuf;
  saidx_t j, k, curbufsize, limit;
#endif
  saidx_t i;

  if(lastsuffix != 0) { ++first; }

#if SS_BLOCKSIZE == 0
  ss_mintrosort(T, B, PA, first, last, depth);
#else
  if((bufsize < SS_BLOCKSIZE) &&
      (bufsize < (last - first)) &&
      (bufsize < (limit = ss_isqrt(last - first)))) {
    if(SS_BLOCKSIZE < limit) { limit = SS_BLOCKSIZE; }
    buf = middle = last - limit, bufsize = limit;
  } else {
    middle = last, limit = 0;
  }
  for(a = first, i = 0; SS_BLOCKSIZE < (middle - a); a += SS_BLOCKSIZE, ++i) {
#if SS_INSERTIONSORT_THRESHOLD < SS_BLOCKSIZE
    ss_mintrosort(T, B, PA, a, a + SS_BLOCKSIZE, depth);
#elif 1 < SS_BLOCKSIZE
    ss_insertionsort(T, B, PA, a, a + SS_BLOCKSIZE, depth);
#endif
    curbufsize = last - (a + SS_BLOCKSIZE);
    curbuf = a + SS_BLOCKSIZE;
    if(curbufsize <= bufsize) { curbufsize = bufsize, curbuf = buf; }
    for(b = a, k = SS_BLOCKSIZE, j = i; j & 1; b -= k, k <<= 1, j >>= 1) {
      ss_swapmerge(T, B, PA, b - k, b, b + k, curbuf, curbufsize, depth);
    }
  }
#if SS_INSERTIONSORT_THRESHOLD < SS_BLOCKSIZE
  ss_mintrosort(T, B, PA, a, middle, depth);
#elif 1 < SS_BLOCKSIZE
  ss_insertionsort(T, B, PA, a, middle, depth);
#endif
  for(k = SS_BLOCKSIZE; i != 0; k <<= 1, i >>= 1) {
    if(i & 1) {
      ss_swapmerge(T, B, PA, a - k, a, middle, buf, bufsize, depth);
      a -= k;
    }
  }
  if(limit != 0) {
#if SS_INSERTIONSORT_THRESHOLD < SS_BLOCKSIZE
    ss_mintrosort(T, B, PA, middle, last, depth);
#elif 1 < SS_BLOCKSIZE
    ss_insertionsort(T, B, PA, middle, last, depth);
#endif
    ss_inplacemerge(T, B, PA, first, middle, last, depth);
  }
#endif

  if(lastsuffix != 0) {
    /* Insert last type B* suffix. */
    saidx_t PAi[2]; PAi[0] = B[PA + B[first - 1]], PAi[1] = n - 2;
    for(a = first, i = B[first - 1];
        (a < last) && ((B[a] < 0) || (0 < ss_compare(T, B, PAi, PA + B[a], 0, depth)));
        ++a) {
      B[a - 1] = B[a];
    }
    B[a - 1] = i;
  }
}

}} //ns
