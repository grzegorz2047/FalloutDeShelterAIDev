#include "dwellers/WorkAssignment.hpp"

#include <cassert>

using namespace deep_shelter::dwellers;

int main() {
    DwellerService dwellers({{0, 100, 300}, 3});

    Dweller mara;
    mara.id = 1;
    mara.name = "Mara";
    mara.base_special.values = {{8, 3, 4, 2, 5, 6, 7}};
    mara.outfit_bonus.values = {{2, 0, 0, 0, 0, 0, 0}};
    assert(dwellers.add(mara));

    Dweller ivo;
    ivo.id = 2;
    ivo.name = "Ivo";
    ivo.base_special.values = {{2, 9, 4, 5, 3, 4, 6}};
    assert(dwellers.add(ivo));

    WorkAssignmentService work(dwellers);
    assert(work.add_room({10, 1, 0, true, {}}));
    assert(work.add_room({20, 2, 1, true, {}}));
    assert(!work.add_room({10, 1, 0, true, {}}));

    const auto preview = work.preview(1, 10);
    assert(preview.error == AssignmentError::None);
    assert(preview.target_efficiency > 0);
    assert(preview.difference == preview.target_efficiency);
    assert(work.suggest_room(1).value() == 10);
    assert(work.suggest_room(2).value() == 20);

    assert(work.move(1, 10, 100, 2) == AssignmentError::None);
    assert(dwellers.find(1)->status == ActivityStatus::InTransit);
    assert(work.group_efficiency(10) == 0);
    assert(work.move(1, 20, 100, 3) == AssignmentError::AlreadyInTransit);
    assert(work.move(2, 10, 100, 4) == AssignmentError::RoomFull);
    work.advance(99);
    assert(dwellers.find(1)->room_id == 0);
    work.advance(100);
    assert(dwellers.find(1)->room_id == 10);
    assert(dwellers.find(1)->status == ActivityStatus::Working);
    assert(work.group_efficiency(10) == work.efficiency(1, 10));

    const int base_before = dwellers.find(1)->base_special.values[1];
    const int old_target = work.preview(1, 20).target_efficiency;
    dwellers.find(1)->outfit_bonus.values[1] = 3;
    assert(work.preview(1, 20).target_efficiency > old_target);
    assert(dwellers.find(1)->base_special.values[1] == base_before);

    assert(work.move(1, 20, 110, 5) == AssignmentError::None);
    assert(work.group_efficiency(10) == 0);
    assert(work.remove_room(10, 105));
    assert(dwellers.find(1)->status == ActivityStatus::Idle);
    assert(dwellers.find(1)->room_id == 0);
    assert(work.transit_queue().empty());

    assert(work.move(1, 20, 120, 6) == AssignmentError::None);
    work.advance(120);
    assert(dwellers.find(1)->room_id == 20);

    const auto before = dwellers.find(1)->happiness;
    assert(work.apply_happiness(1, 20, "good_assignment", 130));
    assert(dwellers.find(1)->happiness == before + 20);
    assert(work.happiness_log().size() == 1);
    assert(work.happiness_log()[0].reason == "good_assignment");

    assert(work.move(2, 20, 140, 7) == AssignmentError::None);
    work.cancel_for_dweller(2, 135, "incident");
    assert(dwellers.find(2)->status == ActivityStatus::Idle);
    assert(dwellers.find(2)->room_id == 0);
    assert(work.transit_queue().empty());

    assert(work.remove_room(20, 150));
    assert(dwellers.find(1)->room_id == 0);
    assert(dwellers.find(1)->status == ActivityStatus::Idle);
    return 0;
}
