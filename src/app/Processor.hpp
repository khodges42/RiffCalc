// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "packrat/workstation.hpp"
#include "packrat/preview.hpp"
#include <mutex>
namespace packrat::app {
    class Processor final : public juce::AudioProcessor {
        ProfileBank bank_;

        mutable std::mutex document_mutex_;

        Document document_;

        preview::Mailbox mailbox_;

        preview::Player player_;

        float output_gain_=0,output_smoothing_=0;

        public:
        juce::AudioParameterBool *play=nullptr,*host_tempo=nullptr;

        juce::AudioParameterFloat *output=nullptr,*tempo=nullptr;

        juce::AudioParameterChoice* sound=nullptr;

        std::atomic<float> position{
            0
        },meter{
            0
        };

        std::atomic<unsigned> revision{
            0
        };

        Processor();

        const ProfileBank& profiles() const noexcept {
            return bank_;
        }

        Document snapshot() const;

        void install(Document document,bool restore_parameters=false);

        void set_playing(bool value);

        void prepareToPlay(double rate,int block) override;

        void releaseResources() override {
        }

        void processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midi) override;

        bool isBusesLayoutSupported(const BusesLayout& layout) const override;

        juce::AudioProcessorEditor* createEditor() override;

        bool hasEditor() const override {
            return true;
        }

        const juce::String getName() const override {
            return "RiffCalc 2000 Pro";
        }

        bool acceptsMidi() const override {
            return true;
        }

        bool producesMidi() const override {
            return false;
        }

        double getTailLengthSeconds() const override {
            return 2;
        }

        int getNumPrograms() override {
            return 1;
        }

        int getCurrentProgram() override {
            return 0;
        }

        void setCurrentProgram(int) override {
        }

        const juce::String getProgramName(int) override {
            return "Current idea";
        }

        void changeProgramName(int,const juce::String&) override {
        }

        void getStateInformation(juce::MemoryBlock& data) override;

        void setStateInformation(const void* data,int size) override;

        std::atomic<bool> state_error{
            false
        };
    };
}
