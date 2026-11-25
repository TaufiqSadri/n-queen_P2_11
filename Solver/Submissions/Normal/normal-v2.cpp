#include "nqueens.h"

#include <array>
#include <cstdint>

namespace {

using Mask = std::uint32_t;
constexpr int kMinSize = 4;
constexpr int kMaxSize = 12;
// Used only to reserve capacity. The search computes every returned board.
constexpr std::array<int, 9> kSolutionCounts = {2, 10, 4, 40, 92, 352, 724, 2680, 14200};

template <int N>
struct Search {
  using Board = std::array<int, N>;
  static constexpr int kLast = N - 1;
  static constexpr Mask kFull = (Mask{1} << N) - 1;
  static constexpr Mask kSideColumns = 1 | (Mask{1} << kLast);

  Board columnsByRow{};
  std::array<Mask, N> allowedByRow{};
  std::vector<std::vector<int>> solutions;
  int lastSideRow = 0;

  void emitOrbit() {
    // Compare with the rotations and their mirrors. Keep the smallest board
    // in each group so it is emitted only once.
    std::array<Board, 4> rotations;
    int rotationCount = 0;
    Board rotated = columnsByRow;
    do {
      if (rotated < columnsByRow) {
        return;
      }
      for (int row = 0; row < N; ++row) {
        const int mirroredColumn = kLast - rotated[row];
        if (mirroredColumn < columnsByRow[row]) {
          return;
        }
        if (mirroredColumn > columnsByRow[row]) {
          break;
        }
      }
      rotations[rotationCount++] = rotated;

      Board next;
      for (int row = 0; row < N; ++row) {
        next[rotated[row]] = kLast - row;
      }
      rotated = next;
    } while (rotated != columnsByRow);

    // A valid board with N > 1 has no reflection symmetry. Only rotations
    // can repeat, so every distinct rotation yields two distinct solutions.
    for (int i = 0; i < rotationCount; ++i) {
      const Board& board = rotations[i];
      solutions.emplace_back(board.begin(), board.end());
      auto& mirror = solutions.emplace_back(N);
      for (int row = 0; row < N; ++row) {
        mirror[row] = kLast - board[row];
      }
    }
  }

  Mask candidates(int row, Mask columns, Mask left, Mask right) const {
    Mask available = allowedByRow[row] & ~(columns | left | right);
    if (row == lastSideRow) {
      // This is the last row allowed to use either side column.
      const Mask missingSides = kSideColumns & ~columns;
      if (missingSides == kSideColumns) {
        return 0;
      }
      if (missingSides != 0) {
        available &= missingSides;
      }
    }
    return available;
  }

  template <bool InlineNextRow>
  __attribute__((always_inline)) inline void visit(int row, Mask columns, Mask left,
                                                   Mask right, Mask available) {
    if (row == kLast) {
      // The caller checked available, and only one column can remain unused.
      columnsByRow[row] = __builtin_ctz(available);
      emitOrbit();
      return;
    }

    while (available != 0) {
      const Mask queen = available & -available;
      available ^= queen;
      const Mask nextColumns = columns | queen;
      const Mask nextLeft = (left | queen) << 1;
      const Mask nextRight = (right | queen) >> 1;
      const Mask nextAvailable = candidates(row + 1, nextColumns, nextLeft, nextRight);
      if (nextAvailable != 0) {
        columnsByRow[row] = __builtin_ctz(queen);
        if constexpr (InlineNextRow) {
          visit<false>(row + 1, nextColumns, nextLeft, nextRight, nextAvailable);
        } else {
          visitTwoRows(row + 1, nextColumns, nextLeft, nextRight, nextAvailable);
        }
      }
    }
  }

  // Expand two levels per recursive call to reduce call overhead.
  __attribute__((noinline)) void visitTwoRows(int row, Mask columns, Mask left,
                                             Mask right, Mask available) {
    visit<true>(row, columns, left, right, available);
  }

  void searchCorner() {
    allowedByRow.fill(kFull);
    lastSideRow = kLast;
    columnsByRow[0] = 0;

    const Mask topQueen = 1;
    Mask secondRow = candidates(1, topQueen, topQueen << 1, topQueen >> 1);
    while (secondRow != 0) {
      const Mask queen = secondRow & -secondRow;
      secondRow ^= queen;
      const int secondColumn = __builtin_ctz(queen);
      columnsByRow[1] = secondColumn;

      // Transpose leaves (0, 0) fixed. Keep the orientation whose second
      // row comes before the transposed second row (the row of column 1).
      // Second-row candidates increase, so the ban accumulates over rows
      // 2 through secondColumn. Rows 0 and 1 already hold a different queen.
      allowedByRow[secondColumn] &= ~(Mask{1} << 1);

      const Mask columns = topQueen | queen;
      const Mask left = ((topQueen << 1) | queen) << 1;
      const Mask right = queen >> 1;
      const Mask available = candidates(2, columns, left, right);
      if (available != 0) {
        visitTwoRows(2, columns, left, right, available);
      }
    }
  }

  std::vector<std::vector<int>> run() {
    solutions.reserve(kSolutionCounts[N - kMinSize]);
    searchCorner();
    for (int col = 1; col < N / 2; ++col) {
      // In the smallest orientation, the top queen is at least as close to
      // a corner as each of the other three edge queens.
      const Mask nearLeft = (Mask{1} << col) - 1;
      const Mask nearRight = nearLeft << (N - col);
      allowedByRow.fill(kFull);
      for (int row = 1; row < col; ++row) {
        allowedByRow[row] &= ~kSideColumns;
        allowedByRow[kLast - row] &= ~kSideColumns;
      }
      allowedByRow[kLast] &= ~(nearLeft | nearRight);
      lastSideRow = kLast - col;

      columnsByRow[0] = col;
      const Mask queen = Mask{1} << col;
      const Mask left = queen << 1;
      const Mask right = queen >> 1;
      visitTwoRows(1, queen, left, right, candidates(1, queen, left, right));
    }
    return std::move(solutions);
  }
};

}  // namespace

std::vector<std::vector<int>> solveNQueens(int n) {
  switch (n) {
    case 4: return Search<4>{}.run();
    case 5: return Search<5>{}.run();
    case 6: return Search<6>{}.run();
    case 7: return Search<7>{}.run();
    case 8: return Search<8>{}.run();
    case 9: return Search<9>{}.run();
    case 10: return Search<10>{}.run();
    case 11: return Search<11>{}.run();
    case 12: return Search<12>{}.run();
    default: return {};
  }
}
