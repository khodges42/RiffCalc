// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "ClassicLook.hpp"
#include <juce_audio_basics/juce_audio_basics.h>
#include "packrat/music.hpp"
namespace packrat::app {
    class NoteSheet final : public juce::Component,private juce::TableListBoxModel {
        Clip clip_;
        unsigned epoch_=0;

        juce::TableListBox table_{
            "MIDI Event Sheet",this
        };

        int getNumRows() override {
            return static_cast<int>(clip_.notes.size());
        }

        void paintRowBackground(juce::Graphics&,int,int,int,bool) override;

        void paintCell(juce::Graphics&,int,int,int,int,bool) override;

        juce::Component* refreshComponentForCell(int,int,bool,juce::Component*) override;

        void selectedRowsChanged(int row) override;

        public:
        std::function<void(std::size_t,int)> onVelocity;

        std::function<void(const juce::String&)> onInspect;

        NoteSheet();

        void set(Clip clip);

        void resized() override;
    };
}
