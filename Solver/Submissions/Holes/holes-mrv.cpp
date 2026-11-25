#include <cstddef>
#include <cstdint>
#include <new>
#include <string>
#include <vector>

#include <unistd.h>

#include "nqueens.h"

namespace {

char* g_at = nullptr;
char* g_end = nullptr;

inline void* bump(std::size_t bytes) {
  bytes = (bytes + 3) & ~static_cast<std::size_t>(3);
  if (!bytes) bytes = 4;
  if (static_cast<std::size_t>(g_end - g_at) < bytes) return sbrk(static_cast<intptr_t>(bytes));
  void* p = g_at;
  g_at += bytes;
  return p;
}

inline void prealloc(std::size_t bytes) {
  bytes = (bytes + 3) & ~static_cast<std::size_t>(3);
  g_at = static_cast<char*>(sbrk(static_cast<intptr_t>(bytes)));
  g_end = g_at + bytes;
}

const int kMaxBuffered = 12;
const int kMaxSolutions = 14200;
uint8_t g_buffer[kMaxSolutions * kMaxBuffered];

struct Solver {
  int n;
  int last;
  int m;
  uint32_t full;
  uint32_t allowed[32];
  uint32_t rowMask[32];
  uint8_t holeImg[16][16];
  int cols[32];
  long count;
  uint8_t* at;
  std::vector<std::vector<int>>* out;

  inline void emit(const int* s) {
    ++count;
    if (at) {
      for (int r = 0; r < n; ++r) at[r] = static_cast<uint8_t>(s[r]);
      at += n;
    } else if (out) {
      out->emplace_back(s, s + n);
    }
  }

  void plain(int row, uint32_t taken, uint32_t left, uint32_t right) {
    uint32_t free = allowed[row] & ~(taken | left | right);
    if (row == last) {
      if (free) {
        cols[row] = __builtin_ctz(free);
        emit(cols);
      }
      return;
    }
    while (free) {
      const uint32_t bit = free & (0u - free);
      free ^= bit;
      cols[row] = __builtin_ctz(bit);
      plain(row + 1, taken | bit, ((left | bit) << 1) & full, (right | bit) >> 1);
    }
  }

  void mrv(uint32_t rows, uint32_t taken, uint32_t d1, uint32_t d2) {
    int row;
    uint32_t free;
    for (;;) {
      int best = 99;
      row = 0;
      free = 0;
      uint32_t rr = rows;
      do {
        const int r = __builtin_ctz(rr);
        rr &= rr - 1;
        const uint32_t f = allowed[r] & ~(taken | (d1 >> r) | (d2 >> (last - r)));
        if (!f) return;
        const int k = __builtin_popcount(f);
        if (k < best) {
          best = k;
          row = r;
          free = f;
        }
      } while (rr);
      rows &= ~(1u << row);
      if (!rows) {
        do {
          cols[row] = __builtin_ctz(free);
          free &= free - 1;
          emit(cols);
        } while (free);
        return;
      }
      if (best != 1) break;
      cols[row] = __builtin_ctz(free);
      taken |= free;
      d1 |= free << row;
      d2 |= free << (last - row);
    }
    do {
      const uint32_t bit = free & (0u - free);
      free ^= bit;
      cols[row] = __builtin_ctz(bit);
      mrv(rows, taken | bit, d1 | (bit << row), d2 | (bit << (last - row)));
    } while (free);
  }

  void buildHoleImages() {
    const int L = last;
    for (int r = 0; r < n; ++r)
      for (int c = 0; c < n; ++c) {
        const int ir[8] = {r, r, L - r, L - r, c, c, L - c, L - c};
        const int ic[8] = {c, L - c, c, L - c, r, L - r, r, L - r};
        uint8_t bits = 0;
        for (int t = 0; t < 8; ++t)
          if (!((allowed[ir[t]] >> ic[t]) & 1u)) bits |= static_cast<uint8_t>(1u << t);
        holeImg[r][c] = bits;
      }
  }

  void found(uint32_t alive) {
    int q[8][16];
    uint64_t key[8];
    for (int r = 0; r < n; ++r) {
      q[0][r] = cols[r];
      q[4][cols[r]] = r;
    }
    for (int b = 0; b < 8; b += 4)
      for (int r = 0; r < n; ++r) {
        q[b + 1][r] = last - q[b][r];
        q[b + 2][r] = q[b][last - r];
        q[b + 3][r] = last - q[b][last - r];
      }
    for (int i = 0; i < 8; ++i) {
      uint64_t k = 0;
      for (int r = 0; r < n; ++r) k = (k << 4) | static_cast<uint64_t>(q[i][r]);
      key[i] = k;
    }
    for (int i = 1; i < 8; ++i)
      if (q[i][0] == m && key[i] < key[0]) return;
    for (int i = 0; i < 8; ++i) {
      if (!((alive >> i) & 1u)) continue;
      bool fresh = true;
      for (int j = 0; j < i; ++j)
        if (key[j] == key[i]) {
          fresh = false;
          break;
        }
      if (fresh) emit(q[i]);
    }
  }

  void sym(int row, uint32_t taken, uint32_t left, uint32_t right, uint32_t alive) {
    uint32_t free = rowMask[row] & ~(taken | left | right);
    if (row == last - 1) {
      while (free) {
        const uint32_t bit = free & (0u - free);
        free ^= bit;
        const int c = __builtin_ctz(bit);
        const uint32_t a = alive & ~holeImg[row][c];
        if (!a) continue;
        const uint32_t end = rowMask[last] & ~(taken | bit | (((left | bit) << 1) & full) | ((right | bit) >> 1));
        if (!end) continue;
        const int ce = __builtin_ctz(end);
        const uint32_t a2 = a & ~holeImg[last][ce];
        if (!a2) continue;
        cols[row] = c;
        cols[last] = ce;
        found(a2);
      }
      return;
    }
    while (free) {
      const uint32_t bit = free & (0u - free);
      free ^= bit;
      const int c = __builtin_ctz(bit);
      const uint32_t a = alive & ~holeImg[row][c];
      if (!a) continue;
      cols[row] = c;
      sym(row + 1, taken | bit, ((left | bit) << 1) & full, (right | bit) >> 1, a);
    }
  }

  void symmetric() {
    m = 0;
    cols[0] = 0;
    const uint32_t a0 = 0xFFu & ~holeImg[0][0];
    if (a0)
      for (int c = 2; c <= last; ++c) {
        const uint32_t a1 = a0 & ~holeImg[1][c];
        if (!a1) continue;
        for (int r = 2; r <= last; ++r) rowMask[r] = r < c ? full & ~2u : full;
        cols[1] = c;
        const uint32_t bit = 1u << c;
        sym(2, 1u | bit, ((2u | bit) << 1) & full, bit >> 1, a1);
      }
    const uint32_t edges = 1u | (1u << last);
    for (m = 1; m <= last / 2; ++m) {
      const uint32_t a = 0xFFu & ~holeImg[0][m];
      if (!a) continue;
      for (int r = 1; r < last; ++r)
        rowMask[r] = (r < m || r > last - m) ? full & ~edges : full;
      rowMask[last] = (full >> m) & (full << m) & full;
      cols[0] = m;
      const uint32_t bit = 1u << m;
      sym(1, bit, (bit << 1) & full, bit >> 1, a);
    }
  }

  void run(bool useSym) {
    if (useSym)
      symmetric();
    else if (n <= 16)
      mrv(full, 0, 0, 0);
    else
      plain(0, 0, 0, 0);
  }
};

}

void* operator new(std::size_t bytes) { return bump(bytes); }
void* operator new[](std::size_t bytes) { return bump(bytes); }
void operator delete(void*) noexcept {}
void operator delete[](void*) noexcept {}
void operator delete(void*, std::size_t) noexcept {}
void operator delete[](void*, std::size_t) noexcept {}

std::vector<std::vector<int>> solveHoles(int n, const std::vector<std::string>& grid) {
  std::vector<std::vector<int>> solutions;
  if (n <= 0 || n > 31) return solutions;

  Solver s;
  s.n = n;
  s.last = n - 1;
  s.full = (1u << n) - 1;
  s.count = 0;
  s.at = nullptr;
  s.out = nullptr;
  int holeCount = 0;
  for (int r = 0; r < n; ++r) {
    uint32_t mask = 0;
    for (int c = 0; c < n; ++c)
      if (grid[r][c] != 'H')
        mask |= 1u << c;
      else
        ++holeCount;
    if (!mask) return solutions;
    s.allowed[r] = mask;
  }

  const bool useSym = n >= 5 && n <= 16 && holeCount * 12 < n * n;
  if (useSym) s.buildHoleImages();

  if (n <= kMaxBuffered) {
    s.at = g_buffer;
    s.run(useSym);
    if (!s.count) return solutions;
    const std::size_t count = static_cast<std::size_t>(s.count);
    prealloc(count * (sizeof(std::vector<int>) + sizeof(int) * static_cast<std::size_t>(n)));
    solutions.reserve(count);
    const uint8_t* p = g_buffer;
    for (std::size_t i = 0; i < count; ++i, p += n) solutions.emplace_back(p, p + n);
  } else {
    s.run(useSym);
    if (!s.count) return solutions;
    const std::size_t count = static_cast<std::size_t>(s.count);
    prealloc(count * (sizeof(std::vector<int>) + sizeof(int) * static_cast<std::size_t>(n)));
    solutions.reserve(count);
    s.count = 0;
    s.out = &solutions;
    s.run(useSym);
  }

  if (snapshotsEnabled())
    for (std::size_t i = 0; i < solutions.size(); ++i)
      snapshot(nq::queensOf(solutions[i]), "solution " + std::to_string(i + 1));
  return solutions;
}
