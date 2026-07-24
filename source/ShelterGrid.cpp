#include "shelter/ShelterGrid.hpp"

#include <algorithm>
#include <queue>

namespace deep_shelter::shelter {
namespace {
constexpr int kExcavateCost = 25;
}

ShelterGrid::ShelterGrid(int columns, int floors, int initial_credits)
    : columns_(std::max(1, columns)),
      floors_(std::max(1, floors)),
      credits_(std::max(0, initial_credits)),
      cells_(static_cast<std::size_t>(columns_) * static_cast<std::size_t>(floors_), CellType::Rock) {
    for (int column = 0; column < columns_; ++column) {
        cells_[index({column, 0})] = CellType::Empty;
    }
    cells_[index({0, 0})] = CellType::Elevator;
}

int ShelterGrid::columns() const noexcept { return columns_; }
int ShelterGrid::floors() const noexcept { return floors_; }
int ShelterGrid::credits() const noexcept { return credits_; }

bool ShelterGrid::in_bounds(CellPosition position) const noexcept {
    return position.column >= 0 && position.floor >= 0 &&
           position.column < columns_ && position.floor < floors_;
}

std::size_t ShelterGrid::index(CellPosition position) const noexcept {
    return static_cast<std::size_t>(position.floor) * static_cast<std::size_t>(columns_) +
           static_cast<std::size_t>(position.column);
}

std::optional<CellType> ShelterGrid::cell(CellPosition position) const noexcept {
    if (!in_bounds(position)) return std::nullopt;
    return cells_[index(position)];
}

BuildPreview ShelterGrid::preview_excavate(CellPosition position) const noexcept {
    if (!in_bounds(position)) return {false, 0, "outside map"};
    if (cells_[index(position)] != CellType::Rock) return {false, 0, "cell is not rock"};
    if (credits_ < kExcavateCost) return {false, kExcavateCost, "not enough credits"};

    const CellPosition neighbours[] = {
        {position.column - 1, position.floor},
        {position.column + 1, position.floor},
        {position.column, position.floor - 1},
        {position.column, position.floor + 1},
    };
    for (const auto neighbour : neighbours) {
        if (in_bounds(neighbour) && cells_[index(neighbour)] != CellType::Rock &&
            cells_[index(neighbour)] != CellType::Blocked) {
            return {true, kExcavateCost, ""};
        }
    }
    return {false, kExcavateCost, "excavation must touch accessible shelter"};
}

BuildPreview ShelterGrid::preview_build(CellPosition position, int width, CellType type, int cost) const noexcept {
    if (width <= 0 || width > 3) return {false, 0, "width must be 1 to 3"};
    if (cost < 0) return {false, 0, "cost must be non-negative"};
    if (type != CellType::Room && type != CellType::Elevator) return {false, cost, "invalid build type"};
    if (position.column < 0 || position.floor < 0 || position.floor >= floors_ ||
        position.column + width > columns_) {
        return {false, cost, "outside map"};
    }
    if (credits_ < cost) return {false, cost, "not enough credits"};
    for (int offset = 0; offset < width; ++offset) {
        if (cells_[index({position.column + offset, position.floor})] != CellType::Empty) {
            return {false, cost, "cell is occupied"};
        }
    }
    if (type == CellType::Elevator && width != 1) return {false, cost, "elevator width must be one"};
    return {true, cost, ""};
}

bool ShelterGrid::confirm_excavate(CellPosition position) noexcept {
    const auto preview = preview_excavate(position);
    if (!preview.valid) return false;
    cells_[index(position)] = CellType::Empty;
    credits_ -= preview.cost;
    return true;
}

bool ShelterGrid::confirm_build(CellPosition position, int width, CellType type, int cost) noexcept {
    const auto preview = preview_build(position, width, type, cost);
    if (!preview.valid) return false;
    for (int offset = 0; offset < width; ++offset) {
        cells_[index({position.column + offset, position.floor})] = type;
    }
    credits_ -= cost;
    return true;
}

bool ShelterGrid::traversable(CellPosition position) const noexcept {
    if (!in_bounds(position)) return false;
    const auto value = cells_[index(position)];
    return value == CellType::Empty || value == CellType::Room || value == CellType::Elevator;
}

PathResult ShelterGrid::find_path(CellPosition start, CellPosition goal, std::size_t work_limit) const {
    PathResult result;
    if (!traversable(start) || !traversable(goal) || work_limit == 0) return result;

    const std::size_t total = cells_.size();
    std::vector<int> previous(total, -1);
    std::vector<bool> visited(total, false);
    std::queue<CellPosition> frontier;
    frontier.push(start);
    visited[index(start)] = true;
    std::size_t work = 0;

    while (!frontier.empty()) {
        if (++work > work_limit) {
            result.work_limit_reached = true;
            return result;
        }
        const auto current = frontier.front();
        frontier.pop();
        if (current == goal) break;

        const CellPosition neighbours[] = {
            {current.column - 1, current.floor},
            {current.column + 1, current.floor},
            {current.column, current.floor - 1},
            {current.column, current.floor + 1},
        };
        for (const auto neighbour : neighbours) {
            if (!traversable(neighbour)) continue;
            if (neighbour.floor != current.floor &&
                (cells_[index(current)] != CellType::Elevator ||
                 cells_[index(neighbour)] != CellType::Elevator)) {
                continue;
            }
            const auto neighbour_index = index(neighbour);
            if (visited[neighbour_index]) continue;
            visited[neighbour_index] = true;
            previous[neighbour_index] = static_cast<int>(index(current));
            frontier.push(neighbour);
        }
    }

    if (!visited[index(goal)]) return result;
    for (int cursor = static_cast<int>(index(goal)); cursor >= 0; cursor = previous[static_cast<std::size_t>(cursor)]) {
        result.cells.push_back({cursor % columns_, cursor / columns_});
        if (cursor == static_cast<int>(index(start))) break;
    }
    std::reverse(result.cells.begin(), result.cells.end());
    result.found = true;
    return result;
}

bool ShelterGrid::connected_without(CellPosition removed) const noexcept {
    std::optional<CellPosition> first;
    std::size_t traversable_count = 0;
    for (int floor = 0; floor < floors_; ++floor) {
        for (int column = 0; column < columns_; ++column) {
            const CellPosition position{column, floor};
            if (position == removed || !traversable(position)) continue;
            if (!first) first = position;
            ++traversable_count;
        }
    }
    if (!first || traversable_count <= 1) return true;
    const auto path_limit = cells_.size() * cells_.size();
    for (int floor = 0; floor < floors_; ++floor) {
        for (int column = 0; column < columns_; ++column) {
            const CellPosition target{column, floor};
            if (target == removed || !traversable(target)) continue;
            // Temporarily removed cells cannot be traversed by find_path, so use a local BFS clone.
            std::vector<bool> visited(cells_.size(), false);
            std::queue<CellPosition> frontier;
            frontier.push(*first);
            visited[index(*first)] = true;
            std::size_t work = 0;
            while (!frontier.empty() && work++ < path_limit) {
                const auto current = frontier.front();
                frontier.pop();
                const CellPosition neighbours[] = {{current.column - 1, current.floor}, {current.column + 1, current.floor},
                                                   {current.column, current.floor - 1}, {current.column, current.floor + 1}};
                for (const auto neighbour : neighbours) {
                    if (neighbour == removed || !traversable(neighbour)) continue;
                    if (neighbour.floor != current.floor &&
                        (cells_[index(current)] != CellType::Elevator || cells_[index(neighbour)] != CellType::Elevator)) continue;
                    if (!visited[index(neighbour)]) {
                        visited[index(neighbour)] = true;
                        frontier.push(neighbour);
                    }
                }
            }
            if (!visited[index(target)]) return false;
        }
    }
    return true;
}

bool ShelterGrid::demolish(CellPosition position, bool allow_disconnect) noexcept {
    if (!in_bounds(position)) return false;
    const auto current = cells_[index(position)];
    if (current != CellType::Room && current != CellType::Elevator) return false;
    if (!allow_disconnect && !connected_without(position)) return false;
    cells_[index(position)] = CellType::Empty;
    return true;
}

bool ShelterGrid::layout_valid() const noexcept {
    if (cells_.size() != static_cast<std::size_t>(columns_) * static_cast<std::size_t>(floors_)) return false;
    for (const auto value : cells_) {
        if (value > CellType::Blocked) return false;
    }
    return true;
}

}  // namespace deep_shelter::shelter
