// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "profiles.hpp"
#include <span>
namespace packrat {
    // SplitMix64, fixed unsigned modulo-2^64 arithmetic. No std::*_distribution.
    class Random {
        std::uint64_t state_;
        public:
        explicit Random(std::uint64_t seed) : state_(seed) {}
        std::uint64_t next() noexcept;
        std::uint64_t bounded(std::uint64_t bound);
        std::size_t weighted(std::span<const std::uint32_t> weights);
    };
    struct Seeds {
        std::uint64_t harmony, voicing, rhythm, timing, velocity;
        bool operator==(const Seeds&) const = default;
    };
    Seeds stage_seeds(std::uint64_t master) noexcept;
    struct Config {
        Key key;
        Scale scale = Scale::Minor;
        int bars = 4;
        int complexity = 50;
        std::uint64_t seed = 1;
        int low = 48, high = 84;
        int bpm = 174;
        int velocity = 88;
        int humanize = 0;
        bool resolve = false;
        bool colour = false;
        bool operator==(const Config&) const = default;
    };
    void validate(const Config& config);
    struct Result {
        Config config;
        Seeds seeds;
        std::string genre;
        int profile_version = 1;
        std::string profile_fingerprint;
        std::vector<Chord> chords;
        Clip clip;
        bool operator==(const Result&) const = default;
    };
    std::vector<ChordSpec> harmony(const Config& config,const GenreProfile& profile,Random& random);
    std::vector<Chord> voice(const Config& config,const GenreProfile& profile,
    std::span<const ChordSpec> progression,Random& random);
    int movement_cost(std::span<const Pitch> a,std::span<const Pitch> b);
    Result generate(const Config& config,const ProfileBank& bank,std::string_view genre);
    std::string metadata(const Result& result);
}
// namespace packrat
