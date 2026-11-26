#include "nqueens.h"

#include <algorithm>
#include <array>
#include <cstdint>

namespace {

constexpr int kMaxBoardSize = 12;
constexpr int kMaxCells = kMaxBoardSize * kMaxBoardSize;
constexpr int kWords = (kMaxCells + 63) / 64;
using Board = std::vector<std::pair<int, int>>;

struct Mask {
  std::array<std::uint64_t, kWords> words{};
  void set(int i) { words[i / 64] |= std::uint64_t{1} << (i % 64); }
  bool intersects(const Mask& other) const {
    for (int w = 0; w < kWords; ++w)
      if (words[w] & other.words[w]) return true;
    return false;
  }
  Mask without(const Mask& other) const {
    Mask result;
    for (int w = 0; w < kWords; ++w) result.words[w] = words[w] & ~other.words[w];
    return result;
  }
};

struct Search {
  int n, cells = 0, best = 0;
  std::array<std::pair<int, int>, kMaxCells> positions{}, path{};
  std::array<Mask, kMaxCells> attacks{};
  std::array<std::array<Mask, kMaxCells>, 4> segments{};
  std::array<int, 4> counts{};
  std::vector<Board> results;
  std::uint64_t pruned = 0;

  Board board(int depth) const {
    Board result(path.begin(), path.begin() + depth);
    std::sort(result.begin(), result.end());
    return result;
  }

  template <bool Debug>
  void normal(int row, unsigned columns, unsigned left, unsigned right) {
    if (row == n) {
      results.emplace_back(path.begin(), path.begin() + n);
      if constexpr (Debug) NQ_SNAPSHOT(results.back(), "another maximum board");
      return;
    }
    unsigned available = ((1u << n) - 1) & ~(columns | left | right);
    while (available) {
      const unsigned bit = available & -available;
      available -= bit;
      path[row] = {row, __builtin_ctz(bit)};
      normal<Debug>(row + 1, columns | bit, (left | bit) << 1, (right | bit) >> 1);
    }
  }

  template <bool Debug>
  void visit(Mask available, int depth) {
    int bound = kMaxCells, chosen = -1, smallest = kMaxCells + 1;
    for (int d = 0; d < 4; ++d) {
      int live = 0;
      for (int s = 0; s < counts[d]; ++s) {
        const Mask& group = segments[d][s];
        if (!available.intersects(group)) continue;
        ++live;
        if (d == 0) {
          int size = 0;
          for (int w = 0; w < kWords; ++w)
            size += __builtin_popcountll(available.words[w] & group.words[w]);
          if (size < smallest) { smallest = size; chosen = s; }
        }
      }
      bound = std::min(bound, live);
    }
    // Equality must survive: all maximum boards are required.
    if (depth + bound < best) {
      if constexpr (Debug) {
        if (++pruned % 1000 == 0) NQ_SNAPSHOT(board(depth), "pruned by segment bound");
      }
      return;
    }
    if (chosen < 0) {
      if (depth > best) {
        best = depth;
        results.clear();
        if constexpr (Debug) NQ_SNAPSHOT(board(depth), "new record");
      }
      results.push_back(board(depth));
      if constexpr (Debug) NQ_SNAPSHOT(results.back(), "another maximum board");
      return;
    }
    // A row segment holds at most one queen. These disjoint choices replace
    // individual include/exclude decisions without changing the solution set.
    const Mask group = segments[0][chosen];
    for (int w = 0; w < kWords; ++w) {
      auto candidates = available.words[w] & group.words[w];
      while (candidates) {
        const int cell = 64 * w + __builtin_ctzll(candidates);
        candidates &= candidates - 1;
        path[depth] = positions[cell];
        visit<Debug>(available.without(attacks[cell]), depth + 1);
      }
    }
    visit<Debug>(available.without(group), depth);
  }

  Search(int size, const std::vector<std::string>& grid) : n(size) {
    int ids[kMaxBoardSize][kMaxBoardSize];
    for (int r = 0; r < n; ++r)
      for (int c = 0; c < n; ++c) {
        ids[r][c] = -1;
        if (grid[r][c] != 'W') {
          ids[r][c] = cells;
          positions[cells++] = {r, c};
        }
      }
    constexpr int dr[] = {0, 1, 1, 1}, dc[] = {1, 0, 1, -1};
    for (int d = 0; d < 4; ++d)
      for (int r = 0; r < n; ++r)
        for (int c = 0; c < n; ++c) {
          if (ids[r][c] < 0) continue;
          int pr = r - dr[d], pc = c - dc[d];
          if (pr >= 0 && pr < n && pc >= 0 && pc < n && ids[pr][pc] >= 0) continue;
          Mask& group = segments[d][counts[d]++];
          for (int y = r, x = c; y >= 0 && y < n && x >= 0 && x < n && ids[y][x] >= 0;
               y += dr[d], x += dc[d]) group.set(ids[y][x]);
          for (int i = 0; i < cells; ++i)
            if (group.words[i / 64] & (std::uint64_t{1} << (i % 64)))
              for (int w = 0; w < kWords; ++w) attacks[i].words[w] |= group.words[w];
        }
  }
};

}  // namespace

std::vector<std::vector<std::pair<int, int>>> solveWalled(int n, const std::vector<std::string>& grid) {
  if (n < 0 || n > kMaxBoardSize || static_cast<int>(grid.size()) != n) return {};
  for (const auto& row : grid) if (static_cast<int>(row.size()) != n) return {};
  Search search(n, grid);
  // For n >= 4 an empty board admits n queens; no row can hold two.
  // Small unsatisfiable normal boards still need maximum partial placements.
  if (n >= 4 && search.cells == n * n) {
    if (snapshotsEnabled()) search.normal<true>(0, 0, 0, 0);
    else search.normal<false>(0, 0, 0, 0);
    return std::move(search.results);
  }
  Mask available;
  for (int i = 0; i < search.cells; ++i) available.set(i);
  if (snapshotsEnabled()) search.visit<true>(available, 0);
  else search.visit<false>(available, 0);
  return std::move(search.results);
}
