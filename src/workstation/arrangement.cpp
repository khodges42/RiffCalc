// SPDX-License-Identifier: GPL-3.0-or-later
#include "packrat/workstation.hpp"
#include <stdexcept>
namespace packrat {

    std::vector<std::uint8_t> arrangement_midi(std::span<const Clip> tracks) {

        if(tracks.empty()||tracks.size()>16)throw std::invalid_argument("Arrangement requires 1-16 tracks");

        for(const auto& track:tracks) {

            validate(track);

            if(track.bpm!=tracks[0].bpm || track.meter!=tracks[0].meter || track.length!=tracks[0].length)throw std::invalid_argument("Arrangement timing mismatch");

        }

        Clip conductor=tracks[0];
        conductor.name="PACKRAT Tempo";
        conductor.notes.clear();

        auto file=midi_file(conductor);
        file[9]=1;
        file[10]=0;
        file[11]=static_cast<std::uint8_t>(tracks.size()+1);

        for(const auto& c:tracks) {

            const auto bytes=midi_file(c);

            std::size_t pos=25;
            // after track name's delta, FF and 03
            std::size_t size=0;

            do {
                size=(size<<7)|(bytes[pos]&127);

            }
            while((bytes[pos++]&128)!=0);

            pos+=size;

            // Retain name; tempo (7 bytes) and meter (8 bytes) belong to conductor only.
            const auto track_size=bytes.size()-22-15;

            file.insert(file.end(),{
                'M','T','r','k'
            });

            for(int shift=24;shift>=0;shift-=8)file.push_back(static_cast<std::uint8_t>((track_size>>shift)&255));

            file.insert(file.end(),bytes.begin()+22,bytes.begin()+static_cast<std::ptrdiff_t>(pos));

            file.insert(file.end(),bytes.begin()+static_cast<std::ptrdiff_t>(pos+15),bytes.end());

        }

        return file;

    }

}
