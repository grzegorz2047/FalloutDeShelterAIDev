#pragma once

#include <cstddef>
#include <deque>
#include <optional>

namespace deep_shelter::core {

enum class CommandType {
    None,
    Confirm,
    Cancel,
    Pause,
    Resume,
    OpenMenu,
};

struct Command {
    CommandType type = CommandType::None;
    int subject_id = -1;
    int target_id = -1;
};

class CommandQueue {
public:
    explicit CommandQueue(std::size_t capacity = 64) : capacity_(capacity) {}

    [[nodiscard]] bool push(Command command) {
        if (capacity_ == 0 || commands_.size() >= capacity_) {
            return false;
        }
        commands_.push_back(command);
        return true;
    }

    [[nodiscard]] std::optional<Command> pop() {
        if (commands_.empty()) {
            return std::nullopt;
        }
        Command command = commands_.front();
        commands_.pop_front();
        return command;
    }

    void clear() noexcept { commands_.clear(); }
    [[nodiscard]] std::size_t size() const noexcept { return commands_.size(); }
    [[nodiscard]] bool empty() const noexcept { return commands_.empty(); }
    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }

private:
    std::size_t capacity_;
    std::deque<Command> commands_;
};

}  // namespace deep_shelter::core
