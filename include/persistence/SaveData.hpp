#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace deep_shelter::persistence {

constexpr std::uint32_t kSaveMagic = 0x44533344; // DS3D
constexpr std::uint16_t kCurrentSaveVersion = 2;

struct ShelterSave {
    std::uint64_t generation = 0;
    std::int64_t saved_at_unix_ms = 0;
    std::int32_t credits = 0;
    std::int32_t population = 0;
    std::string shelter_name;
};

enum class SaveStatus {
    Ok,
    Missing,
    Corrupt,
    UnsupportedFutureVersion,
    IoError,
    NoSpace,
};

struct LoadResult {
    SaveStatus status = SaveStatus::Missing;
    ShelterSave save{};
    bool used_backup = false;
    std::string detail;
};

std::vector<std::uint8_t> encode_save(const ShelterSave& save);
LoadResult decode_save(const std::vector<std::uint8_t>& bytes);
std::uint32_t crc32(const std::uint8_t* data, std::size_t size) noexcept;

class SaveStore {
public:
    explicit SaveStore(std::string root_path);

    SaveStatus save_slot(unsigned slot, const ShelterSave& save, std::string* detail = nullptr) const;
    LoadResult load_slot(unsigned slot) const;
    SaveStatus reset_slot(unsigned slot, std::string* detail = nullptr) const;
    std::string export_metadata(unsigned slot) const;

private:
    std::string root_path_;
};

}  // namespace deep_shelter::persistence
