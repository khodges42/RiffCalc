// SPDX-License-Identifier: GPL-3.0-or-later
#include "packrat/generator.hpp"
#include <algorithm>
namespace packrat {
    Result generate(const Config& c,const ProfileBank& bank,std::string_view genre) {
        validate(c);
        const auto& p=find_profile(bank,genre);
        validate(p);
        const auto seeds=stage_seeds(c.seed);
        Random hr(seeds.harmony),vr(seeds.voicing),rr(seeds.rhythm),tr(seeds.timing),vel(seeds.velocity);
        auto specs=harmony(c,p,hr);
        auto chords=voice(c,p,specs,vr);
        Result r{
            c,seeds,p.id,p.version,bank.fingerprint,std::move(chords),{}
        };
        r.clip.name="PACKRAT / "+p.name;
        r.clip.length=bar_ticks*c.bars;
        r.clip.bpm=c.bpm;
        std::vector<std::uint32_t> weights;
        for(const auto& pat:p.rhythms)weights.push_back(pat.min_complexity<=c.complexity?pat.weight:0);
        struct Onset {
            Tick start,gate;
            std::size_t chord;
            int step;
        };
        std::vector<Onset> onsets;
        std::size_t motif=rr.weighted(weights);
        for(std::size_t bar=0;bar<r.chords.size();++bar) {
            if(bar>0&&bar%4==0&&rr.bounded(100)>=static_cast<std::uint64_t>(p.repetition))motif=rr.weighted(weights);
            const auto& pattern=p.rhythms[motif];
            const Tick base=static_cast<Tick>(bar)*bar_ticks;
            for(int step:pattern.steps) {
                Tick jitter=static_cast<Tick>(tr.bounded(static_cast<std::uint64_t>(c.humanize)*2+1))-c.humanize;
                const Tick swing=step%2==1?(ppq/4)*p.swing/100:0;
                Tick start=std::clamp(base+step*(ppq/4)+swing+jitter,base,base+bar_ticks-1);
                onsets.push_back({
                    start,static_cast<Tick>(pattern.gate_steps)*(ppq/4),bar,step
                });
            }
        }
        for(std::size_t i=0;i<onsets.size();++i) {
            const auto& o=onsets[i];
            Tick until=i+1<onsets.size()?onsets[i+1].start:r.clip.length;
            // A chord's notes never extend into the next harmonic bar, even if its
            // successor has a delayed first stab. Preserve predictable harmony.
            until=std::min(until,(static_cast<Tick>(o.chord)+1)*bar_ticks);
            const Tick duration=std::max<Tick>(1,std::min(o.gate,until-o.start));
            for(auto pitch:r.chords[o.chord].notes) {
                int variation=static_cast<int>(vel.bounded(static_cast<std::uint64_t>(p.velocity_spread)*2+1))-p.velocity_spread;
                int v=std::clamp(c.velocity+p.accents[static_cast<std::size_t>(o.step)]+variation,1,127);
                r.clip.notes.push_back({
                    o.start,duration,pitch.midi,v,0
                });
            }
        }
        validate(r.clip);
        return r;
    }
}
// namespace packrat
