#include "nqueens.h"

#include <algorithm>
#include <array>
#include <cstdint>

namespace {

constexpr int kMaxBoardSize = 12;
using Mask = std::uint32_t;
using Board = std::vector<std::pair<int, int>>;

struct RowSearch {
  int n;
  std::array<Mask, kMaxBoardSize> free{}, queens{};
  std::array<std::array<Mask, kMaxBoardSize>, kMaxBoardSize> segments{};
  std::array<int, kMaxBoardSize> count{};
  std::array<int, kMaxBoardSize + 1> rowCapacity{}, columnCapacity{};
  int plainSuffix = 0;
  bool transpose = false;
  std::vector<Board> results;
  int target = 0;
  unsigned prunes = 0;

  explicit RowSearch(int size, const std::vector<std::string>& grid) : n(size) {
    int horizontal = 0, vertical = 0;
    for (int r = 0; r < n; ++r) {
      for (int c = 0; c < n; ++c) {
        if (grid[r][c] == 'W') continue;
        horizontal += c == 0 || grid[r][c - 1] == 'W';
        vertical += r == 0 || grid[r - 1][c] == 'W';
      }
    }
    // Search along the axis with fewer segments to tighten the starting bound.
    transpose = vertical < horizontal;
    for (int r = 0; r < n; ++r) {
      for (int c = 0; c < n; ++c) {
        if ((transpose ? grid[c][r] : grid[r][c]) == 'W') continue;
        if (c == 0 || !(free[r] & (1u << (c - 1)))) ++count[r];
        free[r] |= 1u << c;
        segments[r][count[r] - 1] |= 1u << c;
      }
    }
    plainSuffix = n;
    while (plainSuffix > 0 && free[plainSuffix - 1] == (1u << n) - 1) --plainSuffix;
    // Count vertical runs by their bottom cell, including runs crossing row r.
    Mask below = 0;
    for (int r = n - 1; r >= 0; --r) {
      rowCapacity[r] = rowCapacity[r + 1] + count[r];
      columnCapacity[r] = columnCapacity[r + 1] + __builtin_popcount(free[r] & ~below);
      below = free[r];
    }
  }

  Board board(int rows) const {
    Board result;
    result.reserve(target);
    for (int r = 0; r < rows; ++r) {
      for (Mask bits = queens[r]; bits; bits &= bits - 1) {
        const int c = __builtin_ctz(bits);
        if (transpose) result.emplace_back(c, r);
        else result.emplace_back(r, c);
      }
    }
    if (transpose) std::sort(result.begin(), result.end());
    return result;
  }

  template <bool Debug, bool Reflect>
  void emit() {
    results.push_back(board(n));
    if constexpr (Debug)
      NQ_SNAPSHOT(results.back(), results.size() == 1 ? "new record" : "another maximum board");
    if constexpr (Reflect) {
      Board copy = results.back();
      for (auto& queen : copy) queen.second = n - 1 - queen.second;
      results.push_back(std::move(copy));
      if constexpr (Debug) NQ_SNAPSHOT(results.back(), "reflected maximum board");
    }
  }

  // With no walls or skips left, each remaining row must supply one queen.
  template <bool Debug, bool Reflect = false>
  void plainRows(int r, Mask columns, Mask left, Mask right) {
    if (r == n) {
      emit<Debug, Reflect>();
      return;
    }
    Mask bits = free[r] & ~(columns | left | right);
    while (bits) {
      const Mask bit = bits & -bits;
      bits -= bit;
      queens[r] = bit;
      plainRows<Debug, Reflect>(r + 1, columns | bit, (left | bit) << 1, (right | bit) >> 1);
    }
  }

  template <bool Debug>
  void row(int r, Mask columns, Mask left, Mask right, int skips) {
    if (skips == 0 && r >= plainSuffix) {
      plainRows<Debug>(r, columns, left, right);
      return;
    }
    if (r == n) return;
    // Only a wall on the ray itself can stop it. Clip before shifting diagonals.
    columns &= free[r];
    left &= free[r];
    right &= free[r];
    const int needed = rowCapacity[r] - skips;
    if (needed > columnCapacity[r] - __builtin_popcount(columns)) {
      if constexpr (Debug) {
        if (++prunes % 1000 == 0) NQ_SNAPSHOT(board(r), "pruned by column capacity");
      }
      return;
    }
    const Mask available = free[r] & ~(columns | left | right);
    choose<Debug>(r, 0, available, 0, columns, left, right, skips);
  }

  template <bool Debug>
  void choose(int r, int segment, Mask available, Mask placed, Mask columns,
              Mask left, Mask right, int skips) {
    if (segment == count[r]) {
      queens[r] = placed;
      row<Debug>(r + 1, columns | placed, (left | placed) << 1, (right | placed) >> 1, skips);
      return;
    }
    for (Mask bits = available & segments[r][segment]; bits; bits &= bits - 1) {
      const Mask bit = bits & -bits;
      choose<Debug>(r, segment + 1, available, placed | bit, columns, left, right, skips);
    }
    if (skips > 0)
      choose<Debug>(r, segment + 1, available, placed, columns, left, right, skips - 1);
  }

  template <bool Debug>
  void run() {
    if (plainSuffix == 0 && n >= 4) {
      target = n;
      // The first queen distinguishes each left-hand solution from its mirror.
      for (int c = 0; c < n / 2; ++c) {
        const Mask bit = queens[0] = 1u << c;
        plainRows<Debug, true>(1, bit, bit << 1, bit >> 1);
      }
      if (n % 2) {
        const Mask bit = queens[0] = 1u << (n / 2);
        plainRows<Debug>(1, bit, bit << 1, bit >> 1);
      }
      return;
    }
    // Exactly one decision per segment. The first attainable target is optimal.
    for (target = rowCapacity[0]; target >= 0 && results.empty(); --target)
      row<Debug>(0, 0, 0, 0, rowCapacity[0] - target);
  }
};

constexpr int kMaxCells = kMaxBoardSize * kMaxBoardSize;
constexpr int kWords = (kMaxCells + 63) / 64;

struct Cells {
  std::array<std::uint64_t, kWords> words{};

  int first() const {
    for (int w = 0; w < kWords; ++w)
      if (words[w]) return 64 * w + __builtin_ctzll(words[w]);
    return -1;
  }
  void add(int i) { words[i / 64] |= std::uint64_t{1} << (i % 64); }
  void erase(int i) { words[i / 64] &= ~(std::uint64_t{1} << (i % 64)); }

  Cells without(const Cells& other) const {
    Cells result;
    for (int w = 0; w < kWords; ++w) result.words[w] = words[w] & ~other.words[w];
    return result;
  }
};

struct GraphSearch {
  Cells all;
  std::array<Cells, kMaxCells> conflicts{};
  std::array<std::pair<int, int>, kMaxCells> positions{}, path{};
  std::vector<Board> results;
  int best = 0;
  unsigned prunes = 0;

  GraphSearch(int n, const std::vector<std::string>& grid) {
    int ids[kMaxBoardSize][kMaxBoardSize]{};
    int count = 0;
    for (int r = 0; r < n; ++r) {
      for (int c = 0; c < n; ++c) {
        if (grid[r][c] == 'W') continue;
        ids[r][c] = count;
        positions[count] = {r, c};
        all.add(count++);
      }
    }
    for (int i = 0; i < count; ++i) {
      const auto [r, c] = positions[i];
      for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
          if (dr == 0 && dc == 0) continue;
          for (int y = r + dr, x = c + dc;
               y >= 0 && y < n && x >= 0 && x < n && grid[y][x] != 'W'; y += dr, x += dc)
            conflicts[i].add(ids[y][x]);
        }
      }
    }
  }

  template <bool Debug>
  void visit(Cells live, int depth) {
    // Greedily partition the remaining cells into mutually attacking groups.
    // Every prefix of this order fits at most one queen per group in the prefix.
    std::array<std::uint8_t, kMaxCells> order, bound;
    int length = 0, groups = 0;
    Cells rest = live;
    while (rest.first() >= 0) {
      ++groups;
      Cells group = rest;
      for (int i = group.first(); i >= 0; i = group.first()) {
        order[length] = i;
        bound[length++] = groups;
        rest.erase(i);
        for (int w = 0; w < kWords; ++w) group.words[w] &= conflicts[i].words[w];
      }
    }
    if (length == 0 && depth >= best) {
      const bool record = results.empty() || depth > best;
      if (record) results.clear();
      best = depth;
      Board& result = results.emplace_back(path.begin(), path.begin() + depth);
      std::sort(result.begin(), result.end());
      if constexpr (Debug) NQ_SNAPSHOT(result, record ? "new record" : "another maximum board");
    }
    // Each extension has exactly one last cell in this node's order.
    while (length > 0) {
      --length;
      if (depth + bound[length] < best) {
        if constexpr (Debug) {
          if (++prunes % 1000 == 0)
            NQ_SNAPSHOT(Board(path.begin(), path.begin() + depth), "pruned by group bound");
        }
        return;
      }
      const int i = order[length];
      live.erase(i);
      path[depth] = positions[i];
      visit<Debug>(live.without(conflicts[i]), depth + 1);
    }
  }
};

}  // namespace

std::vector<std::vector<std::pair<int, int>>> solveWalled(int n, const std::vector<std::string>& grid) {
  if (n < 0 || n > kMaxBoardSize || static_cast<int>(grid.size()) != n) return {};
  for (const auto& line : grid) if (static_cast<int>(line.size()) != n) return {};
  RowSearch rows(n, grid);
  // Few extra row segments keep a row-by-row search tight. On more fragmented
  // boards, use a fresh bound from the actual surviving cells at each node.
  if (rows.rowCapacity[0] > n + 2) {
    GraphSearch graph(n, grid);
    if (snapshotsEnabled()) graph.visit<true>(graph.all, 0);
    else graph.visit<false>(graph.all, 0);
    return std::move(graph.results);
  }
  if (snapshotsEnabled()) rows.run<true>();
  else rows.run<false>();
  return std::move(rows.results);
}
