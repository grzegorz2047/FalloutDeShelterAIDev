#include "shelter/ShelterGrid.hpp"

#include <cassert>

using namespace deep_shelter::shelter;

int main() {
    ShelterGrid grid(8, 4, 500);
    assert(grid.layout_valid());
    assert(grid.cell({0, 0}) == CellType::Elevator);
    assert(!grid.preview_excavate({8, 0}).valid);

    const int before_cancel = grid.credits();
    const auto preview = grid.preview_excavate({0, 1});
    assert(preview.valid && preview.cost == 25);
    assert(grid.credits() == before_cancel); // preview/cancel is side-effect free
    assert(grid.confirm_excavate({0, 1}));
    assert(grid.credits() == before_cancel - 25);
    assert(!grid.confirm_excavate({0, 1})); // double confirm cannot charge twice
    assert(grid.credits() == before_cancel - 25);

    assert(grid.preview_build({1, 0}, 2, CellType::Room, 100).valid);
    assert(grid.confirm_build({1, 0}, 2, CellType::Room, 100));
    assert(!grid.confirm_build({1, 0}, 2, CellType::Room, 100));
    assert(grid.credits() == before_cancel - 125);
    assert(!grid.preview_build({7, 0}, 2, CellType::Room, 10).valid);

    assert(grid.confirm_build({0, 1}, 1, CellType::Elevator, 50));
    const auto vertical = grid.find_path({0, 0}, {0, 1});
    assert(vertical.found && vertical.cells.size() == 2);

    assert(grid.confirm_excavate({1, 1}));
    assert(grid.confirm_build({1, 1}, 1, CellType::Room, 20));
    const auto path = grid.find_path({2, 0}, {1, 1});
    assert(path.found);
    assert(path.cells.front() == CellPosition{2, 0});
    assert(path.cells.back() == CellPosition{1, 1});

    const auto limited = grid.find_path({2, 0}, {1, 1}, 1);
    assert(!limited.found && limited.work_limit_reached);

    assert(!grid.demolish({0, 0})); // sole vertical connection is protected
    assert(grid.demolish({1, 1}));
    assert(grid.cell({1, 1}) == CellType::Empty);

    ShelterGrid poor(4, 2, 0);
    const auto no_money = poor.preview_excavate({0, 1});
    assert(!no_money.valid && no_money.cost == 25);
    assert(!poor.confirm_excavate({0, 1}));
    assert(poor.credits() == 0);
    return 0;
}
