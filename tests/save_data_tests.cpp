#include "persistence/SaveData.hpp"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

using namespace deep_shelter::persistence;

namespace {

std::string make_temp_dir() {
    char pattern[] = "/tmp/deep-shelter-save-XXXXXX";
    char* result = mkdtemp(pattern);
    assert(result != nullptr);
    return result;
}

void remove_tree(const std::string& root) {
    for (unsigned slot = 0; slot < 2; ++slot) {
        std::remove((root + "/slot" + std::to_string(slot) + ".sav").c_str());
        std::remove((root + "/slot" + std::to_string(slot) + ".bak").c_str());
        std::remove((root + "/slot" + std::to_string(slot) + ".tmp").c_str());
    }
    rmdir(root.c_str());
}

ShelterSave sample(std::uint64_t generation, const std::string& name) {
    ShelterSave save;
    save.generation = generation;
    save.saved_at_unix_ms = 123456789;
    save.credits = 400;
    save.population = 12;
    save.shelter_name = name;
    return save;
}

void codec_round_trip() {
    const auto original = sample(7, "Delta");
    const auto decoded = decode_save(encode_save(original));
    assert(decoded.status == SaveStatus::Ok);
    assert(decoded.save.generation == 7);
    assert(decoded.save.shelter_name == "Delta");
    assert(decoded.save.credits == 400);
}

void corrupt_and_partial_files_are_rejected() {
    auto bytes = encode_save(sample(1, "Alpha"));
    bytes.back() ^= 0x7F;
    assert(decode_save(bytes).status == SaveStatus::Corrupt);
    bytes.resize(5);
    assert(decode_save(bytes).status == SaveStatus::Corrupt);
    assert(decode_save({}).status == SaveStatus::Corrupt);
}

void future_version_is_never_accepted() {
    auto bytes = encode_save(sample(1, "Future"));
    bytes[4] = static_cast<std::uint8_t>(kCurrentSaveVersion + 1);
    bytes[5] = 0;
    assert(decode_save(bytes).status == SaveStatus::UnsupportedFutureVersion);
}

void backup_recovers_corrupt_main() {
    const auto root = make_temp_dir();
    SaveStore store(root);
    std::string detail;
    assert(store.save_slot(0, sample(1, "First"), &detail) == SaveStatus::Ok);
    assert(store.save_slot(0, sample(2, "Second"), &detail) == SaveStatus::Ok);

    std::ofstream corrupt(root + "/slot0.sav", std::ios::binary | std::ios::trunc);
    corrupt << "broken";
    corrupt.close();

    const auto loaded = store.load_slot(0);
    assert(loaded.status == SaveStatus::Ok);
    assert(loaded.used_backup);
    assert(loaded.save.generation == 1);
    assert(loaded.save.shelter_name == "First");
    remove_tree(root);
}

void newest_valid_generation_wins() {
    const auto root = make_temp_dir();
    SaveStore store(root);
    assert(store.save_slot(0, sample(3, "Third")) == SaveStatus::Ok);
    assert(store.save_slot(0, sample(4, "Fourth")) == SaveStatus::Ok);
    auto loaded = store.load_slot(0);
    assert(loaded.status == SaveStatus::Ok);
    assert(!loaded.used_backup);
    assert(loaded.save.generation == 4);
    remove_tree(root);
}

void slots_are_independent_and_resettable() {
    const auto root = make_temp_dir();
    SaveStore store(root);
    assert(store.save_slot(0, sample(1, "Zero")) == SaveStatus::Ok);
    assert(store.save_slot(1, sample(9, "One")) == SaveStatus::Ok);
    assert(store.reset_slot(0) == SaveStatus::Ok);
    assert(store.load_slot(0).status == SaveStatus::Missing);
    assert(store.load_slot(1).save.generation == 9);
    assert(store.export_metadata(1).find("generation=9") != std::string::npos);
    remove_tree(root);
}

}  // namespace

int main() {
    codec_round_trip();
    corrupt_and_partial_files_are_rejected();
    future_version_is_never_accepted();
    backup_recovers_corrupt_main();
    newest_valid_generation_wins();
    slots_are_independent_and_resettable();
    return 0;
}
