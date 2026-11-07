// SPDX-License-Identifier: GPL-3.0-or-later
#include "packrat/generator.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace packrat {
    std::uint64_t Random::next() noexcept {
        std::uint64_t z=(state_+=UINT64_C(0x9e3779b97f4a7c15));
        z=(z^(z>>30))*UINT64_C(0xbf58476d1ce4e5b9);
        z=(z^(z>>27))*UINT64_C(0x94d049bb133111eb);
        return z^(z>>31);
    }
    std::uint64_t Random::bounded(std::uint64_t bound) {
        if(bound==0)throw std::invalid_argument("Empty random range");
        const auto threshold=(std::uint64_t(0)-bound)%bound;
        for(;;){
            auto x=next();
            if(x>=threshold)return x%bound;
        }
    }
    std::size_t Random::weighted(std::span<const std::uint32_t> weights) {
        std::uint64_t sum=0;
        for(auto w:weights)sum+=w;
        if(!sum)throw std::invalid_argument("Empty weighted distribution");
        auto n=bounded(sum);
        for(std::size_t i=0;i<weights.size();++i){
            if(n<weights[i])return i;
            n-=weights[i];
        }
        throw std::logic_error("Sampling invariant");
    }
    Seeds stage_seeds(std::uint64_t master) noexcept {
        // Stable domain identifiers; adding a stream cannot shift an existing one.
        auto hash=[master](std::uint64_t domain){
            Random r(master^domain);
            return r.next();
        };
        return {
            hash(0x4841524d4f4e59),hash(0x564f4943494e47),hash(0x52485954484d),hash(0x54494d494e47),hash(0x56454c4f43495459)
        };
    }
    void validate(const Config& c) {
        (void)scale_info(c.scale);
        if(c.key.root<0||c.key.root>11||c.key.accidental< -1||c.key.accidental>1)throw std::invalid_argument("Invalid key");
        const auto key=parse_key(c.key.name());
        if(key!=c.key)throw std::invalid_argument("Inconsistent key spelling/pitch class");
        if(c.bars<1||c.bars>16)throw std::invalid_argument("Bars must be 1..16");
        if(c.complexity<0||c.complexity>100)throw std::invalid_argument("Complexity must be 0..100");
        if(c.low<0||c.low>127||c.high<0||c.high>127||c.high-c.low<12)throw std::invalid_argument("Pitch range must be within 0..127 and span at least an octave");
        if(c.bpm<20||c.bpm>400||c.velocity<1||c.velocity>127||c.humanize<0||c.humanize>60)throw std::invalid_argument("Tempo, velocity or humanization outside supported range");
    }
    static std::vector<Quality> legal(const Config& c,int degree) {
        std::vector<Quality> q;
        for(const auto& f:formulas()) {
            if(f.complexity>c.complexity)continue;
            if(f.quality==Quality::Dominant7Flat9&&!c.colour)continue;
            bool okay=true;
            for(int i:f.intervals)okay=okay&&in_scale(c.key,c.scale,degree_pc(c.key,c.scale,degree)+i);
            if(okay)q.push_back(f.quality);
        }
        return q;
    }
    std::vector<ChordSpec> harmony(const Config& c,const GenreProfile& p,Random& rng) {
        validate(c);
        validate(p);
        std::array<std::vector<Quality>,7> choices;
        std::array<bool,7> available{};
        for(int d=0;d<7;++d){
            choices[static_cast<std::size_t>(d)]=legal(c,d);
            available[static_cast<std::size_t>(d)]=!choices[static_cast<std::size_t>(d)].empty();
        }
        if(!available[0])throw std::invalid_argument("No legal tonic chord in this scale");
        std::vector<std::uint32_t> weights;
        for(const auto& pattern:p.patterns){
            bool okay=true;
            for(int d:pattern.degrees)okay=okay&&available[static_cast<std::size_t>(d-1)];
            weights.push_back(okay?pattern.weight:0);
        }
        const bool has_pattern=std::any_of(weights.begin(),weights.end(),[](auto w){
            return w>0;
        });
        const auto* pattern=has_pattern?&p.patterns[rng.weighted(weights)]:nullptr;
        std::vector<ChordSpec> result;
        int previous=0;
        for(int bar=0;bar<c.bars;++bar) {
            int degree=pattern?pattern->degrees[static_cast<std::size_t>(bar)%pattern->degrees.size()]-1:0;
            if(!pattern||(bar%4==2&&rng.bounded(100)<static_cast<std::uint64_t>((100-p.repetition)*c.complexity/100))) {
                auto w=p.transitions[static_cast<std::size_t>(previous)];
                for(std::size_t i=0;i<7;++i)if(!available[i])w[i]=0;
                degree=static_cast<int>(rng.weighted(w));
            }
            if(bar==c.bars-1) {
                if(c.resolve)degree=0;
                else if(rng.bounded(100)>=static_cast<std::uint64_t>(p.modal)) {
                    auto w=p.cadence;
                    for(std::size_t i=0;i<7;++i)if(!available[i])w[i]=0;
                    if(std::any_of(w.begin(),w.end(),[](auto n){
                        return n>0;
                    }))degree=static_cast<int>(rng.weighted(w));
                }
            }
            const auto& candidates=choices[static_cast<std::size_t>(degree)];
            weights.clear();
            for(auto q:candidates){
                std::uint32_t w=1;
                for(const auto& pref:p.qualities)if(pref.quality==q)w+=pref.weight;
                weights.push_back(w);
            }
            Quality q=candidates[rng.weighted(weights)];
            bool chromatic=false;
            // Explicit, limited colour: V7 leading-tone cadence. Never arbitrary off-scale notes.
            if(c.colour&&c.complexity>=30&&degree==4&&degree_pc(c.key,c.scale,4)==pitch_class(c.key.root+7)&&rng.bounded(100)<static_cast<std::uint64_t>(p.colour)) {
                q=Quality::Dominant7;
                for(int i:formula(q).intervals)chromatic=chromatic||!in_scale(c.key,c.scale,degree_pc(c.key,c.scale,degree)+i);
            }
            result.push_back({
                degree_pc(c.key,c.scale,degree),degree,q,chromatic
            });
            previous=degree;
        }
        return result;
    }
}
// namespace packrat
