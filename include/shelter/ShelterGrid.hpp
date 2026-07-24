#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace deep_shelter::shelter {

enum class CellType : std::uint8_t { Rock, Empty, Room, Elevator, Blocked };

struct CellPosition {
    int column = 0;
    int floor = 0;
    bool operator==(const CellPosition& other) const noexcept {
        return column == other.column && floor == other.floor;
    }
};

struct BuildPreview {
    bool valid = false;
    int cost = 0;
    const char* reason = "";
};

struct PathResult {
    bool found = false;
    bool work_limit_reached = false;
    std::vector<CellPosition> cells;
};

class ShelterGrid {
public:
    ShelterGrid(int columns, int floors, int initial_credits);

    [[nodiscard]] int columns() const noexcept;
    [[nodiscard]] int floors() const noexcept;
    [[nodiscard]] int credits() const noexcept;
    [[nodiscard]] std::optional<CellType> cell(CellPosition position) const noexcept;

    [[nodiscard]] BuildPreview preview_excavate(CellPosition position) const noexcept;
    [[nodiscard]] BuildPreview preview_build(CellPosition position, int width, CellType type, int cost) const noexcept;
    bool confirm_excavate(CellPosition position) noexcept;
    bool confirm_build(CellPosition position, int width, CellType type, int cost) noexcept;
    bool demolish(CellPosition position, bool allow_disconnect = false) noexcept;

    [[nodiscard]] PathResult find_path(CellPosition start,
                                       CellPosition goal,
                                       std::size_t work_limit = 4096) const;
    [[nodiscard]] bool layout_valid() const noexcept;

private:
    [[nodiscard]] bool in_bounds(CellPosition position) const noexcept;
    [[nodiscard]] std::size_t index(CellPosition position) const noexcept;
    [[nodiscard]] bool traversable(CellPosition position) const noexcept;
    [[nodiscard]] bool connected_without(CellPosition removed) const noexcept;

    int columns_;
    int floors_;
    int credits_;
    std::vector<CellType> cells_;
};

}  // namespace deep_shelter::shelter
