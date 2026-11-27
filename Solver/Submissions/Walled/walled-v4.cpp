#include "nqueens.h"

#include <algorithm>
#include <array>
#include <cstdint>

namespace {

constexpr int kMaxN = 12;
using Cell = std::pair<int, int>;
using Board = std::vector<Cell>;
using Results = std::vector<Board>;

// With no walls and n >= 4, every optimum uses every row. Reflect solutions
// whose first queen is left of center; enumerate the odd center branch once.
struct PlainSearch {
  int n;
  std::array<Cell, kMaxN> path;
  Results results;

  template <bool Reflect>
  void visit(int row, unsigned columns, unsigned left, unsigned right) {
    if (row == n) {
      results.emplace_back(path.begin(), path.begin() + n);
      NQ_SNAPSHOT(results.back(), "maximum board");
      if constexpr (Reflect) {
        Board mirror(path.begin(), path.begin() + n);
        for (auto& queen : mirror) queen.second = n - 1 - queen.second;
        results.push_back(std::move(mirror));
        NQ_SNAPSHOT(results.back(), "reflected maximum board");
      }
      return;
    }
    unsigned safe = ((1u << n) - 1) & ~(columns | left | right);
    while (safe) {
      const unsigned bit = safe & -safe;
      safe -= bit;
      path[row] = {row, __builtin_ctz(bit)};
      visit<Reflect>(row + 1, columns | bit, (left | bit) << 1, (right | bit) >> 1);
    }
  }

  Results run() {
    for (int c = 0; c < (n + 1) / 2; ++c) {
      path[0] = {0, c};
      const unsigned bit = 1u << c;
      if (c < n / 2) visit<true>(1, bit, bit << 1, bit >> 1);
      else visit<false>(1, bit, bit << 1, bit >> 1);
    }
    return std::move(results);
  }
};

using Mask = std::uint32_t;

struct RowSearch {
  int n;
  std::array<Mask, kMaxN> free{}, queens{};
  std::array<std::array<Mask, kMaxN>, kMaxN> segments{};
  std::array<int, kMaxN> count{};
  std::array<int, kMaxN + 1> rowCapacity{}, columnCapacity{};
  int plainSuffix = 0;
  bool reverse = false;
  bool transpose = false;
  std::vector<Board> results;
  int target = 0;
  unsigned prunes = 0;

  explicit RowSearch(int size, const std::vector<std::string>& grid, bool swapAxes) : n(size), transpose(swapAxes) {
    for (int r = 0; r < n; ++r) {
      for (int c = 0; c < n; ++c) {
        if ((transpose ? grid[c][r] : grid[r][c]) == 'W') continue;
        if (c == 0 || !(free[r] & (1u << (c - 1)))) ++count[r];
        free[r] |= 1u << c;
        segments[r][count[r] - 1] |= 1u << c;
      }
    }
    // Visit walls early so a longer wall-free suffix uses plain bitboard DFS.
    const Mask full = (1u << n) - 1;
    int prefix = 0, suffix = 0;
    while (prefix < n && free[prefix] == full) ++prefix;
    while (suffix < n && free[n - 1 - suffix] == full) ++suffix;
    reverse = prefix > suffix;
    if (reverse) {
      std::reverse(free.begin(), free.begin() + n);
      std::reverse(count.begin(), count.begin() + n);
      std::reverse(segments.begin(), segments.begin() + n);
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
    std::array<Mask, kMaxN> original{};
    for (int r = 0; r < rows; ++r) {
      const int y = reverse ? n - 1 - r : r;
      if (transpose) {
        for (Mask bits = queens[r]; bits; bits &= bits - 1)
          original[__builtin_ctz(bits)] |= 1u << y;
      } else {
        original[y] = queens[r];
      }
    }
    for (int r = 0; r < n; ++r)
      for (Mask bits = original[r]; bits; bits &= bits - 1)
        result.emplace_back(r, __builtin_ctz(bits));
    return result;
  }

  template <bool Debug>
  void emit() {
    results.push_back(board(n));
    if constexpr (Debug)
      NQ_SNAPSHOT(results.back(), results.size() == 1 ? "new record" : "another maximum board");

  }

  // With no walls or skips left, each remaining row must supply one queen.
  template <bool Debug>
  void plainRows(int r, Mask columns, Mask left, Mask right) {
    if (r == n) {
      emit<Debug>();
      return;
    }
    Mask bits = free[r] & ~(columns | left | right);
    while (bits) {
      const Mask bit = bits & -bits;
      bits -= bit;
      queens[r] = bit;
      plainRows<Debug>(r + 1, columns | bit, (left | bit) << 1, (right | bit) >> 1);
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
    // Exactly one decision per segment. The first attainable target is optimal.
    for (target = rowCapacity[0]; target >= 0 && results.empty(); --target)
      row<Debug>(0, 0, 0, 0, rowCapacity[0] - target);
  }
};

template <int Words>
struct Bits {
  std::array<std::uint64_t, Words> word{};
  void set(int i) { word[i / 64] |= std::uint64_t{1} << (i % 64); }
  Bits without(const Bits& remove) const {
    Bits result;
    for (int w = 0; w < Words; ++w) result.word[w] = word[w] & ~remove.word[w];
    return result;
  }
  template <class Fn>
  void each(Fn fn) const {
    for (int w = 0; w < Words; ++w)
      for (auto b = word[w]; b; b &= b - 1) fn(64 * w + __builtin_ctzll(b));
  }
};

template <int Words>
struct Search {
  using Mask = Bits<Words>;
  std::vector<Cell> cells;
  std::vector<Mask> attacks;
  std::vector<std::uint64_t> runMask;
  std::vector<int> crossing;
  Mask all, starts, ends;
  std::array<unsigned, kMaxN> placed{};
  std::vector<int> first;
  int oppositeRuns = 0;
  Results results;
  int target = 0;

  Search(int n, int slots, bool transpose, const std::vector<std::string>& grid)
      : cells(slots), attacks(slots), runMask(slots), crossing(slots) {
    int index[kMaxN][kMaxN];
    int pos = 0;
    for (int line = 0; line < n; ++line) {
      for (int col = 0; col < n;) {
        int start = col;
        while (col < n && (transpose ? grid[col][line] : grid[line][col]) == '.') ++col;
        int length = col - start;
        if (!length) { ++col; continue; }
        if (pos % 64 + length > 64) pos += 64 - pos % 64;
        first.push_back(pos);
        starts.set(pos);
        ends.set(pos + length - 1);
        const auto group = ((std::uint64_t{1} << length) - 1) << (pos % 64);
        for (int c = start; c < col; ++c, ++pos) {
          int y = transpose ? c : line, x = transpose ? line : c;
          index[y][x] = pos;
          cells[pos] = {y, x};
          runMask[pos] = group;
          all.set(pos);
          const int py = transpose ? y : y - 1, px = transpose ? x - 1 : x;
          crossing[pos] = line && grid[py][px] == '.' ? crossing[index[py][px]] : oppositeRuns++;
        }
      }
    }
    all.each([&](int i) {
      const auto [r, c] = cells[i];
      attacks[i].set(i);
      for (int dr = -1; dr <= 1; ++dr)
        for (int dc = -1; dc <= 1; ++dc) {
          if (!dr && !dc) continue;
          for (int y = r + dr, x = c + dc;
               y >= 0 && y < n && x >= 0 && x < n && grid[y][x] != 'W'; y += dr, x += dc)
            attacks[i].set(index[y][x]);
        }
    });
  }

  // Every placement matches distinct horizontal and vertical runs. Ignoring
  // diagonal attacks gives a safe upper bound on its cardinality.
  int matchingBound() const {
    std::vector<int> owner(oppositeRuns, -1), seen(oppositeRuns, -1);
    auto augment = [&](auto&& self, int h, int stamp) -> bool {
      const int pos = first[h];
      for (auto b = runMask[pos]; b; b &= b - 1) {
          int v = crossing[(pos / 64) * 64 + __builtin_ctzll(b)];
          if (seen[v] == stamp) continue;
          seen[v] = stamp;
          if (owner[v] < 0 || self(self, owner[v], stamp)) {
            owner[v] = h;
            return true;
          }
      }
      return false;
    };
    int size = 0;
    for (int h = 0; h < static_cast<int>(first.size()); ++h)
      size += augment(augment, h, h);
    return size;
  }

  void emit() {
    Board& board = results.emplace_back();
    board.reserve(target);
    for (int r = 0; r < kMaxN; ++r)
      for (unsigned b = placed[r]; b; b &= b - 1)
        board.emplace_back(r, __builtin_ctz(b));
    NQ_SNAPSHOT(board, "maximum board");
  }

  // Each run occupies one contiguous field in a word. Adding ones below its
  // top bit carries into that bit exactly when a lower candidate is present.
  // Mask the top bits first, so no carry can escape into the next run.
  std::uint64_t activeEnds(std::uint64_t bits, int w) const {
    const auto top = ends.word[w];
    return (((bits & ~top) + (all.word[w] & ~top)) | bits) & top;
  }

  void visit(Mask live, int needed) {
    if (!needed) { emit(); return; }
    Mask active;
    int capacity = 0;
    for (int w = 0; w < Words; ++w) {
      active.word[w] = activeEnds(live.word[w], w);
      capacity += __builtin_popcountll(active.word[w]);
    }
    if (capacity < needed) return;

    // Clear the lowest candidate in every nonempty run in parallel. The first
    // runs to empty have the fewest candidates. Setting each top bit before
    // subtracting keeps the borrows inside their respective fields.
    Mask rest = live;
    int chosen = -1;
    while (chosen < 0) {
      for (int w = 0; w < Words; ++w) {
        rest.word[w] &= (rest.word[w] | ends.word[w]) - starts.word[w];
        const auto empty = active.word[w] & ~activeEnds(rest.word[w], w);
        if (empty) { chosen = 64 * w + __builtin_ctzll(empty); break; }
      }
    }
    const int w = chosen / 64;
    auto choices = live.word[w] & runMask[chosen];
    for (auto b = choices; b; b &= b - 1) {
      const int i = 64 * w + __builtin_ctzll(b);
      const auto [r,c] = cells[i];
      placed[r] |= 1u << c;
      visit(live.without(attacks[i]), needed - 1);
      placed[r] &= ~(1u << c);
    }
    if (capacity > needed) {
      live.word[w] &= ~choices;
      visit(live, needed);
    }
  }

  Results run() {
    for (target = matchingBound(); target >= 0; --target) {
      visit(all, target);
      if (!results.empty()) break;
    }
    return std::move(results);
  }
};

}  // namespace

std::vector<std::vector<std::pair<int, int>>> solveWalled(int n, const std::vector<std::string>& grid) {
  if (n < 0 || n > kMaxN || static_cast<int>(grid.size()) != n) return {};
  int freeCount = 0;
  for (const auto& row : grid) {
    if (static_cast<int>(row.size()) != n) return {};
    freeCount += std::count(row.begin(), row.end(), '.');
  }
  if (!freeCount) return {Board{}};
  if (n >= 4 && freeCount == n * n) return PlainSearch{n, {}, {}}.run();
  if (n * n - freeCount == 1) {
    // On a single-wall board, prefer an axis where the wall ends a row.
    int r = 0;
    while (grid[r].find('W') == std::string::npos) ++r;
    int c = static_cast<int>(grid[r].find('W'));
    const bool swapAxes = (r == 0 || r == n - 1) && c > 0 && c + 1 < n;
    RowSearch rows(n, grid, swapAxes);
    if (snapshotsEnabled()) rows.run<true>();
    else rows.run<false>();
    return std::move(rows.results);
  }
  // Put each segment wholly inside a word. Count padding before choosing the
  // bitboard width; at most 144 cells and 11 padding bits per word fit in three.
  int runs[2]{}, slots[2]{};
  for (int axis = 0; axis < 2; ++axis)
    for (int line = 0; line < n; ++line) {
      int length = 0;
      for (int c = 0; c <= n; ++c) {
        if (c < n && (axis ? grid[c][line] : grid[line][c]) == '.') { ++length; continue; }
        if (!length) continue;
        ++runs[axis];
        if (slots[axis] % 64 + length > 64) slots[axis] += 64 - slots[axis] % 64;
        slots[axis] += length;
        length = 0;
      }
    }
  const bool swapAxes = runs[1] < runs[0];
  const int size = slots[swapAxes];
  if (size <= 64) return Search<1>(n, size, swapAxes, grid).run();
  if (size <= 128) return Search<2>(n, size, swapAxes, grid).run();
  return Search<3>(n, size, swapAxes, grid).run();
}
