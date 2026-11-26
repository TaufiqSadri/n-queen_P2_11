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
  static constexpr int kLast = N - 1;
  static constexpr Mask kFull = (Mask{1} << N) - 1;
  static constexpr Mask kSideColumns = 1 | (Mask{1} << kLast);

  std::array<std::uint8_t, N> columnsByRow{};
  std::array<std::uint16_t, N> allowedByRow{};
  std::vector<std::vector<int>> solutions;
  int lastSideRow = 0;

  // Four bits per row, with row 0 in the most significant used nibble.
  // Integer comparison then matches lexicographic order of the columns.
  using PackedBoard = std::uint64_t;
  static constexpr PackedBoard kMirrorMask =
      (0x111111111111ULL >> (4 * (kMaxSize - N))) * kLast;

  static PackedBoard reverseRows(PackedBoard key) {
    key = ((key & 0x0f0f0f0f0f0f0f0fULL) << 4) | ((key >> 4) & 0x0f0f0f0f0f0f0f0fULL);
    key = ((key & 0x00ff00ff00ff00ffULL) << 8) | ((key >> 8) & 0x00ff00ff00ff00ffULL);
    key = ((key & 0x0000ffff0000ffffULL) << 16) | ((key >> 16) & 0x0000ffff0000ffffULL);
    return ((key << 32) | (key >> 32)) >> (64 - 4 * N);
  }

  void emitPair(PackedBoard key) {
    std::array<int, N> unpacked;
    for (int r = N - 1; r >= 0; --r) {
      unpacked[r] = static_cast<int>(key & 15);
      key >>= 4;
    }
    solutions.emplace_back(unpacked.begin(), unpacked.end());
    for (auto& col : unpacked) col = kLast - col;
    solutions.emplace_back(unpacked.begin(), unpacked.end());
  }

  void emitOrbit() {
    PackedBoard original = 0;
    PackedBoard transpose = 0;
    for (int r = 0; r < N; ++r) {
      const int col = columnsByRow[r];
      original = (original << 4) | static_cast<PackedBoard>(col);
      transpose |= static_cast<PackedBoard>(r) << (4 * (kLast - col));
    }

    // The corner and its transpose were already separated by searchCorner.
    // A board with a corner queen cannot have a second corner queen.
    // Each digit is <= kLast, so subtracting reflects all columns without a
    // borrow between digits. Reversing the digits reflects all rows.
    const PackedBoard rotated = kMirrorMask - transpose;
    const PackedBoard upsideDown = reverseRows(original);
    const PackedBoard halfTurn = kMirrorMask - upsideDown;
    const PackedBoard threeQuarterTurn = reverseRows(transpose);
    if (columnsByRow[0] != 0 &&
        (rotated < original || transpose < original ||
         upsideDown < original || halfTurn < original ||
         threeQuarterTurn < original || kMirrorMask - threeQuarterTurn < original)) {
      return;
    }
    // The rotation period is 1, 2, or 4. Each distinct rotation has a
    // distinct reflection for a valid board with N > 1.
    emitPair(original);
    if (rotated == original) return;
    emitPair(rotated);
    if (halfTurn == original) return;
    emitPair(halfTurn);
    emitPair(threeQuarterTurn);
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

  template <int Row>
  void visit(Mask columns, Mask left, Mask right, Mask available) {
    if constexpr (Row + 1 == kLast) {
      // Both unused columns must be placed. Adjacent columns would attack
      // diagonally in either assignment to these two adjacent rows.
      const Mask remaining = kFull ^ columns;
      if ((remaining & (remaining << 1)) != 0) {
        return;
      }
      const Mask final = allowedByRow[kLast] & remaining & ~((left << 1) | (right >> 1));
      if (final == 0) {
        return;
      }
      if (final != remaining) {
        available &= remaining ^ final;
      }
      while (available != 0) {
        const Mask queen = available & -available;
        available ^= queen;
        columnsByRow[Row] = __builtin_ctz(queen);
        columnsByRow[kLast] = __builtin_ctz(remaining ^ queen);
        emitOrbit();
      }
    } else {
      while (available != 0) {
        const Mask queen = available & -available;
        available ^= queen;
        const Mask nextColumns = columns | queen;
        const Mask nextLeft = (left | queen) << 1;
        const Mask nextRight = (right | queen) >> 1;
        const Mask nextAvailable = candidates(Row + 1, nextColumns, nextLeft, nextRight);
        if (nextAvailable != 0) {
          columnsByRow[Row] = __builtin_ctz(queen);
          visit<Row + 1>(nextColumns, nextLeft, nextRight, nextAvailable);
        }
      }
    }
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
        visit<2>(columns, left, right, available);
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
      visit<1>(queen, left, right, candidates(1, queen, left, right));
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
