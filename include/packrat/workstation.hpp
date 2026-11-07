// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "generator.hpp"
#include "midi.hpp"
namespace packrat {

    inline constexpr std::string_view workstation_version = "packrat-workstation-1";

    enum class Part {
        Chords, Bass, Arp, Melody, Arrangement
    };

    enum class ArpOrder {
        Up, Down, UpDown, Random
    };

    struct Locks {

        bool harmony = false, voicing = false, rhythm = false;

        bool operator==(const Locks&) const = default;

    };

    struct Idea {

        Result harmony;

        std::array<Clip,4> parts;

        ArpOrder arp = ArpOrder::Up;

        bool operator==(const Idea&) const = default;

    };

    struct Document {

        Idea idea;

        Part part = Part::Chords;

        int sound = 1;

        float output_db = -9;

        Locks locks;

        bool follow_host = false;

        bool operator==(const Document&) const = default;

    };

    Idea make_idea(Config config, const ProfileBank& bank, std::string_view genre,
    ArpOrder order = ArpOrder::Up, const Idea* previous = nullptr,
    Locks locks = {

    }, bool mutate = false);

    Clip audition_clip(const Idea& idea, Part part);

    void validate(const Idea& idea);

    std::string serialize(const Document& document);

    Document deserialize(std::string_view text, const ProfileBank& bank);

    std::vector<std::uint8_t> arrangement_midi(std::span<const Clip> tracks);

    std::vector<std::uint8_t> export_bytes(const Document& document);

    // Creates a new directory; never reuses or overwrites an existing pack.
    void export_pack(const std::filesystem::path& directory, const Document& source,
    const ProfileBank& bank, int count);

}
