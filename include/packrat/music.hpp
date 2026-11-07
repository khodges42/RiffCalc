// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
namespace packrat {
    using Tick = std::int64_t;
    inline constexpr int ppq = 960;
    inline constexpr Tick bar_ticks = ppq * 4;
    inline constexpr std::string_view engine_version = "packrat-1";
    int pitch_class(int note) noexcept;
    struct Pitch {
        int midi = 60;
        int octave() const noexcept {
            return midi / 12 - 1;
        }
        int pc() const noexcept {
            return pitch_class(midi);
        }
        static Pitch from(int pc, int octave);
        bool operator==(const Pitch&) const = default;
    };
    struct Key {
        int root = 0;
        char letter = 'C';
        int accidental = 0;
        std::string name() const;
        bool operator==(const Key&) const = default;
    };
    Key parse_key(std::string_view text);
    enum class Scale {
        Major, Minor, HarmonicMinor, MelodicMinor, Dorian, Phrygian,
        Mixolydian, Lydian, Locrian, MinorPentatonic
    };
    struct ScaleInfo {
        Scale scale;
        std::string_view id;
        std::vector<int> intervals;
    };
    const std::array<ScaleInfo,10>& scales();
    const ScaleInfo& scale_info(Scale scale);
    Scale parse_scale(std::string_view text);
    bool in_scale(Key key, Scale scale, int note);
    int degree_pc(Key key, Scale scale, int degree);
    std::string spell_degree(Key key, Scale scale, int degree);
    enum class Quality {
        Major, Minor, Diminished, Augmented, Sus2, Sus4, Dominant7,
        Major7, Minor7, MinorMajor7, Minor9, Major9, Add9, Major6,
        Minor6, Dominant7Flat9
    };
    struct Formula {
        Quality quality;
        std::string_view id;
        std::string_view suffix;
        std::vector<int> intervals;
        std::vector<int> letter_steps;
        int complexity;
    };
    const std::array<Formula,16>& formulas();
    const Formula& formula(Quality quality);
    Quality parse_quality(std::string_view id);
    struct ChordSpec {
        int root = 0;
        int degree = 0;
        Quality quality = Quality::Major;
        bool chromatic = false;
        bool operator==(const ChordSpec&) const = default;
    };
    std::vector<int> chord_classes(const ChordSpec& chord);
    std::vector<std::string> spell_chord(Key key, Scale scale, const ChordSpec& chord);
    std::string roman(const ChordSpec& chord);
    struct Chord {
        ChordSpec spec;
        Tick start = 0;
        Tick length = bar_ticks;
        std::vector<Pitch> notes;
        int inversion = 0;
        std::string symbol;
        bool operator==(const Chord&) const = default;
    };
    struct Note {
        Tick start = 0;
        Tick duration = ppq;
        int pitch = 60;
        int velocity = 88;
        int channel = 0;
        bool operator==(const Note&) const = default;
    };
    struct Meter {
        int numerator = 4;
        int denominator = 4;
        bool operator==(const Meter&) const = default;
    };
    struct Clip {
        std::string name = "PACKRAT";
        Tick length = bar_ticks * 4;
        int bpm = 120;
        Meter meter;
        std::vector<Note> notes;
        bool operator==(const Clip&) const = default;
    };
    void validate(const Clip& clip);
}
// namespace packrat
