// SPDX-License-Identifier: GPL-3.0-or-later
#include "packrat/preview.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <tuple>
namespace packrat::preview {

    static_assert(std::atomic<unsigned>::is_always_lock_free);

    Prepared prepare(const Clip& clip) {

        validate(clip);

        if(clip.notes.size()>max_events/2)throw std::invalid_argument("Preview supports at most 4096 notes");

        Prepared p;
        p.length=clip.length;
        p.bpm=clip.bpm;

        for(const auto& n:clip.notes) {

            p.events[p.count++]={
                n.start,n.pitch,n.velocity,n.channel,true
            };

            p.events[p.count++]={
                n.start+n.duration,n.pitch,0,n.channel,false
            };

        }

        std::sort(p.events.begin(),p.events.begin()+static_cast<std::ptrdiff_t>(p.count),[](const auto& a,const auto& b){
            return std::tie(a.tick,a.on,a.pitch,a.channel)<std::tie(b.tick,b.on,b.pitch,b.channel);

        });

        return p;

    }

    void Mailbox::publish(const Prepared& clip) noexcept {

        slots_[writer_]=clip;

        writer_=middle_.exchange(writer_|4,std::memory_order_acq_rel)&3;

    }

    const Prepared* Mailbox::consume() noexcept {

        if((middle_.load(std::memory_order_acquire)&4)==0)return nullptr;

        reader_=middle_.exchange(reader_,std::memory_order_acq_rel)&3;

        return &slots_[reader_];

    }

    void Synth::prepare(double sample_rate) noexcept {

        sample_rate_=std::isfinite(sample_rate)?std::clamp(sample_rate,8000.0,384000.0):48000.0;

        for(std::size_t i=0;i<sine_.size();++i)sine_[i]=static_cast<float>(std::sin(2*std::numbers::pi*static_cast<double>(i)/4096.0));

        reset();

    }

    float Synth::sin_lookup(double p) const noexcept {

        p-=std::floor(p);
        const double pos=p*4096;
        // A tiny negative phase can round to exactly 1 after wrapping.
        const auto i=static_cast<std::size_t>(pos)&4095;

        return sine_[i]+static_cast<float>(pos-std::floor(pos))*(sine_[i+1]-sine_[i]);

    }

    void Synth::note_on(int pitch,int velocity,int channel,int sound) noexcept {

        if(pitch<0||pitch>127||velocity<=0||velocity>127)return;

        Voice* choice=nullptr;

        for(auto& v:voices_)if(!v.active){
            choice=&v;
            break;

        }

        if(!choice)choice=&*std::min_element(voices_.begin(),voices_.end(),[](const auto& a,const auto& b){
            return a.envelope<b.envelope;

        });

        *choice=Voice{

        };
        choice->active=true;
        choice->pitch=pitch;
        choice->channel=channel;
        choice->sound=std::clamp(sound,0,9);

        choice->velocity=static_cast<float>(velocity)/127;
        choice->frequency=440*std::exp2((pitch-69)/12.0);

        for(std::size_t i=0;i<7;++i)choice->phase[i]=static_cast<double>(i)/7;

    }

    void Synth::note_off(int pitch,int channel) noexcept {
        for(auto& v:voices_)if(v.pitch==pitch&&v.channel==channel)v.released=true;

    }

    void Synth::release_all() noexcept {
        for(auto& v:voices_)v.released=true;

    }

    void Synth::reset() noexcept {
        for(auto& v:voices_)v=Voice{

        };

    }

    static float blep(double p,double dt) noexcept {

        if(p<dt){
            p/=dt;
            return static_cast<float>(p+p-p*p-1);

        }

        if(p>1-dt){
            p=(p-1)/dt;
            return static_cast<float>(p*p+p+p+1);

        }

        return 0;

    }

    static float saw(double phase,double dt) noexcept {
        return static_cast<float>(2*phase-1)-blep(phase,dt);

    }

    float Synth::sample() noexcept {

        float out=0;

        constexpr std::array<double,10> attacks={
            .004,.003,.10,.018,.009,.015,.003,.002,.004,.006
        };

        constexpr std::array<double,10> decays={
            1.2,.8,.8,.4,.3,.4,.18,.17,.16,.3
        };

        constexpr std::array<float,10> sustains={
            .0f,.08f,.75f,.65f,.75f,.65f,.12f,.02f,.3f,.8f
        };

        for(auto& v:voices_)if(v.active) {

            const auto index=static_cast<std::size_t>(v.sound);

            if(v.released) v.envelope*=static_cast<float>(std::exp(-1/(sample_rate_*(v.sound==2?.25:.035))));

            else if(v.age<attacks[index])v.envelope=static_cast<float>(v.age/attacks[index]);

            else v.envelope=sustains[index]+(1-sustains[index])*static_cast<float>(std::exp(-(v.age-attacks[index])/decays[index]));

            if((v.released || v.age>attacks[index])&&v.envelope<.00001f){
                v.active=false;
                continue;

            }

            const double pitch_env=v.sound==5?1+.16*std::exp(-v.age/.07):1;

            const double dt=std::clamp(v.frequency*pitch_env/sample_rate_,.000001,.4);

            float signal=0;

            if(v.sound==0)signal=sin_lookup(v.phase[0])+.32f*sin_lookup(v.phase[0]*2)*static_cast<float>(std::exp(-v.age*3))+.13f*sin_lookup(v.phase[0]*3)*static_cast<float>(std::exp(-v.age*6));

            else if(v.sound==1)signal=sin_lookup(v.phase[0]+.13*sin_lookup(v.phase[0]*2)*std::exp(-v.age*3));

            else if(v.sound==9)signal=sin_lookup(v.phase[0]);

            else if(v.sound==7)signal=sin_lookup(v.phase[0]+.25*sin_lookup(v.phase[0]*3)*std::exp(-v.age*10));

            else if(v.sound==3) {

                for(std::size_t i=0;i<7;++i)signal+=saw(v.phase[i],dt)/7;

            } else if(v.sound==4)signal=.45f*saw(v.phase[0],dt)+.45f*saw(v.phase[1],dt)+.25f*sin_lookup(v.phase[2]);

            else if(v.sound==5) {

                const double width=.5+.22*sin_lookup(v.age*.6);
                double shifted=v.phase[0]+1-width;
                shifted-=std::floor(shifted);

                signal=.55f*(saw(v.phase[0],dt)-saw(shifted,dt))+.4f*saw(v.phase[1],dt);

            } else signal=saw(v.phase[0],dt);

            if(v.sound==2)signal=.55f*saw(v.phase[0],dt)+.3f*sin_lookup(v.phase[0])+.2f*saw(v.phase[1],dt);

            const double cutoff=v.sound==8?350+6500*std::exp(-v.age*8):v.sound==4?1800:v.sound==2?2200:v.sound==6?5000:15000;

            const float coeff=static_cast<float>(1-std::exp(-2*std::numbers::pi*std::min(cutoff,sample_rate_*.4)/sample_rate_));

            v.filter+=coeff*(signal-v.filter);

            out+=v.filter*v.envelope*v.velocity*.16f;

            for(std::size_t i=0;i<7;++i) {

                const double detune=v.sound==3?(static_cast<double>(i)-3)*.0035:i==1?.004:0;

                v.phase[i]+=dt*((v.sound==4 && i==2)?.5:(1+detune));
                v.phase[i]-=std::floor(v.phase[i]);

            }

            v.age+=1/sample_rate_;

        }

        return std::tanh(out);

    }

    void Player::prepare(double sample_rate) noexcept {
        sample_rate_=std::isfinite(sample_rate)?std::clamp(sample_rate,8000.0,384000.0):48000.0;
        synth_.prepare(sample_rate_);
        tick_=0;
        event_=0;
        playing_=false;
        fade_=0;

    }

    void Player::set_clip(const Prepared* clip) noexcept {
        clip_=clip;
        tick_=0;
        event_=0;
        synth_.release_all();

    }

    float Player::sample(bool play,int sound,double bpm) noexcept {

        if(play!=playing_) {

            if(play){
                tick_=0;
                event_=0;
                synth_.release_all();

            } else synth_.release_all();

            playing_=play;

        }

        if(play&&clip_) {

            while(event_<clip_->count && static_cast<double>(clip_->events[event_].tick)<=tick_) {

                const auto& e=clip_->events[event_++];

                if(e.on)synth_.note_on(e.pitch,e.velocity,e.channel,sound);
                else synth_.note_off(e.pitch,e.channel);

            }

            const double tempo=std::isfinite(bpm)&&bpm>=20&&bpm<=400?bpm:clip_->bpm;

            tick_+=tempo*ppq/(60*sample_rate_);

            if(tick_>=static_cast<double>(clip_->length)) {
                tick_-=static_cast<double>(clip_->length);
                event_=0;
                synth_.release_all();

            }

        }

        fade_+=static_cast<float>(1/(sample_rate_*.005))*(1-fade_);

        return synth_.sample()*fade_;

    }

}
