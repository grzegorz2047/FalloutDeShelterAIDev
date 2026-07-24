#include "rooms/RoomLifecycle.hpp"

#include <algorithm>

namespace deep_shelter::rooms {

RoomLifecycle::RoomLifecycle(int credits, int max_group_width, int max_level)
    : credits_(credits),
      max_group_width_(std::max(1, max_group_width)),
      max_level_(std::max(1, max_level)) {}

int RoomLifecycle::credits() const noexcept { return credits_; }
const std::vector<RoomSegment>& RoomLifecycle::segments() const noexcept { return segments_; }

bool RoomLifecycle::add_segment(RoomSegment segment) {
    if (segment.segment_id == 0 || segment.type_id.empty() || segment.level < 1 ||
        segment.level > max_level_) return false;
    for (const auto& current : segments_) {
        if (current.segment_id == segment.segment_id ||
            (current.column == segment.column && current.row == segment.row)) return false;
    }
    if (segment.group_id == 0) segment.group_id = segment.segment_id;
    segments_.push_back(std::move(segment));
    normalize_groups();
    return true;
}

std::vector<std::size_t> RoomLifecycle::group_indices(std::uint64_t group_id) const {
    std::vector<std::size_t> values;
    for (std::size_t index = 0; index < segments_.size(); ++index) {
        if (segments_[index].group_id == group_id) values.push_back(index);
    }
    return values;
}

void RoomLifecycle::normalize_groups() {
    std::sort(segments_.begin(), segments_.end(), [](const RoomSegment& left, const RoomSegment& right) {
        if (left.row != right.row) return left.row < right.row;
        if (left.column != right.column) return left.column < right.column;
        return left.segment_id < right.segment_id;
    });

    for (auto& segment : segments_) segment.group_id = segment.segment_id;
    std::size_t start = 0;
    while (start < segments_.size()) {
        std::size_t end = start + 1;
        while (end < segments_.size() &&
               segments_[end].row == segments_[start].row &&
               segments_[end].column == segments_[end - 1].column + 1 &&
               segments_[end].type_id == segments_[start].type_id &&
               segments_[end].level == segments_[start].level &&
               static_cast<int>(end - start) < max_group_width_) {
            ++end;
        }
        std::uint64_t stable_group = segments_[start].segment_id;
        for (std::size_t index = start + 1; index < end; ++index) {
            stable_group = std::min(stable_group, segments_[index].segment_id);
        }
        for (std::size_t index = start; index < end; ++index) segments_[index].group_id = stable_group;
        start = end;
    }
}

std::vector<RoomSegment> RoomLifecycle::normalized_segments() const { return segments_; }

LifecyclePreview RoomLifecycle::preview_upgrade(std::uint64_t group_id, int cost) const {
    LifecyclePreview preview;
    const auto indices = group_indices(group_id);
    if (indices.empty()) {
        preview.reason = "Room group does not exist.";
        return preview;
    }
    if (cost <= 0) {
        preview.reason = "Upgrade cost must be positive.";
        return preview;
    }
    if (credits_ < cost) {
        preview.reason = "Not enough credits.";
        return preview;
    }
    for (const auto index : indices) {
        const auto& segment = segments_[index];
        if (segment.level >= max_level_) {
            preview.reason = "Room group is already at maximum level.";
            return preview;
        }
        if (segment.incident_active) {
            preview.reason = "Resolve the active incident before upgrading.";
            return preview;
        }
        preview.residents_to_evacuate += segment.residents;
        preview.in_progress_units_to_preserve += segment.in_progress_units;
    }
    preview.allowed = true;
    preview.credit_delta = -cost;
    return preview;
}

bool RoomLifecycle::confirm_upgrade(std::uint64_t group_id, int cost,
                                    std::uint64_t transaction_id) {
    if (transaction_id == 0 || committed_transactions_.count(transaction_id) != 0) return false;
    const auto preview = preview_upgrade(group_id, cost);
    if (!preview.allowed) return false;
    const auto indices = group_indices(group_id);
    credits_ += preview.credit_delta;
    for (const auto index : indices) ++segments_[index].level;
    committed_transactions_.insert(transaction_id);
    normalize_groups();
    return true;
}

LifecyclePreview RoomLifecycle::preview_demolish(std::uint64_t segment_id, int refund) const {
    LifecyclePreview preview;
    const auto it = std::find_if(segments_.begin(), segments_.end(),
                                 [&](const RoomSegment& value) { return value.segment_id == segment_id; });
    if (it == segments_.end()) {
        preview.reason = "Room segment does not exist.";
        return preview;
    }
    if (refund < 0) {
        preview.reason = "Refund cannot be negative.";
        return preview;
    }
    if (it->incident_active) {
        preview.reason = "Resolve the active incident before demolition.";
        return preview;
    }
    preview.allowed = true;
    preview.credit_delta = refund;
    preview.residents_to_evacuate = it->residents;
    preview.stored_units_to_relocate = it->stored_units;
    preview.in_progress_units_to_preserve = it->in_progress_units;
    return preview;
}

bool RoomLifecycle::confirm_demolish(std::uint64_t segment_id, int refund,
                                     int relocation_capacity,
                                     std::uint64_t transaction_id) {
    if (transaction_id == 0 || committed_transactions_.count(transaction_id) != 0) return false;
    const auto preview = preview_demolish(segment_id, refund);
    if (!preview.allowed) return false;
    const int required = preview.residents_to_evacuate + preview.stored_units_to_relocate +
                         preview.in_progress_units_to_preserve;
    if (relocation_capacity < required) return false;
    const auto it = std::find_if(segments_.begin(), segments_.end(),
                                 [&](const RoomSegment& value) { return value.segment_id == segment_id; });
    if (it == segments_.end()) return false;
    credits_ += preview.credit_delta;
    segments_.erase(it);
    committed_transactions_.insert(transaction_id);
    normalize_groups();
    return true;
}

}  // namespace deep_shelter::rooms
