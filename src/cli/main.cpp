// SPDX-License-Identifier: GPL-3.0-or-later
#include "packrat/generator.hpp"
#include "packrat/midi.hpp"
#include <algorithm>
#include <charconv>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
namespace {
    using namespace packrat;
    std::filesystem::path path(const std::string& s){
        return std::filesystem::path(std::u8string(s.begin(),s.end()));
    }
    std::uint64_t number(const std::string& s){
        std::uint64_t n=0;
        auto r=std::from_chars(s.data(),s.data()+s.size(),n);
        if(s.empty()||r.ec!=std::errc{}
        ||r.ptr!=s.data()+s.size())throw std::invalid_argument("Expected an unsigned decimal integer: "+s);
        return n;
    }
    void help(){
        std::cout<<"RiffCalc 2000 Pro / Voidmutt Enterprises\n"
        "riffcalc list [--profiles FILE]\n"
        "riffcalc generate --out FILE.mid [options]\n"
        "  --key C|F#|Eb  --scale minor  --genre liquid-dnb\n"
        "  --bars 1..16  --seed UINT64  --complexity 0..100\n"
        "  --bpm 20..400  --low 0..115  --high 12..127  --velocity 1..127\n"
        "  --humanize 0..60 (ticks)  --cadence loop|resolve  --colour\n"
        "  --profiles FILE (validated JSON bank)\n"
        "Default: C natural minor, Liquid DnB, four bars, seed 1, genre tempo.\n"
        "Writes Type 0 MIDI plus reproducibility JSON. Existing files are protected.\n";
    }
    int run(const std::vector<std::string>& args) {
        if(args.size()<2||args[1]=="--help"||args[1]=="help"){
            help();
            return 0;
        }
        if(args[1]!="generate"&&args[1]!="list")throw std::invalid_argument("Expected generate or list; use --help");
        const std::set<std::string> valid{
            "--out","--key","--scale","--genre","--bars","--seed","--complexity","--bpm","--low","--high","--velocity","--humanize","--cadence","--profiles","--colour"
        };
        std::map<std::string,std::string> options;
        for(std::size_t i=2;i<args.size();++i){
            const auto key=args[i];
            if(!valid.contains(key)||options.contains(key))throw std::invalid_argument("Unknown/duplicate option: "+key);
            if(key=="--colour"){
                options[key]="true";
                continue;
            }
            if(i+1>=args.size())throw std::invalid_argument("Missing value for "+key);
            options[key]=args[++i];
        }
        if(args[1]=="list"&&std::any_of(options.begin(),options.end(),[](const auto& x){
            return x.first!="--profiles";
        }))throw std::invalid_argument("list accepts only --profiles");
        const auto bank=options.contains("--profiles")?load_profiles(path(options.at("--profiles"))):bundled_profiles();
        if(args[1]=="list"){
            std::cout<<"Scales:";
            for(const auto& s:scales())std::cout<<" "<<s.id;
            std::cout<<"\nGenres:\n";
            for(const auto& p:bank.profiles){
                std::cout<<"  "<<p.id<<" / "<<p.name<<" / "<<p.bpm_default<<" BPM / preferred:";
                for(auto s:p.preferred_scales)std::cout<<" "<<scale_info(s).id;
                std::cout<<"\n";
            }
            return 0;
        }
        if(!options.contains("--out"))throw std::invalid_argument("generate requires --out FILE.mid");
        const auto genre=options.contains("--genre")?options.at("--genre"):"liquid-dnb";
        const auto& profile=find_profile(bank,genre);
        Config c;
        c.bpm=profile.bpm_default;
        if(options.contains("--key"))c.key=parse_key(options.at("--key"));
        if(options.contains("--scale"))c.scale=parse_scale(options.at("--scale"));
        if(options.contains("--seed"))c.seed=number(options.at("--seed"));
        auto set=[&options](const std::string& key,int& value){
            if(options.contains(key)){
                auto n=number(options.at(key));
                if(n>static_cast<std::uint64_t>(std::numeric_limits<int>::max()))throw std::invalid_argument("Integer too large: "+key);
                value=static_cast<int>(n);
            }
        };
        set("--bars",c.bars);
        set("--complexity",c.complexity);
        set("--bpm",c.bpm);
        set("--low",c.low);
        set("--high",c.high);
        set("--velocity",c.velocity);
        set("--humanize",c.humanize);
        if(options.contains("--cadence")){
            auto v=options.at("--cadence");
            if(v!="loop"&&v!="resolve")throw std::invalid_argument("Cadence must be loop or resolve");
            c.resolve=v=="resolve";
        }
        c.colour=options.contains("--colour");
        const auto result=generate(c,bank,genre);
        auto file=path(options.at("--out"));
        if(file.extension()!=".mid"&&file.extension()!=".midi")throw std::invalid_argument("Output extension must be .mid or .midi");
        auto sidecar=file;
        sidecar.replace_extension(".json");
        auto bytes=midi_file(result.clip);
        auto json=metadata(result);
        write_new(file,bytes);
        try{
            write_new(sidecar,std::span(reinterpret_cast<const std::uint8_t*>(json.data()),json.size()));
        } catch(...){
            std::error_code ignored;
            std::filesystem::remove(file,ignored);
            throw;
        }
        std::cout<<"RiffCalc / "<<engine_version<<" / "<<profile.name<<"\n"<<c.key.name()<<" "<<scale_info(c.scale).id<<" / "<<c.bpm<<" BPM / "<<c.bars<<" bars / seed "<<c.seed<<"\n";
        for(std::size_t i=0;i<result.chords.size();++i){
            const auto& chord=result.chords[i];
            std::cout<<i+1<<": "<<chord.symbol<<" ["<<roman(chord.spec)<<"] ";
            for(auto n:chord.notes)std::cout<<n.midi<<" ";
            std::cout<<"\n";
        }
        std::cout<<"Wrote MIDI and JSON / "<<result.clip.notes.size()<<" notes / "<<result.clip.length<<" ticks\n";
        return 0;
    }
}
#ifdef _WIN32
int wmain(int argc,wchar_t** argv) {
    std::vector<std::string> args;for(int i=0;i<argc;++i){auto u=std::filesystem::path(argv[i]).u8string();args.emplace_back(u.begin(),u.end());}
#else
int main(int argc,char** argv) {
    std::vector<std::string> args(argv,argv+argc);
#endif
    try{return run(args);}catch(const std::exception& e){std::cerr<<"RiffCalc: "<<e.what()<<"\n";return 1;}
}
