#include "dwellers/WorkAssignment.hpp"

#include <algorithm>
#include <limits>

namespace deep_shelter::dwellers {

WorkAssignmentService::WorkAssignmentService(DwellerService& dwellers) : dwellers_(dwellers) {}

bool WorkAssignmentService::add_room(WorkRoom room) {
    if (room.id == 0 || room.capacity < 0 || room.preferred_special_index < 0 ||
        room.preferred_special_index >= 7 || rooms_.count(room.id) != 0)
        return false;
    room.occupants.clear();
    rooms_.emplace(room.id, std::move(room));
    return true;
}

WorkRoom* WorkAssignmentService::find_room(std::uint64_t id) noexcept {
    auto it = rooms_.find(id);
    return it == rooms_.end() ? nullptr : &it->second;
}

const WorkRoom* WorkAssignmentService::find_room(std::uint64_t id) const noexcept {
    auto it = rooms_.find(id);
    return it == rooms_.end() ? nullptr : &it->second;
}

bool WorkAssignmentService::remove_occupant(std::uint64_t room_id, std::uint64_t dweller_id) {
    auto* room = find_room(room_id);
    if (room == nullptr) return false;
    const auto old_size = room->occupants.size();
    room->occupants.erase(std::remove(room->occupants.begin(), room->occupants.end(), dweller_id),
                          room->occupants.end());
    return room->occupants.size() != old_size;
}

AssignmentError WorkAssignmentService::validate_target(const Dweller& dweller,
                                                        const WorkRoom& room) const {
    if (!dweller.alive() || dweller.status == ActivityStatus::Exploring ||
        dweller.status == ActivityStatus::Questing)
        return AssignmentError::DwellerUnavailable;
    if (!room.available) return AssignmentError::RoomUnavailable;
    const bool already_inside =
        std::find(room.occupants.begin(), room.occupants.end(), dweller.id) != room.occupants.end();
    if (!already_inside && static_cast<int>(room.occupants.size()) >= room.capacity)
        return AssignmentError::RoomFull;
    return AssignmentError::None;
}

int WorkAssignmentService::efficiency(std::uint64_t dweller_id, std::uint64_t room_id) const {
    const auto* dweller = const_cast<DwellerService&>(dwellers_).find(dweller_id);
    const auto* room = find_room(room_id);
    if (dweller == nullptr || room == nullptr || !room->available || !dweller->alive()) return 0;
    const int stat = dweller->effective_special().values[static_cast<std::size_t>(room->preferred_special_index)];
    return stat * 100 + dweller->level * 5;
}

int WorkAssignmentService::group_efficiency(std::uint64_t room_id) const {
    const auto* room = find_room(room_id);
    if (room == nullptr) return 0;
    int total = 0;
    for (const auto id : room->occupants) total += efficiency(id, room_id);
    return total;
}

AssignmentPreview WorkAssignmentService::preview(std::uint64_t dweller_id,
                                                 std::uint64_t room_id) const {
    AssignmentPreview result;
    const auto* dweller = const_cast<DwellerService&>(dwellers_).find(dweller_id);
    if (dweller == nullptr) {
        result.error = AssignmentError::UnknownDweller;
        return result;
    }
    const auto* room = find_room(room_id);
    if (room == nullptr) {
        result.error = AssignmentError::UnknownRoom;
        return result;
    }
    result.error = validate_target(*dweller, *room);
    result.current_efficiency = dweller->room_id == 0 ? 0 : efficiency(dweller_id, dweller->room_id);
    result.target_efficiency = result.error == AssignmentError::None ? efficiency(dweller_id, room_id) : 0;
    result.difference = result.target_efficiency - result.current_efficiency;
    return result;
}

AssignmentError WorkAssignmentService::move(std::uint64_t dweller_id, std::uint64_t room_id,
                                            std::int64_t complete_at,
                                            std::uint64_t command_sequence) {
    auto* dweller = dwellers_.find(dweller_id);
    if (dweller == nullptr) return AssignmentError::UnknownDweller;
    auto* room = find_room(room_id);
    if (room == nullptr) return AssignmentError::UnknownRoom;
    if (std::any_of(transit_.begin(), transit_.end(), [dweller_id](const TransitOrder& order) {
            return order.dweller_id == dweller_id;
        }))
        return AssignmentError::AlreadyInTransit;
    const auto validation = validate_target(*dweller, *room);
    if (validation != AssignmentError::None) return validation;

    transit_.push_back({command_sequence, dweller_id, dweller->room_id, room_id, complete_at});
    std::stable_sort(transit_.begin(), transit_.end(), [](const TransitOrder& a, const TransitOrder& b) {
        if (a.complete_at != b.complete_at) return a.complete_at < b.complete_at;
        return a.sequence < b.sequence;
    });
    dweller->status = ActivityStatus::Idle;
    return AssignmentError::None;
}

void WorkAssignmentService::advance(std::int64_t now) {
    std::size_t index = 0;
    while (index < transit_.size() && transit_[index].complete_at <= now) {
        const TransitOrder order = transit_[index];
        auto* dweller = dwellers_.find(order.dweller_id);
        auto* target = find_room(order.to_room_id);
        if (dweller != nullptr && target != nullptr &&
            validate_target(*dweller, *target) == AssignmentError::None) {
            remove_occupant(order.from_room_id, order.dweller_id);
            if (std::find(target->occupants.begin(), target->occupants.end(), order.dweller_id) ==
                target->occupants.end())
                target->occupants.push_back(order.dweller_id);
            dweller->room_id = target->id;
            dweller->status = ActivityStatus::Working;
        } else if (dweller != nullptr) {
            dweller->status = ActivityStatus::Idle;
        }
        ++index;
    }
    transit_.erase(transit_.begin(), transit_.begin() + static_cast<std::ptrdiff_t>(index));
}

bool WorkAssignmentService::remove_room(std::uint64_t room_id, std::int64_t timestamp) {
    auto it = rooms_.find(room_id);
    if (it == rooms_.end()) return false;
    for (const auto id : it->second.occupants) {
        if (auto* dweller = dwellers_.find(id)) {
            dweller->room_id = 0;
            dweller->status = ActivityStatus::Idle;
            dweller->history.push_back({timestamp, "assignment_cancelled", "room_removed"});
        }
    }
    rooms_.erase(it);
    transit_.erase(std::remove_if(transit_.begin(), transit_.end(), [room_id](const TransitOrder& order) {
                       return order.to_room_id == room_id || order.from_room_id == room_id;
                   }),
                   transit_.end());
    return true;
}

void WorkAssignmentService::cancel_for_dweller(std::uint64_t dweller_id, std::int64_t timestamp,
                                               const std::string& reason) {
    transit_.erase(std::remove_if(transit_.begin(), transit_.end(), [dweller_id](const TransitOrder& order) {
                       return order.dweller_id == dweller_id;
                   }),
                   transit_.end());
    if (auto* dweller = dwellers_.find(dweller_id)) {
        remove_occupant(dweller->room_id, dweller_id);
        dweller->room_id = 0;
        if (dweller->alive()) dweller->status = ActivityStatus::Idle;
        dweller->history.push_back({timestamp, "assignment_cancelled", reason});
    }
}

bool WorkAssignmentService::apply_happiness(std::uint64_t dweller_id, int delta,
                                            std::string reason, std::int64_t timestamp) {
    auto* dweller = dwellers_.find(dweller_id);
    if (dweller == nullptr || reason.empty()) return false;
    const int before = dweller->happiness;
    dweller->happiness = std::clamp(dweller->happiness + delta, 0, 100);
    happiness_log_.push_back({timestamp, dweller_id, dweller->happiness - before, std::move(reason)});
    return true;
}

std::optional<std::uint64_t> WorkAssignmentService::suggest_room(std::uint64_t dweller_id) const {
    std::optional<std::uint64_t> best;
    int best_efficiency = std::numeric_limits<int>::min();
    for (const auto& [id, room] : rooms_) {
        const auto preview_result = preview(dweller_id, id);
        if (preview_result.error != AssignmentError::None) continue;
        if (preview_result.target_efficiency > best_efficiency) {
            best_efficiency = preview_result.target_efficiency;
            best = id;
        }
    }
    return best;
}

const std::vector<TransitOrder>& WorkAssignmentService::transit_queue() const noexcept {
    return transit_;
}

const std::vector<HappinessEntry>& WorkAssignmentService::happiness_log() const noexcept {
    return happiness_log_;
}

}  // namespace deep_shelter::dwellers
