// SPDX-License-Identifier: GPL-3.0-or-later
#include "packrat/workstation.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <tuple>
namespace packrat {

    namespace {

        int fit(int pc, int low, int high, int target) {

            int best = low, distance = 1000;

            for (int n = low; n <= high; ++n)
            if (pitch_class(n) == pitch_class(pc) && std::abs(n-target) < distance) {

                best = n;
                distance = std::abs(n-target);

            }

            return best;

        }

        void sort_notes(Clip& c) {

            std::sort(c.notes.begin(),c.notes.end(),[](auto a,auto b) {

                return std::tie(a.start,a.pitch,a.channel)<std::tie(b.start,b.pitch,b.channel);

            });

            validate(c);

        }

        void apply_rhythm(Result& result, const Result& old) {

            const auto old_notes = old.clip.notes;

            result.clip.notes.clear();

            for (const auto& chord : result.chords) {

                std::vector<std::pair<Tick,Tick>> rhythm;

                for (const auto& n : old_notes) if (n.start >= chord.start && n.start < chord.start+chord.length)
                rhythm.emplace_back(n.start,n.duration);

                std::sort(rhythm.begin(),rhythm.end());

                rhythm.erase(std::unique(rhythm.begin(),rhythm.end()),rhythm.end());

                for (auto [start,duration] : rhythm) for (auto pitch : chord.notes)
                result.clip.notes.push_back({
                    start,duration,pitch.midi,result.config.velocity,0
                });

            }

            sort_notes(result.clip);

        }

    }

    Idea make_idea(Config config,const ProfileBank& bank,std::string_view genre,ArpOrder order,
    const Idea* previous,Locks locks,bool mutate) {

        auto r = generate(config,bank,genre);

        const auto& p = find_profile(bank,genre);

        const bool compatible = previous && previous->harmony.config.key == config.key &&
        previous->harmony.config.scale == config.scale && previous->harmony.config.bars == config.bars;

        if (previous && (locks.harmony || locks.voicing || locks.rhythm) && !compatible)
        throw std::invalid_argument("Unlock stages before changing key, scale or length");

        if (compatible && (mutate || locks.harmony || locks.voicing)) {

            auto specs = std::vector<ChordSpec>{

            };

            for (const auto& c : previous->harmony.chords) specs.push_back(c.spec);

            if (mutate && !locks.harmony && !locks.voicing) {

                Random choice(config.seed ^ 0x746172676574ULL);

                const auto bar = static_cast<std::size_t>(choice.bounded(specs.size()));

                specs[bar] = r.chords[bar].spec;

            }

            Random random(r.seeds.voicing);

            r.chords = voice(config,p,specs,random);

            if (locks.voicing) r.chords = previous->harmony.chords;

            apply_rhythm(r, (locks.rhythm || mutate) ? previous->harmony : r);

        } else if (compatible && locks.rhythm) {

            apply_rhythm(r,previous->harmony);

        }

        Idea idea;

        idea.harmony = std::move(r);
        idea.arp = order;

        idea.parts[0] = idea.harmony.clip;

        for (int i=1;i<4;++i) {

            idea.parts[static_cast<std::size_t>(i)] = {
                i==1?"PACKRAT Bass":i==2?"PACKRAT Arp":"PACKRAT Melody",
                idea.harmony.clip.length,config.bpm,{

                }, {

                }

            };

        }

        Random bass(config.seed ^ 0x62617373ULL), arp(config.seed ^ 0x617270ULL), melody(config.seed ^ 0x6d656c6f6479ULL);

        std::vector<const RhythmPattern*> patterns;

        for (const auto& rhythm : p.rhythms) if (rhythm.min_complexity <= config.complexity) patterns.push_back(&rhythm);

        const auto& motif = *patterns[static_cast<std::size_t>(bass.bounded(patterns.size()))];

        std::array<int,8> melody_steps{

        };

        for (auto& s : melody_steps) s = static_cast<int>(melody.bounded(5))-2;

        int previous_melody = 72;

        for (std::size_t b=0;b<idea.harmony.chords.size();++b) {

            const auto& chord = idea.harmony.chords[b];

            const auto classes = chord_classes(chord.spec);

            for (auto step : motif.steps) {

                const Tick start=chord.start+step*(ppq/4);

                const auto next=std::upper_bound(motif.steps.begin(),motif.steps.end(),step);

                const Tick gap=((next==motif.steps.end()?16:*next)-step)*(ppq/4);

                int pc=chord.spec.root;

                if (config.complexity>=60 && step%4!=0 && bass.bounded(4)==0) {

                    const auto& intervals=scale_info(config.scale).intervals;

                    pc=config.key.root+intervals[static_cast<std::size_t>(bass.bounded(intervals.size()))];

                }

                const int target=bass.bounded(5)==0?48:36;

                const int pitch=fit(pc,28,52,target);

                const Tick duration=std::max<Tick>(1,std::min<Tick>(gap-1,motif.gate_steps*(ppq/4)));

                idea.parts[1].notes.push_back({
                    start,duration,pitch,std::clamp(config.velocity+p.accents[static_cast<std::size_t>(step)],1,127),1
                });

            }

            const int count=config.complexity>=60?16:8;

            const Tick stride=bar_ticks/count;

            for (int s=0;s<count;++s) {

                const int n=static_cast<int>(chord.notes.size());

                int index=s%n;

                if (order==ArpOrder::Down) index=n-1-index;

                if (order==ArpOrder::UpDown) {
                    const int cycle=std::max(1,2*n-2);
                    index=s%cycle;
                    if(index>=n)index=cycle-index;

                }

                if (order==ArpOrder::Random) index=static_cast<int>(arp.bounded(static_cast<std::uint64_t>(n)));

                const int pitch=fit(chord.notes[static_cast<std::size_t>(index)].pc(),48,84,chord.notes[static_cast<std::size_t>(index)].midi);

                const Tick swing=(s%2==1)?stride*p.swing/200:0;

                const Tick start=chord.start+s*stride+swing;

                idea.parts[2].notes.push_back({
                    start,std::max<Tick>(1,std::min<Tick>(stride*3/4,chord.start+bar_ticks-start)),pitch,std::clamp(config.velocity+(s%4==0?8:-4),1,127),2
                });

            }

            for (int s=0;s<8;++s) {

                if (s%2==1 && config.complexity<40) continue;

                if (s==5 && melody.bounded(3)==0) continue;

                const bool strong=s%4==0;

                int target=std::clamp(previous_melody+melody_steps[static_cast<std::size_t>(s)]*2,60,88);

                int pitch=60, distance=1000;

                for (int candidate=60;candidate<=88;++candidate) {

                    const bool allowed=strong ? std::find(classes.begin(),classes.end(),pitch_class(candidate))!=classes.end() : in_scale(config.key,config.scale,candidate);

                    const int cost=std::abs(candidate-target)+std::max(0,std::abs(candidate-previous_melody)-7)*3;

                    if (allowed && cost<distance) {
                        pitch=candidate;
                        distance=cost;

                    }

                }

                previous_melody=pitch;

                idea.parts[3].notes.push_back({
                    chord.start+s*(ppq/2),ppq*3/8,pitch,std::clamp(config.velocity+(strong?6:-5),1,127),3
                });

            }

        }

        // Locked rhythm preserves the exact onset/duration pattern of each companion.
        if (compatible && locks.rhythm) for (std::size_t i=1;i<4;++i) {

            auto fresh=idea.parts[i].notes;

            idea.parts[i].notes=previous->parts[i].notes;

            for (std::size_t n=0;n<idea.parts[i].notes.size();++n) {

                auto& note=idea.parts[i].notes[n];

                const auto bar=static_cast<std::size_t>(note.start/bar_ticks);

                const auto& c=idea.harmony.chords[bar];

                if (i==1) note.pitch=fit(c.spec.root,28,52,note.pitch);

                else if (i==2) note.pitch=fit(c.notes[n%c.notes.size()].pc(),48,84,note.pitch);

                else if (!in_scale(config.key,config.scale,note.pitch)) note.pitch=fit(c.spec.root,60,88,note.pitch);

                note.velocity=fresh.empty()?config.velocity:fresh[n%fresh.size()].velocity;

            }

        }

        if (compatible && mutate && !locks.rhythm) {

            Random choice(config.seed ^ 0x746172676574ULL);

            const auto bar=static_cast<Tick>(choice.bounded(previous->harmony.chords.size()));

            for(std::size_t i=1;i<4;++i) {

                auto changed=idea.parts[i].notes;

                idea.parts[i].notes.clear();

                for(const auto& n:previous->parts[i].notes)if(n.start/bar_ticks!=bar)idea.parts[i].notes.push_back(n);

                for(const auto& n:changed)if(n.start/bar_ticks==bar)idea.parts[i].notes.push_back(n);

            }

        }

        for (auto& c : idea.parts) sort_notes(c);

        validate(idea);
        return idea;

    }

    Clip audition_clip(const Idea& idea,Part part) {

        if (part != Part::Arrangement) return idea.parts.at(static_cast<std::size_t>(part));

        auto clip=idea.parts[0];
        clip.name="PACKRAT Arrangement";

        for (std::size_t i=1;i<4;++i) clip.notes.insert(clip.notes.end(),idea.parts[i].notes.begin(),idea.parts[i].notes.end());

        sort_notes(clip);
        return clip;

    }

    void validate(const Idea& idea) {

        validate(idea.harmony.config);

        if (idea.harmony.chords.size()!=static_cast<std::size_t>(idea.harmony.config.bars)) throw std::invalid_argument("Chord count does not match bars");

        for (const auto& c:idea.parts) {

            validate(c);

            if (c.length!=idea.harmony.config.bars*bar_ticks || c.bpm!=idea.harmony.config.bpm || c.notes.size()>4096)
            throw std::invalid_argument("Part size, tempo or length is invalid");

        }

        if (idea.parts[0]!=idea.harmony.clip) throw std::invalid_argument("Chord clip mismatch");

        for (std::size_t b=0;b<idea.harmony.chords.size();++b) {

            const auto& c=idea.harmony.chords[b];

            if(c.start!=static_cast<Tick>(b)*bar_ticks || c.length!=bar_ticks || c.notes.empty() || c.notes.size()>5 || c.spec.degree<0 || c.spec.degree>6 || c.spec.root<0 || c.spec.root>11)
            throw std::invalid_argument("Invalid chord");

            const auto pcs=chord_classes(c.spec);

            for (auto n:c.notes) if(n.midi<idea.harmony.config.low || n.midi>idea.harmony.config.high || std::find(pcs.begin(),pcs.end(),n.pc())==pcs.end()) throw std::invalid_argument("Invalid voicing pitch");

        }

        if(static_cast<int>(idea.arp)<0 || static_cast<int>(idea.arp)>3)throw std::invalid_argument("Invalid arpeggio order");

    }

    std::vector<std::uint8_t> export_bytes(const Document& d) {

        return d.part==Part::Arrangement?arrangement_midi(d.idea.parts):midi_file(audition_clip(d.idea,d.part));

    }

    void export_pack(const std::filesystem::path& directory,const Document& source,const ProfileBank& bank,int count) {

        if(count!=8&&count!=16&&count!=32&&count!=64)throw std::invalid_argument("Pack count must be 8, 16, 32 or 64");

        if(!std::filesystem::create_directory(directory))throw std::runtime_error("Choose a new pack directory; existing exports are protected");

        Random seeds(source.idea.harmony.config.seed);

        for(int i=0;i<count;++i) {

            auto config=source.idea.harmony.config;
            config.seed=seeds.next();

            Document d=source;
            d.idea=make_idea(config,bank,source.idea.harmony.genre,source.idea.arp,&source.idea,source.locks);

            std::ostringstream name;
            name<<std::setw(3)<<std::setfill('0')<<i+1;

            auto path=directory/name.str();
            std::filesystem::create_directory(path);

            constexpr std::array<const char*,4> names={
                "chords.mid","bass.mid","arp.mid","melody.mid"
            };

            for(std::size_t part=0;part<4;++part)export_midi(d.idea.parts[part],path/names[part]);

            const auto text=serialize(d);
            write_new(path/"metadata.json",{
                reinterpret_cast<const std::uint8_t*>(text.data()),text.size()
            });

        }

    }

}
