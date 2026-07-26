from pathlib import Path
p=Path('source/PlayableShelterSession.cpp')
s=p.read_text()
old='''PlayableRoomLifecyclePreview
PlayableShelterSession::preview_demolish_selected() const noexcept {
    PlayableRoomLifecyclePreview preview;
    const int selected_index = state_.selected_room;
    const auto& selected = state_.room_entries[
        static_cast<std::size_t>(selected_index)];
    if (!selected.active) return preview;
    preview.group_width = selected_group_width();
    preview.credit_delta = demolition_refund(selected);
    preview.stored_units_affected = selected.stored;
    preview.production_steps_affected = selected.production_steps;
    for (const auto& resident : state_.residents) {
        if (resident.active && resident.assigned_room == selected_index) {
            ++preview.residents_affected;
        }
    }
    if (state_.rooms <= 1) {
        preview.result = RoomLifecycleResult::LastRoom;
    } else if (preview.residents_affected > 0) {
        preview.result = RoomLifecycleResult::UnsafeResidents;
    } else if (preview.stored_units_affected > 0) {
        preview.result = RoomLifecycleResult::UnsafeStoredResources;
    } else if (preview.production_steps_affected > 0) {
        preview.result = RoomLifecycleResult::UnsafeProduction;
    } else {
        preview.result = RoomLifecycleResult::Applied;
    }
    return preview;
}

RoomLifecycleResult
PlayableShelterSession::confirm_demolish_selected() noexcept {
    const auto preview = preview_demolish_selected();
    if (!preview.allowed()) return preview.result;
    const int removed = state_.selected_room;
    state_.credits += preview.credit_delta;
    state_.room_entries[static_cast<std::size_t>(removed)] = {};
    for (auto& resident : state_.residents) {
        if (resident.assigned_room == removed) {
            resident.assigned_room = -1;
            resident.state = PlayableResidentState::Roaming;
        }
    }
    normalize_room_groups(state_);
    int replacement = -1;
    for (int index = removed - 1; index >= 0; --index) {
        if (state_.room_entries[static_cast<std::size_t>(index)].active) {
            replacement = index;
            break;
        }
    }
    if (replacement < 0) {
        for (int index = removed + 1; index < kPlayableRoomCapacity; ++index) {
            if (state_.room_entries[static_cast<std::size_t>(index)].active) {
                replacement = index;
                break;
            }
        }
    }
    state_.selected_room = replacement;
    sync_legacy_view();
    return RoomLifecycleResult::Applied;
}
'''
new='''PlayableRoomLifecyclePreview
PlayableShelterSession::preview_demolish_selected() const noexcept {
    PlayableRoomLifecyclePreview preview;
    const int selected_index = state_.selected_room;
    const auto& selected = state_.room_entries[
        static_cast<std::size_t>(selected_index)];
    if (!selected.active) return preview;
    preview.group_width = selected_group_width();
    preview.credit_delta = demolition_refund(selected);
    preview.stored_units_affected = selected.stored;
    preview.production_steps_affected = selected.production_steps;

    PlayableShelterState candidate = state_;
    candidate.room_entries[static_cast<std::size_t>(selected_index)] = {};
    normalize_room_groups(candidate);
    for (const auto& resident : state_.residents) {
        if (!resident.active) continue;
        const bool touches_removed =
            resident.assigned_room == selected_index ||
            (resident.current_column == selected.column &&
             resident.current_floor == selected.floor) ||
            (resident.next_column == selected.column &&
             resident.next_floor == selected.floor) ||
            (resident.destination_column == selected.column &&
             resident.destination_floor == selected.floor);
        const bool current_survives = traversable_cell(
            candidate, resident.current_column, resident.current_floor);
        if (touches_removed || !current_survives) {
            ++preview.residents_affected;
        }
    }
    if (state_.rooms <= 1) {
        preview.result = RoomLifecycleResult::LastRoom;
    } else if (preview.stored_units_affected > 0) {
        preview.result = RoomLifecycleResult::UnsafeStoredResources;
    } else if (preview.production_steps_affected > 0) {
        preview.result = RoomLifecycleResult::UnsafeProduction;
    } else {
        preview.result = RoomLifecycleResult::Applied;
    }
    return preview;
}

RoomLifecycleResult
PlayableShelterSession::confirm_demolish_selected() noexcept {
    const auto preview = preview_demolish_selected();
    if (!preview.allowed()) return preview.result;
    const int removed = state_.selected_room;
    const PlayableRoomEntry removed_room = state_.room_entries[
        static_cast<std::size_t>(removed)];

    int replacement = -1;
    int replacement_distance = kPlayableGridColumns + kPlayableGridFloors + 1;
    std::uint64_t replacement_id = UINT64_MAX;
    for (int index = 0; index < kPlayableRoomCapacity; ++index) {
        const auto& room = state_.room_entries[static_cast<std::size_t>(index)];
        if (!room.active || index == removed) continue;
        const int distance = std::abs(room.column - removed_room.column) +
                             std::abs(room.floor - removed_room.floor);
        if (replacement < 0 || distance < replacement_distance ||
            (distance == replacement_distance && room.segment_id < replacement_id)) {
            replacement = index;
            replacement_distance = distance;
            replacement_id = room.segment_id;
        }
    }
    if (replacement < 0) return RoomLifecycleResult::LastRoom;
    const PlayableRoomEntry fallback = state_.room_entries[
        static_cast<std::size_t>(replacement)];

    state_.credits += preview.credit_delta;
    state_.room_entries[static_cast<std::size_t>(removed)] = {};
    normalize_room_groups(state_);
    for (auto& resident : state_.residents) {
        if (!resident.active) continue;
        const bool touches_removed =
            resident.assigned_room == removed ||
            (resident.current_column == removed_room.column &&
             resident.current_floor == removed_room.floor) ||
            (resident.next_column == removed_room.column &&
             resident.next_floor == removed_room.floor) ||
            (resident.destination_column == removed_room.column &&
             resident.destination_floor == removed_room.floor);
        const bool current_survives = traversable_cell(
            state_, resident.current_column, resident.current_floor);
        if (!touches_removed && current_survives) continue;

        if (!current_survives) {
            resident.current_column = fallback.column;
            resident.current_floor = fallback.floor;
        }
        resident.next_column = resident.current_column;
        resident.next_floor = resident.current_floor;
        resident.destination_column = fallback.column;
        resident.destination_floor = fallback.floor;
        resident.assigned_room = -1;
        resident.state = PlayableResidentState::Roaming;
        resident.movement_ticks = 0;
        resident.roaming_ticks = 0;
    }
    state_.selected_room = replacement;
    sync_legacy_view();
    return RoomLifecycleResult::Applied;
}
'''
if old not in s: raise SystemExit('demolition block missing')
s=s.replace(old,new,1)
p.write_text(s)

p=Path('tests/playable_shelter_session_tests.cpp')
t=p.read_text()
old='''    PlayableShelterState blocked_state = session.state();
    PlayableShelterSession blocked(blocked_state);
    const int blocked_room = blocked.room_index_at(2, 0);
    assert(blocked.assign_resident_to_room(0u, blocked_room));
    assert(blocked.preview_demolish_selected().result ==
           RoomLifecycleResult::UnsafeResidents);
    assert(blocked.confirm_demolish_selected() ==
           RoomLifecycleResult::UnsafeResidents);
    assert(blocked.room_index_at(2, 0) == blocked_room);
    assert(valid_playable_state(blocked.state()));
'''
new='''    PlayableShelterState evacuation_state = session.state();
    PlayableShelterSession evacuation(evacuation_state);
    const int evacuation_room = evacuation.room_index_at(2, 0);
    assert(evacuation_room >= 0);
    assert(evacuation.assign_resident_to_room(0u, evacuation_room));
    const auto evacuation_preview = evacuation.preview_demolish_selected();
    assert(evacuation_preview.allowed());
    assert(evacuation_preview.residents_affected == 1);
    assert(evacuation.confirm_demolish_selected() ==
           RoomLifecycleResult::Applied);
    assert(evacuation.room_index_at(2, 0) < 0);
    const auto& evacuated = evacuation.state().residents[0];
    assert(evacuated.active);
    assert(evacuated.assigned_room == -1);
    assert(evacuated.state == PlayableResidentState::Roaming);
    assert(evacuation.room_index_at(evacuated.current_column,
                                    evacuated.current_floor) >= 0);
    assert(valid_playable_state(evacuation.state()));

    PlayableShelterState blocked_state = session.state();
    const int blocked_room = blocked_state.selected_room;
    blocked_state.room_entries[static_cast<std::size_t>(blocked_room)].stored = 1;
    PlayableShelterSession blocked(blocked_state);
    assert(blocked.preview_demolish_selected().result ==
           RoomLifecycleResult::UnsafeStoredResources);
    assert(blocked.confirm_demolish_selected() ==
           RoomLifecycleResult::UnsafeStoredResources);
    assert(blocked.state().room_entries[static_cast<std::size_t>(blocked_room)].active);
    assert(blocked.state().room_entries[static_cast<std::size_t>(blocked_room)].stored == 1);
    assert(valid_playable_state(blocked.state()));
'''
if old not in t: raise SystemExit('test block missing')
t=t.replace(old,new,1)
p.write_text(t)
