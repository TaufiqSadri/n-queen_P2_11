#include "nqueens.h"

#include <algorithm>
#include <array>
#include <cstdint>

namespace {

constexpr int kMaxN = 12;
constexpr int kMaxCells = kMaxN * kMaxN;
using Cell = std::pair<int,int>;
using Board = std::vector<Cell>;
using Domains = std::array<std::uint16_t,kMaxCells>;

struct Segments {
    std::vector<Cell> cells;
    std::vector<std::array<int,4>> id;
    std::array<std::vector<std::vector<int>>, 2> groups;
    int total = 0;

    explicit Segments(const std::vector<std::string>& board) {
        int n = static_cast<int>(board.size());
        std::vector<int> index(n*n,-1);
        // Each predecessor is either in the previous row or one cell left.
        // Thus every predecessor has already been visited in row-major order,
        // including the upper-right predecessor for a descending-left ray.
        constexpr int dr[] = {0,-1,-1,-1};
        constexpr int dc[] = {-1,0,-1,1};
        for (int r = 0; r < n; ++r) {
            for (int c = 0; c < n; ++c) {
                if (board[r][c] == 'W') continue;
                int cell = static_cast<int>(cells.size());
                cells.emplace_back(r,c);
                index[r*n+c] = cell;
                std::array<int,4> ids{};
                for (int d = 0; d < 4; ++d) {
                    int y = r+dr[d], x = c+dc[d];
                    int prev = y >= 0 && x >= 0 && x < n ? index[y*n+x] : -1;
                    // A wall has index -1, just like a missing predecessor.
                    // Start a fresh segment at either boundary. A diagonal
                    // checks only its predecessor, never the two corner cells.
                    if (prev < 0) {
                        ids[d] = total++;
                    } else {
                        ids[d] = id[prev][d];
                    }
                }
                id.push_back(ids);

            }
        }
        for (int axis = 0; axis < 2; ++axis) {
            std::vector<int> groupIndex(total,-1);
            for (int i = 0; i < static_cast<int>(cells.size()); ++i) {
                int& g = groupIndex[id[i][axis]];
                if (g < 0) {
                    g = static_cast<int>(groups[axis].size());
                    groups[axis].emplace_back();
                }
                groups[axis][g].push_back(i);
            }
        }
        // IDs are global across all four directions. Two distinct cells
        // conflict exactly when one of their four segment IDs matches.
    }
};

struct Search {
    const Segments& s;
    const std::vector<std::vector<int>>& groups;
    std::array<Domains,kMaxCells> blocked{};
    std::array<Cell,kMaxCells> path{};
    std::vector<Board> results;

    explicit Search(const Segments& segments)
        : s(segments), groups(s.groups[s.groups[1].size() < s.groups[0].size() ? 1 : 0]) {
        // Each mask addresses only one run (at most 12 cells). Segment IDs
        // themselves are unrestricted by the width of a machine word.
        for (int i = 0; i < static_cast<int>(s.cells.size()); ++i) {
            for (std::size_t g = 0; g < groups.size(); ++g) {
                for (std::size_t j = 0; j < groups[g].size(); ++j) {
                    const int other = groups[g][j];
                    for (int d = 0; d < 4; ++d)
                        if (s.id[i][d] == s.id[other][d])
                            blocked[i][g] |= 1u << j;
                }
            }
        }
    }

    void visit(Domains& domains, int depth, int needed) {
        if (needed == 0) {
            auto& board = results.emplace_back(path.begin(),path.begin()+depth);
            std::sort(board.begin(),board.end());
            NQ_SNAPSHOT(board, "maximum board");
            return;
        }
        int live = 0, chosen = -1, smallest = kMaxN+1;
        for (int g = 0; g < static_cast<int>(groups.size()); ++g) {
            if (!domains[g]) continue;
            ++live;
            const int size = __builtin_popcount(domains[g]);
            if (size < smallest) {
                chosen = g;
                smallest = size;
            }
        }
        if (live < needed) return;
        // A solution uses one candidate of the chosen run or skips the run.
        // These branches are disjoint, even if later branching order changes.
        unsigned choices = domains[chosen];
        domains[chosen] = 0;
        while (choices) {
            const int i = groups[chosen][__builtin_ctz(choices)];
            choices &= choices-1;
            path[depth] = s.cells[i];
            Domains next;
            for (std::size_t g = 0; g < groups.size(); ++g)
                next[g] = domains[g] & ~blocked[i][g];
            visit(next,depth+1,needed-1);
        }
        if (live > needed) visit(domains,depth,needed);
    }

    void run() {
        Domains initial{};
        for (std::size_t g = 0; g < groups.size(); ++g)
            initial[g] = (1u << groups[g].size())-1;
        // The first feasible cardinality is optimal. Finish that entire
        // search to retain ties. Target zero emits exactly one empty board.
        for (int target = static_cast<int>(groups.size()); target >= 0; --target) {
            Domains domains = initial;
            visit(domains,0,target);
            if (!results.empty()) break;
        }
    }
};

}  // namespace

std::vector<std::vector<std::pair<int,int>>> solveWalled(int n, const std::vector<std::string>& grid) {
    if (n < 0 || n > kMaxN || static_cast<int>(grid.size()) != n) return {};
    for (const auto& row : grid) if (static_cast<int>(row.size()) != n) return {};
    Segments segments(grid);
    Search search(segments);
    search.run();
    return std::move(search.results);
}
