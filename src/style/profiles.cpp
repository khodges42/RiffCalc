// SPDX-License-Identifier: GPL-3.0-or-later
#include "packrat/generator.hpp"
#include "bundled.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <locale>
#include <set>
#include <sstream>
#include <stdexcept>
namespace packrat {
    using Json=nlohmann::json;
    static void fields(const Json& j,std::initializer_list<std::string_view> names) {
        if(!j.is_object())throw std::invalid_argument("Expected an object");
        for(const auto& [key,value]:j.items()) {
            (void)value;
            if(std::find(names.begin(),names.end(),key)==names.end())throw std::invalid_argument("Unknown field: "+key);
        }
        for(auto name:names)if(!j.contains(name))throw std::invalid_argument("Missing field: "+std::string(name));
    }
    static int integer(const Json& j,int low,int high) {
        if(!j.is_number_integer())throw std::invalid_argument("Expected an integer");
        if(j.is_number_unsigned()){
            auto n=j.get<std::uint64_t>();
            if(high<0||n>static_cast<std::uint64_t>(high)||(low>0&&n<static_cast<std::uint64_t>(low)))throw std::invalid_argument("Integer out of range");
        } else if(j.get<std::int64_t>()<low||j.get<std::int64_t>()>high)throw std::invalid_argument("Integer out of range");
        return j.get<int>();
    }
    static std::string text(const Json& j,std::size_t maximum) {
        if(!j.is_string())throw std::invalid_argument("Expected a string");
        auto s=j.get<std::string>();
        if(s.empty()||s.size()>maximum||std::any_of(s.begin(),s.end(),[](unsigned char x){
            return x<32;
        }))throw std::invalid_argument("Invalid string length/control character");
        return s;
    }
    static void array(const Json& j,std::size_t minimum,std::size_t maximum) {
        if(!j.is_array()||j.size()<minimum||j.size()>maximum)throw std::invalid_argument("Array length out of range");
    }
    void validate(const GenreProfile& p) {
        if(p.id.empty()||p.id.size()>32||!std::all_of(p.id.begin(),p.id.end(),[](char c){
            return (c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-';
        }))throw std::invalid_argument("Genre id must use lowercase ASCII, numbers and hyphens");
        if(p.name.empty()||p.name.size()>64||p.version<1||p.version>1000000)throw std::invalid_argument("Invalid genre name/version");
        if(p.preferred_scales.empty()||p.preferred_scales.size()>10)throw std::invalid_argument("Genre needs 1..10 preferred scales");
        for(auto s:p.preferred_scales)(void)scale_info(s);
        if(p.bpm_min<20||p.bpm_max>400||p.bpm_min>p.bpm_default||p.bpm_default>p.bpm_max)throw std::invalid_argument("Invalid min/default/max tempo");
        if(p.center<0||p.center>127||p.spread<0||p.spread>100||p.repetition<0||p.repetition>100||p.modal<0||p.modal>100||p.colour<0||p.colour>100||p.swing<0||p.swing>45||p.velocity_spread<0||p.velocity_spread>20)throw std::invalid_argument("Genre control outside range");
        if(p.patterns.empty()||p.patterns.size()>32)throw std::invalid_argument("Genre needs 1..32 progression patterns");
        for(const auto& x:p.patterns)if(x.weight<1||x.weight>10000||x.degrees.empty()||x.degrees.size()>16||!std::all_of(x.degrees.begin(),x.degrees.end(),[](int d){
            return d>=1&&d<=7;
        }))throw std::invalid_argument("Invalid progression pattern");
        for(const auto& row:p.transitions){
            if(!row[0])throw std::invalid_argument("Every transition row needs a tonic fallback");
            for(auto w:row)if(w>10000)throw std::invalid_argument("Transition weight too large");
        }
        if(!p.cadence[0])throw std::invalid_argument("Cadence needs a tonic fallback");
        for(auto w:p.cadence)if(w>10000)throw std::invalid_argument("Cadence weight too large");
        std::set<Quality> qualities;
        for(const auto& q:p.qualities){
            (void)formula(q.quality);
            if(!q.weight||q.weight>10000||!qualities.insert(q.quality).second)throw std::invalid_argument("Invalid/duplicate quality weight");
        }
        if(p.qualities.empty()||p.qualities.size()>16||p.rhythms.empty()||p.rhythms.size()>8)throw std::invalid_argument("Missing or excessive quality/rhythm data");
        bool base=false;
        std::set<std::string> ids;
        for(const auto& r:p.rhythms){
            if(r.id.empty()||r.id.size()>32||!ids.insert(r.id).second||r.steps.empty()||r.steps.size()>16||r.gate_steps<1||r.gate_steps>16||r.min_complexity<0||r.min_complexity>100||!r.weight||r.weight>10000)throw std::invalid_argument("Invalid rhythm pattern");
            int previous=-1;
            for(int step:r.steps){
                if(step<=previous||step>15)throw std::invalid_argument("Rhythm steps must be unique, sorted and within 0..15");
                previous=step;
            }
            base=base||r.min_complexity==0;
        }
        if(!base)throw std::invalid_argument("A rhythm must be available at complexity zero");
        for(int x:p.accents)if(x< -20||x>20)throw std::invalid_argument("Accent outside -20..20");
        if(p.preview.empty()||p.preview.size()>32)throw std::invalid_argument("Invalid preview hint");
    }
    ProfileBank parse_profiles(std::string_view source) {
        if(source.empty()||source.size()>262144)throw std::invalid_argument("Genre bank requires 1..262144 bytes");
        try {
            std::vector<std::set<std::string>> objects;
            auto json=Json::parse(source,[&objects](int depth,Json::parse_event_t event,Json& value){
                if(depth>32)throw std::invalid_argument("JSON nesting exceeds 32");
                if(event==Json::parse_event_t::object_start)objects.emplace_back();
                if(event==Json::parse_event_t::key&&!objects.back().insert(value.get<std::string>()).second)throw std::invalid_argument("Duplicate JSON key");
                if(event==Json::parse_event_t::object_end)objects.pop_back();
                return true;
            });
            fields(json,{
                "schema","profiles"
            });
            (void)integer(json.at("schema"),1,1);
            array(json.at("profiles"),1,64);
            ProfileBank bank;
            std::set<std::string> ids;
            std::size_t index=0;
            for(const auto& j:json.at("profiles")) {
                try {
                    fields(j,{
                        "id","name","version","scales","tempo","patterns","transitions","cadence","qualities","rhythms","center","spread","repetition","modal","colour","swing","velocity_spread","accents","preview"
                    });
                    GenreProfile p;
                    p.id=text(j.at("id"),32);
                    p.name=text(j.at("name"),64);
                    p.version=integer(j.at("version"),1,1000000);
                    array(j.at("scales"),1,10);
                    std::set<Scale> scale_ids;
                    for(const auto& s:j.at("scales")){
                        auto x=parse_scale(text(s,32));
                        if(!scale_ids.insert(x).second)throw std::invalid_argument("Duplicate scale");
                        p.preferred_scales.push_back(x);
                    }
                    const auto& tempo=j.at("tempo");
                    array(tempo,3,3);
                    p.bpm_min=integer(tempo[0],20,400);
                    p.bpm_default=integer(tempo[1],20,400);
                    p.bpm_max=integer(tempo[2],20,400);
                    array(j.at("patterns"),1,32);
                    for(const auto& pattern:j.at("patterns")){
                        fields(pattern,{
                            "degrees","weight"
                        });
                        array(pattern.at("degrees"),1,16);
                        ProgressionPattern x;
                        for(const auto& d:pattern.at("degrees"))x.degrees.push_back(integer(d,1,7));
                        x.weight=static_cast<std::uint32_t>(integer(pattern.at("weight"),1,10000));
                        p.patterns.push_back(std::move(x));
                    }
                    array(j.at("transitions"),7,7);
                    for(std::size_t i=0;i<7;++i){
                        array(j.at("transitions")[i],7,7);
                        for(std::size_t k=0;k<7;++k)p.transitions[i][k]=static_cast<std::uint32_t>(integer(j.at("transitions")[i][k],0,10000));
                    }
                    array(j.at("cadence"),7,7);
                    for(std::size_t i=0;i<7;++i)p.cadence[i]=static_cast<std::uint32_t>(integer(j.at("cadence")[i],0,10000));
                    array(j.at("qualities"),1,16);
                    for(const auto& x:j.at("qualities")){
                        fields(x,{
                            "id","weight"
                        });
                        p.qualities.push_back({
                            parse_quality(text(x.at("id"),32)),static_cast<std::uint32_t>(integer(x.at("weight"),1,10000))
                        });
                    }
                    array(j.at("rhythms"),1,8);
                    for(const auto& x:j.at("rhythms")){
                        fields(x,{
                            "id","steps","gate_steps","min_complexity","weight"
                        });
                        RhythmPattern r;
                        r.id=text(x.at("id"),32);
                        array(x.at("steps"),1,16);
                        for(const auto& v:x.at("steps"))r.steps.push_back(integer(v,0,15));
                        r.gate_steps=integer(x.at("gate_steps"),1,16);
                        r.min_complexity=integer(x.at("min_complexity"),0,100);
                        r.weight=static_cast<std::uint32_t>(integer(x.at("weight"),1,10000));
                        p.rhythms.push_back(std::move(r));
                    }
                    p.center=integer(j.at("center"),0,127);
                    p.spread=integer(j.at("spread"),0,100);
                    p.repetition=integer(j.at("repetition"),0,100);
                    p.modal=integer(j.at("modal"),0,100);
                    p.colour=integer(j.at("colour"),0,100);
                    p.swing=integer(j.at("swing"),0,45);
                    p.velocity_spread=integer(j.at("velocity_spread"),0,20);
                    array(j.at("accents"),16,16);
                    for(std::size_t i=0;i<16;++i)p.accents[i]=integer(j.at("accents")[i],-20,20);
                    p.preview=text(j.at("preview"),32);
                    validate(p);
                    if(!ids.insert(p.id).second)throw std::invalid_argument("Duplicate genre id");
                    bank.profiles.push_back(std::move(p));
                } catch(const std::exception& e){
                    throw std::invalid_argument("profiles["+std::to_string(index)+"]: "+e.what());
                }
                ++index;
            }
            const auto canonical=json.dump();
            std::uint64_t h=UINT64_C(14695981039346656037);
            for(unsigned char x:canonical){
                h^=x;
                h*=UINT64_C(1099511628211);
            }
            std::ostringstream ss;
        ss.imbue(std::locale::classic());
        ss<<"fnv1a64:"<<std::hex<<std::setfill('0')<<std::setw(16)<<h;
            bank.fingerprint=ss.str();
            return bank;
        } catch(const std::exception& e){
            throw std::invalid_argument("Genre bank: "+std::string(e.what()));
        }
    }
    ProfileBank load_profiles(const std::filesystem::path& path) {
        std::ifstream f(path,std::ios::binary);
        if(!f)throw std::invalid_argument("Cannot open genre bank: "+path.string());
        std::string text;
        char buffer[4096];
        while(f.read(buffer,sizeof buffer)||f.gcount()>0){
            text.append(buffer,static_cast<std::size_t>(f.gcount()));
            if(text.size()>262144)throw std::invalid_argument("Genre bank exceeds 256 KiB");
        }
        if(f.bad())throw std::runtime_error("Error reading genre bank");
        return parse_profiles(text);
    }
    ProfileBank bundled_profiles(){
        std::string text;
        for(auto chunk:bundled_genres_json)text+=chunk;
        return parse_profiles(text);
    }
    const GenreProfile& find_profile(const ProfileBank& bank,std::string_view id) {
        for(const auto& p:bank.profiles)if(p.id==id)return p;
        throw std::invalid_argument("Unknown genre: "+std::string(id));
    }
    std::string metadata(const Result& r) {
        const auto& c=r.config;
        Json j={
            {
                "engine",engine_version
            },{
                "profile_schema",1
            },{
                "profile_id",r.genre
            },{
                "profile_version",r.profile_version
            },{
                "profile_bank_fingerprint",r.profile_fingerprint
            },
            {
                "config",{
                    {
                        "key",c.key.name()
                    },{
                        "scale",scale_info(c.scale).id
                    },{
                        "bars",c.bars
                    },{
                        "complexity",c.complexity
                    },{
                        "seed",std::to_string(c.seed)
                    },{
                        "low",c.low
                    },{
                        "high",c.high
                    },{
                        "bpm",c.bpm
                    },{
                        "velocity",c.velocity
                    },{
                        "humanize_ticks",c.humanize
                    },{
                        "cadence",c.resolve?"resolve":"loop"
                    },{
                        "colour",c.colour
                    }
                }
            },
            {
                "ppq",ppq
            },{
                "length_ticks",r.clip.length
            },{
                "meter",{
                    r.clip.meter.numerator,r.clip.meter.denominator
                }
            },
            {
                "stage_seeds",{
                    {
                        "harmony",std::to_string(r.seeds.harmony)
                    },{
                        "voicing",std::to_string(r.seeds.voicing)
                    },{
                        "rhythm",std::to_string(r.seeds.rhythm)
                    },{
                        "timing",std::to_string(r.seeds.timing)
                    },{
                        "velocity",std::to_string(r.seeds.velocity)
                    }
                }
            }
        };
        j["chords"]=Json::array();
        for(const auto& chord:r.chords){
            std::vector<int> notes;
            for(auto n:chord.notes)notes.push_back(n.midi);
            j["chords"].push_back({
                {
                    "symbol",chord.symbol
                },{
                    "roman",roman(chord.spec)
                },{
                    "degree",chord.spec.degree+1
                },{
                    "root_pc",chord.spec.root
                },{
                    "quality",formula(chord.spec.quality).id
                },{
                    "chromatic",chord.spec.chromatic
                },{
                    "spelling",spell_chord(c.key,c.scale,chord.spec)
                },{
                    "inversion",chord.inversion
                },{
                    "pitches",notes
                },{
                    "start",chord.start
                },{
                    "length",chord.length
                }
            });
        }
        j["notes"]=Json::array();
        for(const auto& n:r.clip.notes)j["notes"].push_back({
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
        return j.dump(2)+"\n";
    }
}
// namespace packrat
