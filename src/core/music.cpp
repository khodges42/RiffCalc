// SPDX-License-Identifier: GPL-3.0-or-later
#include "packrat/music.hpp"
#include <algorithm>
#include <cstdlib>
#include <map>
#include <stdexcept>
#include <tuple>
namespace packrat {
    int pitch_class(int note) noexcept {
        return (note % 12 + 12) % 12;
    }
    Pitch Pitch::from(int pc,int octave) {
        if(pc<0||pc>11||octave< -1||octave>9) throw std::invalid_argument("Invalid pitch class/octave");
        int n=(octave+1)*12+pc;
        if(n>127) throw std::invalid_argument("Pitch exceeds MIDI 127");
        return {
            n
        };
    }
    static int natural(char c) {
        switch(c) {
            case 'C':return 0;
            case 'D':return 2;
            case 'E':return 4;
            case 'F':return 5;
            case 'G':return 7;
            case 'A':return 9;
            case 'B':return 11;
            default:throw std::invalid_argument("Root letter must be A..G");
        }
    }
    std::string Key::name() const {
        if(accidental< -1||accidental>1)throw std::invalid_argument("Key accidental must be flat, natural or sharp");
        std::string s(1,letter);
        s.append(static_cast<std::size_t>(std::abs(accidental)),accidental<0?'b':'#');
        return s;
    }
    Key parse_key(std::string_view s) {
        if(s.empty()||s.size()>2) throw std::invalid_argument("Key must be a note such as C, F# or Eb");
        char letter=s[0];
        if(letter>='a'&&letter<='g') letter=static_cast<char>(letter-'a'+'A');
        int a=0;
        if(s.size()==2) {
            if(s[1]=='#') a=1;
            else if(s[1]=='b') a=-1;
            else throw std::invalid_argument("Unknown accidental");
        }
        return {
            pitch_class(natural(letter)+a),letter,a
        };
    }
    const std::array<ScaleInfo,10>& scales() {
        static const std::array<ScaleInfo,10> data{
            {
                {
                    Scale::Major,"major",{
                        0,2,4,5,7,9,11
                    }
                }, {
                    Scale::Minor,"minor",{
                        0,2,3,5,7,8,10
                    }
                },
                {
                    Scale::HarmonicMinor,"harmonic-minor",{
                        0,2,3,5,7,8,11
                    }
                },
                {
                    Scale::MelodicMinor,"melodic-minor",{
                        0,2,3,5,7,9,11
                    }
                },
                {
                    Scale::Dorian,"dorian",{
                        0,2,3,5,7,9,10
                    }
                }, {
                    Scale::Phrygian,"phrygian",{
                        0,1,3,5,7,8,10
                    }
                },
                {
                    Scale::Mixolydian,"mixolydian",{
                        0,2,4,5,7,9,10
                    }
                }, {
                    Scale::Lydian,"lydian",{
                        0,2,4,6,7,9,11
                    }
                },
                {
                    Scale::Locrian,"locrian",{
                        0,1,3,5,6,8,10
                    }
                }, {
                    Scale::MinorPentatonic,"minor-pentatonic",{
                        0,3,5,7,10
                    }
                }
            }
        };
        return data;
    }
    const ScaleInfo& scale_info(Scale s) {
        for(const auto& x:scales()) if(x.scale==s) return x;
        throw std::invalid_argument("Unknown scale enum");
    }
    Scale parse_scale(std::string_view s) {
        if(s=="natural-minor") return Scale::Minor;
        for(const auto& x:scales()) if(x.id==s) return x.scale;
        throw std::invalid_argument("Unknown scale: "+std::string(s));
    }
    bool in_scale(Key k,Scale s,int note) {
        const auto& v=scale_info(s).intervals;
        return std::find(v.begin(),v.end(),pitch_class(note-k.root))!=v.end();
    }
    int degree_pc(Key k,Scale s,int degree) {
        if(degree<0||degree>6) throw std::invalid_argument("Degree must be 0..6");
        const auto& v=scale_info(s==Scale::MinorPentatonic?Scale::Minor:s).intervals;
        return pitch_class(k.root+v[static_cast<std::size_t>(degree)]);
    }
    static std::string spelling(char root,int steps,int target) {
        constexpr std::string_view letters="CDEFGAB";
        const auto i=letters.find(root);
        if(i==std::string_view::npos) throw std::invalid_argument("Invalid spelling root");
        char letter=letters[(i+static_cast<std::size_t>(steps))%7];
        int accidental=pitch_class(target-natural(letter));
        if(accidental>6) accidental-=12;
        std::string name(1,letter);
        name.append(static_cast<std::size_t>(std::abs(accidental)),accidental<0?'b':'#');
        return name;
    }
    std::string spell_degree(Key k,Scale s,int degree) {
        return spelling(k.letter,degree,degree_pc(k,s,degree));
    }
    const std::array<Formula,16>& formulas() {
        static const std::array<Formula,16> data{
            {
                {
                    Quality::Major,"major","",{
                        0,4,7
                    },{
                        0,2,4
                    },0
                },
                {
                    Quality::Minor,"minor","m",{
                        0,3,7
                    },{
                        0,2,4
                    },0
                },
                {
                    Quality::Diminished,"diminished","dim",{
                        0,3,6
                    },{
                        0,2,4
                    },0
                },
                {
                    Quality::Augmented,"augmented","+",{
                        0,4,8
                    },{
                        0,2,4
                    },0
                },
                {
                    Quality::Sus2,"sus2","sus2",{
                        0,2,7
                    },{
                        0,1,4
                    },0
                },
                {
                    Quality::Sus4,"sus4","sus4",{
                        0,5,7
                    },{
                        0,3,4
                    },0
                },
                {
                    Quality::Dominant7,"dominant7","7",{
                        0,4,7,10
                    },{
                        0,2,4,6
                    },30
                },
                {
                    Quality::Major7,"major7","maj7",{
                        0,4,7,11
                    },{
                        0,2,4,6
                    },30
                },
                {
                    Quality::Minor7,"minor7","m7",{
                        0,3,7,10
                    },{
                        0,2,4,6
                    },30
                },
                {
                    Quality::MinorMajor7,"minor-major7","m(maj7)",{
                        0,3,7,11
                    },{
                        0,2,4,6
                    },40
                },
                {
                    Quality::Minor9,"minor9","m9",{
                        0,3,7,10,14
                    },{
                        0,2,4,6,8
                    },65
                },
                {
                    Quality::Major9,"major9","maj9",{
                        0,4,7,11,14
                    },{
                        0,2,4,6,8
                    },65
                },
                {
                    Quality::Add9,"add9","add9",{
                        0,4,7,14
                    },{
                        0,2,4,8
                    },45
                },
                {
                    Quality::Major6,"major6","6",{
                        0,4,7,9
                    },{
                        0,2,4,5
                    },35
                },
                {
                    Quality::Minor6,"minor6","m6",{
                        0,3,7,9
                    },{
                        0,2,4,5
                    },35
                },
                {
                    Quality::Dominant7Flat9,"dominant7-flat9","7(b9)",{
                        0,4,7,10,13
                    },{
                        0,2,4,6,8
                    },80
                }
            }
        };
        return data;
    }
    const Formula& formula(Quality q) {
        for(const auto& x:formulas()) if(x.quality==q)return x;
        throw std::invalid_argument("Unknown chord enum");
    }
    Quality parse_quality(std::string_view s) {
        for(const auto& x:formulas())if(x.id==s)return x.quality;
        throw std::invalid_argument("Unknown chord quality: "+std::string(s));
    }
    std::vector<int> chord_classes(const ChordSpec& c) {
        std::vector<int> r;
        for(int i:formula(c.quality).intervals)r.push_back(pitch_class(c.root+i));
        return r;
    }
    std::vector<std::string> spell_chord(Key k,Scale s,const ChordSpec& c) {
        const auto& f=formula(c.quality);
        const auto root=spell_degree(k,s,c.degree);
        std::vector<std::string> result;
        for(std::size_t i=0;i<f.intervals.size();++i) result.push_back(spelling(root[0],f.letter_steps[i],pitch_class(c.root+f.intervals[i])));
        return result;
    }
    std::string roman(const ChordSpec& c) {
        static const std::array<std::string,7> upper{
            "I","II","III","IV","V","VI","VII"
        };
        static const std::array<std::string,7> lower{
            "i","ii","iii","iv","v","vi","vii"
        };
        bool minor=c.quality==Quality::Minor||c.quality==Quality::Minor7||c.quality==Quality::Minor9||c.quality==Quality::Minor6||c.quality==Quality::MinorMajor7||c.quality==Quality::Diminished;
        auto r=(minor?lower:upper).at(static_cast<std::size_t>(c.degree));
        auto suffix=std::string(formula(c.quality).suffix);
        if(minor&&suffix.starts_with('m')&&!suffix.starts_with("maj"))suffix.erase(0,1);
        return r+suffix+(c.chromatic?"*":"");
    }
    void validate(const Clip& c) {
        if(c.name.empty()||c.name.size()>128||c.name.find('\0')!=std::string::npos)throw std::invalid_argument("Track name requires 1..128 non-NUL bytes");
        if(c.length<1||c.length>bar_ticks*1024)throw std::invalid_argument("Clip length outside supported range");
        if(c.bpm<20||c.bpm>400)throw std::invalid_argument("Tempo must be 20..400 BPM");
        if(c.meter.numerator<1||c.meter.numerator>32||c.meter.denominator<1||c.meter.denominator>32||(c.meter.denominator&(c.meter.denominator-1))!=0)throw std::invalid_argument("Invalid time signature");
        if(c.notes.size()>65536)throw std::invalid_argument("Too many notes");
        auto sorted=c.notes;
        std::sort(sorted.begin(),sorted.end(),[](const Note&a,const Note&b){
            return std::tie(a.channel,a.pitch,a.start)<std::tie(b.channel,b.pitch,b.start);
        });
        std::map<std::pair<int,int>,Tick> ends;
        for(const auto& n:sorted) {
            if(n.pitch<0||n.pitch>127||n.velocity<1||n.velocity>127||n.channel<0||n.channel>15||n.start<0||n.duration<1||n.start>=c.length||n.duration>c.length-n.start)throw std::invalid_argument("Invalid MIDI note or timing");
            auto key=std::pair(n.channel,n.pitch);
            if(ends.contains(key)&&ends[key]>n.start)throw std::invalid_argument("Overlapping note on the same channel/pitch");
            ends[key]=n.start+n.duration;
        }
    }
}
// namespace packrat
