// SPDX-License-Identifier: GPL-3.0-or-later
#include "packrat/generator.hpp"
#include "packrat/midi.hpp"
#include <algorithm>
#include <functional>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
using namespace packrat;
static int cases=0;
static void check(bool okay,const char* message){
    if(!okay)throw std::runtime_error(message);
}
template<class F>static void rejects(F f){
    bool threw=false;
    try{
        f();
    } catch(const std::exception&){
        threw=true;
    }
    check(threw,"Expected validation error");
}
static void test(const char* name,const std::function<void()>& f){
    f();
    ++cases;
    std::cout<<"PASS "<<name<<"\n";
}
int main(){
    try{
        const auto bank=bundled_profiles();
        test("all scales and spelling",[]{
            check(scales().size()==10,"Scale count");
            check(scale_info(Scale::HarmonicMinor).intervals==std::vector<int>({
                0,2,3,5,7,8,11
            }),"Harmonic minor");
            check(scale_info(Scale::MelodicMinor).intervals==std::vector<int>({
                0,2,3,5,7,9,11
            }),"Ascending melodic minor");
            check(scale_info(Scale::MinorPentatonic).intervals==std::vector<int>({
                0,3,5,7,10
            }),"Pentatonic");
            for(int root=0;root<12;++root)for(const auto& s:scales()){
                check(s.intervals.front()==0&&std::is_sorted(s.intervals.begin(),s.intervals.end()),"Scale order");
                check(std::set<int>(s.intervals.begin(),s.intervals.end()).size()==s.intervals.size(),"Duplicate scale tone");
                for(int i:s.intervals)check(in_scale({
                    root,'C',0
                },s.scale,root+i),"Scale membership");
            }
            auto key=parse_key("C#");
            check(key.root==1&&key.name()=="C#","Sharp key");
            check(spell_chord(key,Scale::Major,{
                1,0,Quality::Major,false
            })==std::vector<std::string>({
                "C#","E#","G#"
            }),"Enharmonic chord spelling");
            check(spell_chord(parse_key("Eb"),Scale::Minor,{
                3,0,Quality::Diminished,false
            })==std::vector<std::string>({
                "Eb","Gb","Bbb"
            }),"Diminished spelling");
            check(Pitch::from(0,4).midi==60&&Pitch{
                60
            }
            .octave()==4,"MIDI octave");
            rejects([]{
                (void)parse_key("H");
            });
            rejects([]{
                (void)Pitch::from(11,9);
            });
        });
        test("sixteen chord formulas",[]{
            check(formulas().size()==16,"Formula count");
            for(const auto& f:formulas())for(int root=0;root<12;++root){
                auto pcs=chord_classes({
                    root,0,f.quality,false
                });
                check(pcs.size()==f.intervals.size(),"Chord size");
                check(std::set<int>(pcs.begin(),pcs.end()).size()==pcs.size(),"Duplicate chord pitch");
                for(std::size_t i=0;i<pcs.size();++i)check(pcs[i]==pitch_class(root+f.intervals[i]),"Chord interval");
            }
            check(formula(Quality::MinorMajor7).intervals==std::vector<int>({
                0,3,7,11
            }),"Minor-major seventh");
            check(formula(Quality::Dominant7Flat9).intervals==std::vector<int>({
                0,4,7,10,13
            }),"Altered dominant");
        });
        test("PRNG golden vector and rejection sampling",[]{
            Random r(0);
            check(r.next()==UINT64_C(0xe220a8397b1dcdaf),"SplitMix64 first vector");
            check(r.next()==UINT64_C(0x6e789e6aa1b965f4),"SplitMix64 second vector");
            Random a(13),b(13);
            for(int i=0;i<10000;++i)check(a.bounded(37)==b.bounded(37),"Random determinism");
            std::array<std::uint32_t,3> weights{
                0,0,1
            };
            check(a.weighted(weights)==2,"Zero weight selected");
            rejects([&]{
                (void)a.bounded(0);
            });
            check(stage_seeds(42)==stage_seeds(42)&&stage_seeds(42)!=stage_seeds(43),"Stage seeds");
        });
        test("profile errors and canonical identity",[&]{
            check(bank.profiles.size()==12,"Genre count");
            for(const auto& p:bank.profiles)validate(p);
            rejects([]{
                (void)parse_profiles("{}");
            });
            rejects([]{
                (void)parse_profiles("{\"schema\":1,\"schema\":1,\"profiles\":[]}");
            });
            rejects([]{
                (void)parse_profiles("{\"schema\":0,\"profiles\":[{}]}");
            });
            rejects([]{
                (void)parse_profiles(std::string(262145,' '));
            });
            auto p=bank.profiles[0];
            p.transitions[0][0]=0;
            rejects([&]{
                validate(p);
            });
            p=bank.profiles[0];
            p.rhythms[0].steps={
                0,0
            };
            rejects([&]{
                validate(p);
            });
            p=bank.profiles[0];
            p.bpm_min=p.bpm_max+1;
            rejects([&]{
                validate(p);
            });
            auto bad=Config{};
            bad.bars=0;
            rejects([&]{
                (void)generate(bad,bank,"liquid-dnb");
            });
            bad=Config{};
            bad.low=std::numeric_limits<int>::max();
            bad.high=std::numeric_limits<int>::min();
            rejects([&]{
                validate(bad);
            });
            bad=Config{};
            bad.key.accidental=std::numeric_limits<int>::min();
            rejects([&]{
                validate(bad);
            });
        });
        test("determinism, different seeds and stream separation",[&]{
            Config c;
            c.seed=42;
            auto a=generate(c,bank,"liquid-dnb");
            auto b=generate(c,bank,"liquid-dnb");
            check(a==b&&metadata(a)==metadata(b)&&midi_file(a.clip)==midi_file(b.clip),"Repeatability");
            c.seed=43;
            check(generate(c,bank,"liquid-dnb").clip!=a.clip,"Seed variation");
            c=a.config;
            c.velocity=27;
            c.humanize=50;
            auto changed=generate(c,bank,"liquid-dnb");
            check(changed.chords==a.chords,"Velocity/timing changed harmony or voicing");
            auto x=a.config;
            x.resolve=true;
            auto resolved=generate(x,bank,"liquid-dnb");
            check(resolved.chords.back().spec.degree==0,"Resolve cadence");
        });
        test("voice leading improves a root-position phrase",[&]{
            Config c;
            c.key=parse_key("C");
            c.scale=Scale::Major;
            c.low=48;
            c.high=84;
            Random rng(12);
            std::vector<ChordSpec> specs{
                {
                    0,0,Quality::Major,false
                },{
                    9,5,Quality::Minor,false
                },{
                    5,3,Quality::Major,false
                },{
                    7,4,Quality::Major,false
                }
            };
            auto chosen=voice(c,bank.profiles[0],specs,rng);
            std::vector<std::vector<Pitch>> roots{
                {
                    {
                        60
                    },{
                        64
                    },{
                        67
                    }
                },{
                    {
                        69
                    },{
                        72
                    },{
                        76
                    }
                },{
                    {
                        65
                    },{
                        69
                    },{
                        72
                    }
                },{
                    {
                        67
                    },{
                        71
                    },{
                        74
                    }
                }
            };
            int actual=0,baseline=0;
            for(std::size_t i=1;i<chosen.size();++i){
                actual+=movement_cost(chosen[i-1].notes,chosen[i].notes);
                baseline+=movement_cost(roots[i-1],roots[i]);
            }
            check(actual<baseline,"No voice-leading improvement");
        });
        test("complexity changes extension and rhythmic vocabulary",[&]{
            std::size_t low_notes=0,high_notes=0;
            bool extension=false;
            for(std::uint64_t seed=0;seed<40;++seed){
                Config low;
                low.seed=seed;
                low.complexity=0;
                auto high=low;
                high.complexity=100;
                auto a=generate(low,bank,"trance"),b=generate(high,bank,"trance");
                low_notes+=a.clip.notes.size();
                high_notes+=b.clip.notes.size();
                for(const auto& chord:a.chords)check(chord.notes.size()==3,"Complexity-zero extension");
                for(const auto& chord:b.chords)extension=extension||chord.notes.size()>3;
            }
            check(high_notes>low_notes&&extension,"Complexity did not affect musical material");
        });
        test("all 1440 root/scale/genre combinations stay valid",[&]{
            const std::array<const char*,12> keys{
                "C","Db","D","Eb","E","F","F#","G","Ab","A","Bb","B"
            };
            std::size_t index=0;
            for(const auto& key:keys)for(const auto& scale:scales())for(const auto& profile:bank.profiles){
                Config c;
                c.key=parse_key(key);
                c.scale=scale.scale;
                c.complexity=static_cast<int>(index%3)*50;
                c.seed=1000+index++;
                c.humanize=60;
                c.low=36;
                c.high=84;
                c.resolve=true;
                auto r=generate(c,bank,profile.id);
                validate(r.clip);
                check(r.chords.size()==4&&r.clip.length==15360&&!r.clip.notes.empty(),"Phrase shape");
                for(const auto& n:r.clip.notes)check(n.pitch>=c.low&&n.pitch<=c.high&&in_scale(c.key,c.scale,n.pitch),"Out-of-key/range note");
                for(const auto& chord:r.chords){
                    auto pcs=chord_classes(chord.spec);
                    for(auto n:chord.notes)check(std::find(pcs.begin(),pcs.end(),n.pc())!=pcs.end(),"Voicing changed chord");
                }
                check(r.chords.back().spec.degree==0,"Tonic cadence");
            }
            check(index==1440,"Property case count");
        });
        test("one to sixteen bars and edge registers",[&]{
            for(int bars=1;bars<=16;++bars){
                Config c;
                c.bars=bars;
                c.seed=static_cast<std::uint64_t>(bars);
                c.low=bars%2?0:115;
                c.high=c.low+12;
                c.complexity=100;
                auto r=generate(c,bank,"ambient");
                validate(r.clip);
                check(r.clip.length==bars*bar_ticks&&r.chords.size()==static_cast<std::size_t>(bars),"Bar length");
                for(auto n:r.clip.notes)check(n.pitch>=c.low&&n.pitch<=c.high,"Edge register");
            }
        });
        test("chromatic colour is explicit",[&]{
            bool found=false;
            for(std::uint64_t seed=0;seed<80&&!found;++seed){
                Config c;
                c.seed=seed;
                c.complexity=80;
                c.colour=true;
                auto r=generate(c,bank,"hard-dance");
                for(const auto& chord:r.chords)for(auto n:chord.notes)if(!in_scale(c.key,c.scale,n.midi)){
                    check(chord.spec.chromatic&&chord.spec.quality==Quality::Dominant7&&roman(chord.spec).ends_with('*'),"Unmarked chromatic note");
                    found=true;
                }
            }
            check(found,"Colour option never acted");
        });
        test("MIDI empty/short/retrigger and validation",[]{
            Clip c;
            c.length=960;
            c.notes={
                {
                    0,1,60,127,0
                },{
                    1,1,60,1,0
                }
            };
            auto a=midi_file(c);
            check(a.size()>30&&std::string(a.begin(),a.begin()+4)=="MThd","SMF header");
            c.notes.clear();
            check(midi_file(c).size()>30,"Empty clip");
            c.notes={
                {
                    0,2,60,1,0
                },{
                    1,2,60,1,0
                }
            };
            rejects([&]{
                (void)midi_file(c);
            });
            c.notes={
                {
                    0,0,60,1,0
                }
            };
            rejects([&]{
                validate(c);
            });
            c.notes={
                {
                    std::numeric_limits<Tick>::max(),2,60,1,0
                }
            };
            rejects([&]{
                validate(c);
            });
            c.notes.clear();
            c.length=bar_ticks*1024;
            c.meter={
                6,8
            };
            auto long_file=midi_file(c);
            const std::vector<std::uint8_t> tail{
                0x81,0xf0,0x80,0,0xff,0x2f,0
            };
            check(std::equal(tail.rbegin(),tail.rend(),long_file.rbegin()),"Four-byte delta/EOT");
            const std::vector<std::uint8_t> meter{
                0xff,0x58,4,6,3,24,8
            };
            check(std::search(long_file.begin(),long_file.end(),meter.begin(),meter.end())!=long_file.end(),"6/8 meter encoding");
        });
        std::cout<<cases<<" test groups passed (including 1440 exhaustive generation cases).\n";
        return 0;
    } catch(const std::exception& e){
        std::cerr<<"FAIL: "<<e.what()<<"\n";
        return 1;
    }
}
