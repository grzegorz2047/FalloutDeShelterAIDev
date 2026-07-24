#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

namespace deep_shelter::rooms {

struct RoomSegment {
    std::uint64_t segment_id = 0;
    std::uint64_t group_id = 0;
    std::string type_id;
    int level = 1;
    int column = 0;
    int row = 0;
    int residents = 0;
    int stored_units = 0;
    int in_progress_units = 0;
    bool incident_active = false;
};

struct LifecyclePreview {
    bool allowed = false;
    int credit_delta = 0;
    int residents_to_evacuate = 0;
    int stored_units_to_relocate = 0;
    int in_progress_units_to_preserve = 0;
    std::string reason;
};

class RoomLifecycle {
public:
    RoomLifecycle(int credits, int max_group_width = 3, int max_level = 3);

    [[nodiscard]] int credits() const noexcept;
    [[nodiscard]] const std::vector<RoomSegment>& segments() const noexcept;
    [[nodiscard]] std::vector<RoomSegment> normalized_segments() const;

    bool add_segment(RoomSegment segment);
    [[nodiscard]] LifecyclePreview preview_upgrade(std::uint64_t group_id, int cost) const;
    bool confirm_upgrade(std::uint64_t group_id, int cost, std::uint64_t transaction_id);
    [[nodiscard]] LifecyclePreview preview_demolish(std::uint64_t segment_id, int refund) const;
    bool confirm_demolish(std::uint64_t segment_id, int refund, int relocation_capacity,
                          std::uint64_t transaction_id);

private:
    void normalize_groups();
    std::vector<std::size_t> group_indices(std::uint64_t group_id) const;

    int credits_ = 0;
    int max_group_width_ = 3;
    int max_level_ = 3;
    std::vector<RoomSegment> segments_;
    std::unordered_set<std::uint64_t> committed_transactions_;
};

}  // namespace deep_shelter::rooms
