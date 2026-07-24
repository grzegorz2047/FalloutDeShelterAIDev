#include "persistence/SaveData.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <vector>

namespace deep_shelter::persistence {
namespace {

constexpr std::size_t kHeaderSize = 24;
constexpr std::size_t kMaxPayloadSize = 1024 * 1024;

void append_u16(std::vector<std::uint8_t>& out, std::uint16_t value) {
    out.push_back(static_cast<std::uint8_t>(value));
    out.push_back(static_cast<std::uint8_t>(value >> 8));
}
void append_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8) out.push_back(static_cast<std::uint8_t>(value >> shift));
}
void append_u64(std::vector<std::uint8_t>& out, std::uint64_t value) {
    for (int shift = 0; shift < 64; shift += 8) out.push_back(static_cast<std::uint8_t>(value >> shift));
}

bool read_u16(const std::vector<std::uint8_t>& data, std::size_t& offset, std::uint16_t& value) {
    if (offset + 2 > data.size()) return false;
    value = static_cast<std::uint16_t>(data[offset]) |
            static_cast<std::uint16_t>(data[offset + 1] << 8);
    offset += 2;
    return true;
}
bool read_u32(const std::vector<std::uint8_t>& data, std::size_t& offset, std::uint32_t& value) {
    if (offset + 4 > data.size()) return false;
    value = 0;
    for (int shift = 0; shift < 32; shift += 8) value |= static_cast<std::uint32_t>(data[offset++]) << shift;
    return true;
}
bool read_u64(const std::vector<std::uint8_t>& data, std::size_t& offset, std::uint64_t& value) {
    if (offset + 8 > data.size()) return false;
    value = 0;
    for (int shift = 0; shift < 64; shift += 8) value |= static_cast<std::uint64_t>(data[offset++]) << shift;
    return true;
}

std::string slot_path(const std::string& root, unsigned slot, const char* suffix) {
    return root + "/slot" + std::to_string(slot) + suffix;
}

bool read_file(const std::string& path, std::vector<std::uint8_t>& bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return false;
    input.seekg(0, std::ios::end);
    const auto length = input.tellg();
    if (length < 0 || static_cast<std::uint64_t>(length) > kMaxPayloadSize + kHeaderSize) return false;
    input.seekg(0, std::ios::beg);
    bytes.resize(static_cast<std::size_t>(length));
    return bytes.empty() || static_cast<bool>(input.read(reinterpret_cast<char*>(bytes.data()), length));
}

SaveStatus errno_status() {
    return errno == ENOSPC ? SaveStatus::NoSpace : SaveStatus::IoError;
}

bool ensure_directory(const std::string& path) {
    if (::mkdir(path.c_str(), 0777) == 0) return true;
    return errno == EEXIST;
}

}  // namespace

std::uint32_t crc32(const std::uint8_t* data, std::size_t size) noexcept {
    std::uint32_t crc = 0xFFFFFFFFu;
    for (std::size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

std::vector<std::uint8_t> encode_save(const ShelterSave& save) {
    std::vector<std::uint8_t> payload;
    append_u64(payload, static_cast<std::uint64_t>(save.saved_at_unix_ms));
    append_u32(payload, static_cast<std::uint32_t>(save.credits));
    append_u32(payload, static_cast<std::uint32_t>(save.population));
    const auto name_size = static_cast<std::uint16_t>(std::min<std::size_t>(save.shelter_name.size(), 255));
    append_u16(payload, name_size);
    payload.insert(payload.end(), save.shelter_name.begin(), save.shelter_name.begin() + name_size);

    std::vector<std::uint8_t> bytes;
    append_u32(bytes, kSaveMagic);
    append_u16(bytes, kCurrentSaveVersion);
    append_u16(bytes, 0);
    append_u32(bytes, static_cast<std::uint32_t>(payload.size()));
    append_u32(bytes, crc32(payload.data(), payload.size()));
    append_u64(bytes, save.generation);
    bytes.insert(bytes.end(), payload.begin(), payload.end());
    return bytes;
}

LoadResult decode_save(const std::vector<std::uint8_t>& bytes) {
    LoadResult result;
    if (bytes.size() < kHeaderSize) {
        result.status = SaveStatus::Corrupt;
        result.detail = "partial save header";
        return result;
    }
    std::size_t offset = 0;
    std::uint32_t magic = 0, payload_size = 0, checksum = 0;
    std::uint16_t version = 0, reserved = 0;
    std::uint64_t generation = 0;
    if (!read_u32(bytes, offset, magic) || !read_u16(bytes, offset, version) ||
        !read_u16(bytes, offset, reserved) || !read_u32(bytes, offset, payload_size) ||
        !read_u32(bytes, offset, checksum) || !read_u64(bytes, offset, generation)) {
        result.status = SaveStatus::Corrupt;
        result.detail = "invalid save header";
        return result;
    }
    if (magic != kSaveMagic) {
        result.status = SaveStatus::Corrupt;
        result.detail = "invalid save magic";
        return result;
    }
    if (version > kCurrentSaveVersion) {
        result.status = SaveStatus::UnsupportedFutureVersion;
        result.detail = "save was created by a newer game version";
        return result;
    }
    if (payload_size > kMaxPayloadSize || bytes.size() != kHeaderSize + payload_size) {
        result.status = SaveStatus::Corrupt;
        result.detail = "save payload length mismatch";
        return result;
    }
    const auto* payload = bytes.data() + kHeaderSize;
    if (crc32(payload, payload_size) != checksum) {
        result.status = SaveStatus::Corrupt;
        result.detail = "save checksum mismatch";
        return result;
    }

    std::vector<std::uint8_t> body(payload, payload + payload_size);
    std::size_t body_offset = 0;
    std::uint64_t saved_at = 0;
    std::uint32_t credits = 0, population = 0;
    std::uint16_t name_size = 0;
    if (!read_u64(body, body_offset, saved_at) || !read_u32(body, body_offset, credits) ||
        !read_u32(body, body_offset, population) || !read_u16(body, body_offset, name_size) ||
        body_offset + name_size != body.size()) {
        result.status = SaveStatus::Corrupt;
        result.detail = "invalid save payload";
        return result;
    }
    result.save.generation = generation;
    result.save.saved_at_unix_ms = static_cast<std::int64_t>(saved_at);
    result.save.credits = static_cast<std::int32_t>(credits);
    result.save.population = static_cast<std::int32_t>(population);
    result.save.shelter_name.assign(reinterpret_cast<const char*>(body.data() + body_offset), name_size);
    result.status = SaveStatus::Ok;
    result.detail = version < kCurrentSaveVersion ? "migrated legacy save" : "ok";
    return result;
}

SaveStore::SaveStore(std::string root_path) : root_path_(std::move(root_path)) {}

SaveStatus SaveStore::save_slot(unsigned slot, const ShelterSave& save, std::string* detail) const {
    if (slot > 1) {
        if (detail) *detail = "slot must be 0 or 1";
        return SaveStatus::IoError;
    }
    if (!ensure_directory(root_path_)) {
        if (detail) *detail = "cannot create save directory";
        return errno_status();
    }
    const auto main = slot_path(root_path_, slot, ".sav");
    const auto temp = slot_path(root_path_, slot, ".tmp");
    const auto backup = slot_path(root_path_, slot, ".bak");
    const auto bytes = encode_save(save);

    {
        std::ofstream output(temp, std::ios::binary | std::ios::trunc);
        if (!output || !output.write(reinterpret_cast<const char*>(bytes.data()), bytes.size()) || !output.flush()) {
            std::remove(temp.c_str());
            if (detail) *detail = "cannot write temporary save; previous save preserved";
            return errno_status();
        }
    }
    std::vector<std::uint8_t> verification;
    if (!read_file(temp, verification) || decode_save(verification).status != SaveStatus::Ok) {
        std::remove(temp.c_str());
        if (detail) *detail = "temporary save failed validation";
        return SaveStatus::IoError;
    }
    std::remove(backup.c_str());
    if (std::rename(main.c_str(), backup.c_str()) != 0 && errno != ENOENT) {
        std::remove(temp.c_str());
        if (detail) *detail = "cannot preserve previous save";
        return errno_status();
    }
    if (std::rename(temp.c_str(), main.c_str()) != 0) {
        std::rename(backup.c_str(), main.c_str());
        std::remove(temp.c_str());
        if (detail) *detail = "cannot activate new save; previous save restored";
        return errno_status();
    }
    if (detail) *detail = "ok";
    return SaveStatus::Ok;
}

LoadResult SaveStore::load_slot(unsigned slot) const {
    LoadResult main_result;
    std::vector<std::uint8_t> bytes;
    const auto main = slot_path(root_path_, slot, ".sav");
    const auto backup = slot_path(root_path_, slot, ".bak");
    if (read_file(main, bytes)) main_result = decode_save(bytes);
    else main_result.status = SaveStatus::Missing;

    bytes.clear();
    LoadResult backup_result;
    if (read_file(backup, bytes)) backup_result = decode_save(bytes);
    else backup_result.status = SaveStatus::Missing;

    if (main_result.status == SaveStatus::UnsupportedFutureVersion) return main_result;
    if (backup_result.status == SaveStatus::UnsupportedFutureVersion && main_result.status != SaveStatus::Ok) return backup_result;
    if (main_result.status == SaveStatus::Ok && backup_result.status == SaveStatus::Ok) {
        if (backup_result.save.generation > main_result.save.generation) {
            backup_result.used_backup = true;
            return backup_result;
        }
        return main_result;
    }
    if (main_result.status == SaveStatus::Ok) return main_result;
    if (backup_result.status == SaveStatus::Ok) {
        backup_result.used_backup = true;
        backup_result.detail = "main save invalid; loaded last valid backup";
        return backup_result;
    }
    return main_result.status == SaveStatus::Missing ? backup_result : main_result;
}

SaveStatus SaveStore::reset_slot(unsigned slot, std::string* detail) const {
    if (slot > 1) return SaveStatus::IoError;
    std::remove(slot_path(root_path_, slot, ".tmp").c_str());
    std::remove(slot_path(root_path_, slot, ".bak").c_str());
    if (std::remove(slot_path(root_path_, slot, ".sav").c_str()) != 0 && errno != ENOENT) {
        if (detail) *detail = "cannot remove slot";
        return errno_status();
    }
    if (detail) *detail = "slot reset";
    return SaveStatus::Ok;
}

std::string SaveStore::export_metadata(unsigned slot) const {
    const auto result = load_slot(slot);
    std::ostringstream out;
    out << "slot=" << slot << ";status=" << static_cast<int>(result.status)
        << ";generation=" << result.save.generation
        << ";backup=" << (result.used_backup ? 1 : 0)
        << ";detail=" << result.detail;
    return out.str();
}

}  // namespace deep_shelter::persistence
