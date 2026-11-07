// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "music.hpp"
#include <filesystem>
namespace packrat {
    struct ProgressionPattern {
        std::vector<int> degrees;
        std::uint32_t weight = 1;
    };
    struct QualityWeight {
        Quality quality;
        std::uint32_t weight = 1;
    };
    struct RhythmPattern {
        std::string id;
        std::vector<int> steps;
        int gate_steps = 16;
        int min_complexity = 0;
        std::uint32_t weight = 1;
    };
    struct GenreProfile {
        std::string id, name;
        int version = 1;
        std::vector<Scale> preferred_scales;
        int bpm_min = 60, bpm_default = 120, bpm_max = 200;
        std::vector<ProgressionPattern> patterns;
        std::array<std::array<std::uint32_t,7>,7> transitions{};
        std::array<std::uint32_t,7> cadence{};
        std::vector<QualityWeight> qualities;
        std::vector<RhythmPattern> rhythms;
        int center = 64;
        int spread = 20;
        int repetition = 80;
        int modal = 0;
        int colour = 0;
        int swing = 0;
        int velocity_spread = 4;
        std::array<int,16> accents{};
        std::string preview;
    };
    struct ProfileBank {
        std::vector<GenreProfile> profiles;
        std::string fingerprint;
    };
    void validate(const GenreProfile& profile);
    ProfileBank parse_profiles(std::string_view json);
    ProfileBank load_profiles(const std::filesystem::path& file);
    ProfileBank bundled_profiles();
    const GenreProfile& find_profile(const ProfileBank& bank, std::string_view id);
}
// namespace packrat
