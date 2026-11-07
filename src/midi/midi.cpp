// SPDX-License-Identifier: GPL-3.0-or-later
#include "packrat/midi.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <stdexcept>
#include <system_error>
#include <tuple>
namespace packrat {
    using Bytes=std::vector<std::uint8_t>;
    static void be(Bytes& b,std::uint32_t x,int count) {
        for(int i=count-1;i>=0;--i)b.push_back(static_cast<std::uint8_t>((x>>(i*8))&255));
    }
    static void vlq(Bytes& b,Tick ticks) {
        if(ticks<0||ticks>0x0fffffff)throw std::invalid_argument("MIDI delta exceeds four-byte VLQ");
        auto x=static_cast<std::uint32_t>(ticks);
        std::uint8_t temp[4];
        int n=0;
        temp[n++]=static_cast<std::uint8_t>(x&127);
        while((x>>=7)!=0)temp[n++]=static_cast<std::uint8_t>((x&127)|128);
        while(n>0)b.push_back(temp[--n]);
    }
    std::vector<std::uint8_t> midi_file(const Clip& clip) {
        validate(clip);
        Bytes track;
        track.insert(track.end(),{
            0,0xff,0x03
        });
        vlq(track,static_cast<Tick>(clip.name.size()));
        track.insert(track.end(),clip.name.begin(),clip.name.end());
        const auto tempo=static_cast<std::uint32_t>((60000000+clip.bpm/2)/clip.bpm);
        track.insert(track.end(),{
            0,0xff,0x51,3
        });
        be(track,tempo,3);
        int exponent=0;
        while((1<<exponent)<clip.meter.denominator)++exponent;
        track.insert(track.end(),{
            0,0xff,0x58,4,static_cast<std::uint8_t>(clip.meter.numerator),static_cast<std::uint8_t>(exponent),24,8
        });
        struct Event {
            Tick tick;
            int order,pitch,channel,velocity;
        };
        std::vector<Event> events;
        events.reserve(clip.notes.size()*2);
        for(const auto& n:clip.notes){
            events.push_back({
                n.start,1,n.pitch,n.channel,n.velocity
            });
            events.push_back({
                n.start+n.duration,0,n.pitch,n.channel,0
            });
        }
        std::sort(events.begin(),events.end(),[](const auto& a,const auto& b){
            return std::tie(a.tick,a.order,a.pitch,a.channel)<std::tie(b.tick,b.order,b.pitch,b.channel);
        });
        Tick previous=0;
        for(const auto& e:events){
            vlq(track,e.tick-previous);
            track.push_back(static_cast<std::uint8_t>((e.order?0x90:0x80)+e.channel));
            track.push_back(static_cast<std::uint8_t>(e.pitch));
            track.push_back(static_cast<std::uint8_t>(e.velocity));
            previous=e.tick;
        }
        vlq(track,clip.length-previous);
        track.insert(track.end(),{
            0xff,0x2f,0
        });
        Bytes file{
            'M','T','h','d'
        };
        be(file,6,4);
        be(file,0,2);
        be(file,1,2);
        be(file,ppq,2);
        file.insert(file.end(),{
            'M','T','r','k'
        });
        be(file,static_cast<std::uint32_t>(track.size()),4);
        file.insert(file.end(),track.begin(),track.end());
        return file;
    }
    void write_new(const std::filesystem::path& path,std::span<const std::uint8_t> bytes) {
        std::FILE* file=nullptr;
#ifdef _WIN32
        const auto error=_wfopen_s(&file,path.c_str(),L"wbx");
#else
        file=std::fopen(path.c_str(),"wbx");
        const int error=file?0:errno;
#endif
        if(!file)throw std::system_error(error,std::generic_category(),"Cannot create new output (existing files are protected)");
        const bool okay=std::fwrite(bytes.data(),1,bytes.size(),file)==bytes.size();
        const int close=std::fclose(file);
        if(!okay||close!=0){
            std::error_code ignored;
            std::filesystem::remove(path,ignored);
            throw std::runtime_error("Output write failed; partial new file removed");
        }
    }
    void export_midi(const Clip& clip,const std::filesystem::path& path){
        auto bytes=midi_file(clip);
        write_new(path,bytes);
    }
}
// namespace packrat
