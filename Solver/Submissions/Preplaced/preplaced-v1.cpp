#include "nqueens.h"

#include <array>
#include <cstdint>

namespace {

using Mask = std::uint32_t;
constexpr int kMinSize = 4;
constexpr int kMaxSize = 12;
// Exact output capacity for an empty board. All placements are generated at run time.
constexpr std::array<int, 9> kNormalCounts = {2, 10, 4, 40, 92, 352, 724, 2680, 14200};

struct OpenRow {
  int index;
  int nextDistance;
  Mask allowed;
};

struct Search {
  int n;
  std::array<int, kMaxSize> board{};
  std::array<OpenRow, kMaxSize> openRows{};
  std::vector<std::vector<int>> solutions;

  template <bool Reflect>
  void visit(const OpenRow* row, Mask columns, Mask left, Mask right, Mask available) {
    if (row->nextDistance == 0) {
      // Every other column is already occupied, and available is nonzero.
      board[row->index] = __builtin_ctz(available);
      solutions.emplace_back(board.begin(), board.begin() + n);
      if constexpr (Reflect) {
        auto& reflected = solutions.emplace_back(n);
        for (int r = 0; r < n; ++r) {
          reflected[r] = n - 1 - board[r];
        }
      }
      return;
    }

    while (available != 0) {
      const Mask queen = available & -available;
      available ^= queen;
      const Mask nextColumns = columns | queen;
      const int distance = row->nextDistance;
      const Mask nextLeft = (left | queen) << distance;
      const Mask nextRight = (right | queen) >> distance;
      // The next row's mask already excludes attacks from every locked queen.
      const Mask nextAvailable = row[1].allowed & ~(nextColumns | nextLeft | nextRight);
      if (nextAvailable != 0) {
        board[row->index] = __builtin_ctz(queen);
        visit<Reflect>(row + 1, nextColumns, nextLeft, nextRight, nextAvailable);
      }
    }
  }

  void solveEmptyBoard() {
    solutions.reserve(kNormalCounts[n - kMinSize]);
    const Mask leftHalf = (Mask{1} << (n / 2)) - 1;
    visit<true>(openRows.data(), 0, 0, 0, leftHalf);

    if (n % 2 != 0) {
      // With the first queen in the center, the second queen breaks the mirror tie.
      const Mask center = Mask{1} << (n / 2);
      board[0] = n / 2;
      const Mask secondRow = leftHalf & ~(center >> 1);
      visit<true>(openRows.data() + 1, center, center << 1, center >> 1, secondRow);
    }
  }
};

}  // namespace

std::vector<std::vector<int>> solvePreplaced(
    int n, const std::vector<std::pair<int, int>>& preplaced) {
  if (n < kMinSize || n > kMaxSize) {
    return {};
  }

  Search search{n, {}, {}, {}};
  Mask fixedRows = 0;
  Mask fixedColumns = 0;
  Mask sumDiagonals = 0;
  Mask diffDiagonals = 0;
  for (const auto& [r, c] : preplaced) {
    if (r < 0 || r >= n || c < 0 || c >= n) {
      return {};
    }
    const Mask row = Mask{1} << r;
    const Mask col = Mask{1} << c;
    if ((fixedRows & row) != 0) {
      if (search.board[r] != c) {
        return {};
      }
      continue;
    }
    const Mask sum = col << r;
    const Mask diff = col << (n - 1 - r);
    if ((fixedColumns & col) || (sumDiagonals & sum) || (diffDiagonals & diff)) {
      return {};
    }
    fixedRows |= row;
    fixedColumns |= col;
    sumDiagonals |= sum;
    diffDiagonals |= diff;
    search.board[r] = c;
  }

  const Mask full = (Mask{1} << n) - 1;
  int openCount = 0;
  for (int r = 0; r < n; ++r) {
    if ((fixedRows & (Mask{1} << r)) != 0) {
      continue;
    }
    // Project the two absolute diagonal masks onto this row's columns.
    const Mask blocked = fixedColumns | (sumDiagonals >> r) | (diffDiagonals >> (n - 1 - r));
    const Mask allowed = full & ~blocked;
    if (allowed == 0) {
      return {};
    }
    if (openCount != 0) {
      auto& previous = search.openRows[openCount - 1];
      previous.nextDistance = r - previous.index;
    }
    search.openRows[openCount++] = {r, 0, allowed};
  }

  if (openCount == 0) {
    search.solutions.emplace_back(search.board.begin(), search.board.begin() + n);
  } else if (fixedRows == 0) {
    search.solveEmptyBoard();
  } else {
    search.visit<false>(search.openRows.data(), 0, 0, 0, search.openRows[0].allowed);
  }
  return std::move(search.solutions);
}
