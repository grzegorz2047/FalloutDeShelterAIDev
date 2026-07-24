#pragma once

#include "dwellers/Dweller.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace deep_shelter::dwellers {

enum class AssignmentError { None, UnknownDweller, UnknownRoom, RoomUnavailable, RoomFull, DwellerUnavailable, AlreadyInTransit };

struct WorkRoom {
    std::uint64_t id = 0;
    int capacity = 0;
    int preferred_special_index = 0;
    bool available = true;
    std::vector<std::uint64_t> occupants;
};

struct TransitOrder {
    std::uint64_t sequence = 0;
    std::uint64_t dweller_id = 0;
    std::uint64_t from_room_id = 0;
    std::uint64_t to_room_id = 0;
    std::int64_t complete_at = 0;
};

struct HappinessEntry {
    std::int64_t timestamp = 0;
    std::uint64_t dweller_id = 0;
    int delta = 0;
    std::string reason;
};

struct AssignmentPreview {
    AssignmentError error = AssignmentError::None;
    int current_efficiency = 0;
    int target_efficiency = 0;
    int difference = 0;
};

class WorkAssignmentService {
public:
    explicit WorkAssignmentService(DwellerService& dwellers);

    bool add_room(WorkRoom room);
    bool remove_room(std::uint64_t room_id, std::int64_t timestamp);
    [[nodiscard]] AssignmentPreview preview(std::uint64_t dweller_id, std::uint64_t room_id) const;
    AssignmentError move(std::uint64_t dweller_id, std::uint64_t room_id,
                         std::int64_t complete_at, std::uint64_t command_sequence);
    void advance(std::int64_t now);
    void cancel_for_dweller(std::uint64_t dweller_id, std::int64_t timestamp,
                            const std::string& reason);
    bool apply_happiness(std::uint64_t dweller_id, int delta, std::string reason,
                         std::int64_t timestamp);

    [[nodiscard]] int efficiency(std::uint64_t dweller_id, std::uint64_t room_id) const;
    [[nodiscard]] int group_efficiency(std::uint64_t room_id) const;
    [[nodiscard]] std::optional<std::uint64_t> suggest_room(std::uint64_t dweller_id) const;
    [[nodiscard]] const std::vector<TransitOrder>& transit_queue() const noexcept;
    [[nodiscard]] const std::vector<HappinessEntry>& happiness_log() const noexcept;

private:
    WorkRoom* find_room(std::uint64_t id) noexcept;
    const WorkRoom* find_room(std::uint64_t id) const noexcept;
    bool remove_occupant(std::uint64_t room_id, std::uint64_t dweller_id);
    AssignmentError validate_target(const Dweller& dweller, const WorkRoom& room) const;

    DwellerService& dwellers_;
    std::map<std::uint64_t, WorkRoom> rooms_;
    std::vector<TransitOrder> transit_;
    std::vector<HappinessEntry> happiness_log_;
};

}  // namespace deep_shelter::dwellers
