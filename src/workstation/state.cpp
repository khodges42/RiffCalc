// SPDX-License-Identifier: GPL-3.0-or-later
#include "packrat/workstation.hpp"
#include <nlohmann/json.hpp>
#include <charconv>
#include <cmath>
#include <set>
#include <stdexcept>
namespace packrat {

    using Json = nlohmann::json;

    namespace {

        int integer(const Json& j,int low,int high) {

            if(!j.is_number_integer() || j < low || j > high) throw std::invalid_argument("State integer outside range");

            return j.get<int>();

        }

        std::uint64_t seed_value(const Json& j) {

            const auto text=j.get<std::string>();
            std::uint64_t value=0;

            auto [end,error]=std::from_chars(text.data(),text.data()+text.size(),value);

            if(error!=std::errc{

            }
            || end!=text.data()+text.size() || text.empty()) throw std::invalid_argument("Invalid seed");

            return value;

        }

        std::vector<Note> notes_from(const Json& array) {

            if(!array.is_array() || array.size()>4096)throw std::invalid_argument("Too many notes");

            std::vector<Note> notes;

            for(const auto& n:array) notes.push_back({
                integer(n.at("start"),0,16*static_cast<int>(bar_ticks)),integer(n.at("duration"),1,16*static_cast<int>(bar_ticks)),integer(n.at("pitch"),0,127),integer(n.at("velocity"),1,127),integer(n.at("channel"),0,15)
            });

            return notes;

        }

    }

    std::string serialize(const Document& d) {

        validate(d.idea);

        Json j={
            {
                "schema",1
            },{
                "workstation",workstation_version
            },{
                "harmony",Json::parse(metadata(d.idea.harmony))
            },
            {
                "part",static_cast<int>(d.part)
            },{
                "sound",d.sound
            },{
                "output_db",d.output_db
            },{
                "arp",static_cast<int>(d.idea.arp)
            },
            {
                "locks",{
                    d.locks.harmony,d.locks.voicing,d.locks.rhythm
                }

            },{
                "follow_host",d.follow_host
            }

        };

        j["parts"]=Json::array();

        for(const auto& c:d.idea.parts) {

            Json part={
                {
                    "name",c.name
                },{
                    "notes",Json::array()
                }

            };

            for(const auto& n:c.notes)part["notes"].push_back({
                {
                    "start",n.start
                },{
                    "duration",n.duration
                },{
                    "pitch",n.pitch
                },{
                    "velocity",n.velocity
                },{
                    "channel",n.channel
                }

            });

            j["parts"].push_back(part);

        }

        return j.dump(2)+"\n";

    }

    Document deserialize(std::string_view text,const ProfileBank& bank) {

        if(text.size()>2*1024*1024)throw std::invalid_argument("Session exceeds 2 MiB");

        int depth=0;

        std::vector<std::set<std::string>> keys;

        auto j=Json::parse(text,[&](int,Json::parse_event_t event,Json& value){

            if(event==Json::parse_event_t::object_start)keys.emplace_back();

            if(event==Json::parse_event_t::key&&!keys.back().insert(value.get<std::string>()).second)throw std::invalid_argument("Duplicate session key");

            if(event==Json::parse_event_t::object_end)keys.pop_back();

            if(event==Json::parse_event_t::object_start || event==Json::parse_event_t::array_start)if(++depth>24)throw std::invalid_argument("State nesting too deep");

            if(event==Json::parse_event_t::object_end || event==Json::parse_event_t::array_end)--depth;

            return true;

        });

        if(integer(j.at("schema"),1,1)!=1 || j.at("workstation")!=std::string(workstation_version) || j.size()!=10)throw std::invalid_argument("Unsupported PACKRAT session version");

        Document d;
        auto& r=d.idea.harmony;

        const auto& h=j.at("harmony");
        const auto& c=h.at("config");

        if(h.at("engine")!=std::string(engine_version))throw std::invalid_argument("Unsupported chord engine version");

        r.config.key=parse_key(c.at("key").get<std::string>());
        r.config.scale=parse_scale(c.at("scale").get<std::string>());

        r.config.bars=integer(c.at("bars"),1,16);
        r.config.complexity=integer(c.at("complexity"),0,100);
        r.config.seed=seed_value(c.at("seed"));

        r.config.low=integer(c.at("low"),0,127);
        r.config.high=integer(c.at("high"),0,127);
        r.config.bpm=integer(c.at("bpm"),20,400);

        r.config.velocity=integer(c.at("velocity"),1,127);
        r.config.humanize=integer(c.at("humanize_ticks"),0,60);

        const auto cadence=c.at("cadence").get<std::string>();
        if(cadence!="resolve"&&cadence!="loop")throw std::invalid_argument("Invalid cadence");

        r.config.resolve=cadence=="resolve";
        r.config.colour=c.at("colour").get<bool>();
        validate(r.config);

        r.seeds=stage_seeds(r.config.seed);
        r.genre=h.at("profile_id").get<std::string>();
        (void)find_profile(bank,r.genre);

        r.profile_version=integer(h.at("profile_version"),1,1000000);
        r.profile_fingerprint=h.at("profile_bank_fingerprint").get<std::string>();

        if(r.profile_fingerprint.empty()||r.profile_fingerprint.size()>128)throw std::invalid_argument("Invalid profile identifier");

        if(h.at("ppq")!=ppq || h.at("length_ticks")!=r.config.bars*bar_ticks || h.at("meter")!=Json::array({4,4}) || h.at("profile_schema")!=1)
            throw std::invalid_argument("Unsupported or inconsistent musical timing");
        r.clip.length=r.config.bars*bar_ticks;
        r.clip.bpm=r.config.bpm;
        r.clip.notes=notes_from(h.at("notes"));

        const auto& chords=h.at("chords");
        if(!chords.is_array()||chords.size()!=static_cast<std::size_t>(r.config.bars))throw std::invalid_argument("Chord count mismatch");

        for(const auto& a:chords) {

            Chord chord;
            chord.start=integer(a.at("start"),0,16*static_cast<int>(bar_ticks));
            chord.length=integer(a.at("length"),1,static_cast<int>(bar_ticks));

            chord.spec={
                integer(a.at("root_pc"),0,11),integer(a.at("degree"),1,7)-1,parse_quality(a.at("quality").get<std::string>()),a.at("chromatic").get<bool>()
            };

            chord.inversion=integer(a.at("inversion"),0,4);
            chord.symbol=a.at("symbol").get<std::string>();

            if(chord.symbol.size()>64 || !a.at("pitches").is_array() || a.at("pitches").size()>5)throw std::invalid_argument("Invalid chord fields");

            for(const auto& pitch:a.at("pitches"))chord.notes.push_back({
                integer(pitch,0,127)
            });

            r.chords.push_back(chord);

        }

        const auto& parts=j.at("parts");
        if(!parts.is_array()||parts.size()!=4)throw std::invalid_argument("Expected four parts");

        for(std::size_t i=0;i<4;++i) {

            auto& clip=d.idea.parts[i];
            clip.name=parts[i].at("name").get<std::string>();
            clip.length=r.clip.length;
            clip.bpm=r.clip.bpm;
            clip.notes=notes_from(parts[i].at("notes"));

        }

        r.clip.name=d.idea.parts[0].name;

        d.part=static_cast<Part>(integer(j.at("part"),0,4));
        d.sound=integer(j.at("sound"),0,9);
        d.output_db=j.at("output_db").get<float>();
        if(!std::isfinite(d.output_db)||d.output_db< -48||d.output_db>0)throw std::invalid_argument("Invalid preview output");
        d.idea.arp=static_cast<ArpOrder>(integer(j.at("arp"),0,3));

        const auto& locks=j.at("locks");
        if(!locks.is_array()||locks.size()!=3)throw std::invalid_argument("Invalid locks");

        d.locks={
            locks[0].get<bool>(),locks[1].get<bool>(),locks[2].get<bool>()
        };
        if(d.locks.voicing)d.locks.harmony=true;

        d.follow_host=j.at("follow_host").get<bool>();
        validate(d.idea);
        return d;

    }

}
