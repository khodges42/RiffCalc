// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "music.hpp"
#include <filesystem>
#include <span>
namespace packrat {
    std::vector<std::uint8_t> midi_file(const Clip& clip);
    // Exclusive create: never replaces an existing path. I/O is not an audio API.
    void write_new(const std::filesystem::path& path,std::span<const std::uint8_t> bytes);
    void export_midi(const Clip& clip,const std::filesystem::path& path);
}
// namespace packrat
