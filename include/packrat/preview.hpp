// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "music.hpp"
#include <atomic>
#include <array>
namespace packrat::preview {

    inline constexpr std::size_t max_events=8192;

    struct Event {
        Tick tick=0;
        int pitch=0,velocity=0,channel=0;
        bool on=false;

    };

    struct Prepared {

        std::array<Event,max_events> events{

        };

        std::size_t count=0;

        Tick length=bar_ticks*4;

        int bpm=120;

    };

    Prepared prepare(const Clip& clip);

    // Producer calls are serialized outside audio. Consumer is the audio callback.
    class Mailbox {

        std::array<Prepared,3> slots_{

        };

        unsigned writer_=1,reader_=0;

        std::atomic<unsigned> middle_{
            2
        };

        public:
        void publish(const Prepared& clip) noexcept;

        const Prepared* consume() noexcept;

    };

    class Synth {

        struct Voice {

            bool active=false,released=false;

            int pitch=0,channel=0,sound=0;

            double frequency=0,age=0;

            std::array<double,7> phase{

            };

            float envelope=0,velocity=0,filter=0;

        };

        std::array<Voice,64> voices_{

        };

        std::array<float,4097> sine_{

        };

        double sample_rate_=48000;

        float sin_lookup(double phase) const noexcept;

        public:
        void prepare(double sample_rate) noexcept;

        void note_on(int pitch,int velocity,int channel,int sound) noexcept;

        void note_off(int pitch,int channel) noexcept;

        void release_all() noexcept;

        void reset() noexcept;

        float sample() noexcept;

    };

    class Player {

        Synth synth_;

        const Prepared* clip_=nullptr;

        double sample_rate_=48000,tick_=0;

        std::size_t event_=0;

        bool playing_=false;

        float fade_=0;

        public:
        void prepare(double sample_rate) noexcept;

        void set_clip(const Prepared* clip) noexcept;

        void note_on(int pitch,int velocity,int channel,int sound) noexcept {
            synth_.note_on(pitch,velocity,channel,sound);

        }

        void note_off(int pitch,int channel) noexcept {
            synth_.note_off(pitch,channel);

        }

        void release_all() noexcept {
            synth_.release_all();

        }

        float sample(bool play,int sound,double bpm) noexcept;

        double position() const noexcept {
            return clip_?tick_/static_cast<double>(clip_->length):0;

        }

    };

    inline constexpr std::array<const char*,10> sounds={
        "Piano","Electric piano","Warm pad","Supersaw","Reese bass","Hoover","Rave stab","Pluck","Acid bass","Sine sub"
    };

}
