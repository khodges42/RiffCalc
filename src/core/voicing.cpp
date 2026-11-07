// SPDX-License-Identifier: GPL-3.0-or-later
#include "packrat/generator.hpp"
#include <algorithm>
#include <cstdlib>
#include <limits>
#include <numeric>
#include <stdexcept>
namespace packrat {
    int movement_cost(std::span<const Pitch> a,std::span<const Pitch> b) {
        if(a.size()>5||b.size()>5)throw std::invalid_argument("Voice leading supports up to five voices");
        int dp[6][6]{};
        for(std::size_t i=0;i<=a.size();++i)dp[i][0]=static_cast<int>(i)*9;
        for(std::size_t j=0;j<=b.size();++j)dp[0][j]=static_cast<int>(j)*9;
        for(std::size_t i=1;i<=a.size();++i)for(std::size_t j=1;j<=b.size();++j) {
            int d=std::abs(a[i-1].midi-b[j-1].midi);
            int leap=d+std::max(0,d-7)*2;
            dp[i][j]=std::min({
                dp[i-1][j-1]+leap,dp[i-1][j]+9,dp[i][j-1]+9
            });
        }
        int result=dp[a.size()][b.size()];
        if(!a.empty()&&!b.empty())result+=std::abs(a.front().midi-b.front().midi)*2+std::abs(a.back().midi-b.back().midi);
        return result;
    }
    struct Candidate {
        std::vector<Pitch> notes;
        int cost=0;
        int previous=-1;
        int total=0;
    };
    static std::vector<Candidate> candidates(const Config& c,const GenreProfile& p,const ChordSpec& spec,Random& rng) {
        auto pcs=chord_classes(spec);
        std::sort(pcs.begin(),pcs.end());
        std::vector<Candidate> out;
        const int center=std::clamp(p.center,c.low,c.high);
        for(int bass=c.low;bass<=c.high;++bass) {
            auto index=std::find(pcs.begin(),pcs.end(),pitch_class(bass));
            if(index==pcs.end())continue;
            std::vector<Pitch> notes{
                {
                    bass
                }
            };
            int note=bass;
            for(std::size_t i=1;i<pcs.size();++i){
                do{
                    ++note;
                }
                while(std::find(pcs.begin(),pcs.end(),pitch_class(note))==pcs.end());
                notes.push_back({
                    note
                });
            }
            for(int variant=0;variant<(p.spread>0?2:1);++variant) {
                auto v=notes;
                if(variant==1)v[v.size()-2].midi-=12;
                std::sort(v.begin(),v.end(),[](auto a,auto b){
                    return a.midi<b.midi;
                });
                if(v.front().midi<c.low||v.back().midi>c.high)continue;
                if(std::any_of(out.begin(),out.end(),[&v](const auto& x){
                    return x.notes==v;
                }))continue;
                int sum=0;
                for(auto n:v)sum+=n.midi;
                int cost=std::abs(sum/static_cast<int>(v.size())-center)*2;
                cost+=std::abs(v.back().midi-v.front().midi-(10+p.spread/10))/2;
                cost+=pitch_class(v.front().midi)==spec.root?0:2;
                cost+=static_cast<int>(rng.bounded(3));
                out.push_back({
                    std::move(v),cost,-1,0
                });
            }
        }
        if(out.empty())throw std::invalid_argument("Chord cannot fit the selected pitch range");
        return out;
    }
    std::vector<Chord> voice(const Config& c,const GenreProfile& p,std::span<const ChordSpec> specs,Random& rng) {
        validate(c);
        validate(p);
        if(specs.empty()||specs.size()>16)throw std::invalid_argument("Voicing requires 1..16 chords");
        std::vector<std::vector<Candidate>> all;
        for(const auto& spec:specs)all.push_back(candidates(c,p,spec,rng));
        for(std::size_t bar=0;bar<all.size();++bar)for(auto& now:all[bar]) {
            if(bar==0){
                now.total=now.cost;
                continue;
            }
            now.total=std::numeric_limits<int>::max();
            for(std::size_t j=0;j<all[bar-1].size();++j) {
                const auto& prev=all[bar-1][j];
                int cost=prev.total+movement_cost(prev.notes,now.notes)+now.cost;
                if(cost<now.total){
                    now.total=cost;
                    now.previous=static_cast<int>(j);
                }
            }
        }
        int best=0;
        for(std::size_t i=1;i<all.back().size();++i)if(all.back()[i].total<all.back()[static_cast<std::size_t>(best)].total)best=static_cast<int>(i);
        std::vector<Chord> result(specs.size());
        for(std::size_t back=all.size();back>0;--back) {
            const auto bar=back-1;
            const auto& chosen=all[bar][static_cast<std::size_t>(best)];
            auto classes=chord_classes(specs[bar]);
            const int bass=pitch_class(chosen.notes.front().midi);
            int inversion=static_cast<int>(std::find(classes.begin(),classes.end(),bass)-classes.begin());
            auto names=spell_chord(c.key,c.scale,specs[bar]);
            auto symbol=names[0]+std::string(formula(specs[bar].quality).suffix);
            if(inversion>0)symbol+="/"+names[static_cast<std::size_t>(inversion)];
            result[bar]={
                specs[bar],static_cast<Tick>(bar)*bar_ticks,bar_ticks,chosen.notes,inversion,symbol
            };
            best=chosen.previous;
        }
        return result;
    }
}
// namespace packrat
