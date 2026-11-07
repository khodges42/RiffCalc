// SPDX-License-Identifier: GPL-3.0-or-later
#include "Processor.hpp"
#include "Editor.hpp"
#include <juce_audio_formats/juce_audio_formats.h>
#include <iostream>
#include <cmath>
#include <chrono>
#include <atomic>
#include <new>
#include <cstdlib>
#include <thread>
#include <fstream>
thread_local bool count_audio=false;
std::atomic<int> audio_new{
    0
};

void* operator new(std::size_t size){
    if(count_audio)++audio_new;
    if(auto* p=std::malloc(size?size:1))return p;
    throw std::bad_alloc();

}

void operator delete(void* p)noexcept{
    std::free(p);

}
void operator delete(void* p,std::size_t)noexcept{
    std::free(p);

}

void* operator new[](std::size_t size){
    return ::operator new(size);

}
void operator delete[](void* p)noexcept{
    ::operator delete(p);

}
void operator delete[](void* p,std::size_t)noexcept{
    ::operator delete(p);

}

void check(bool ok,const char* message){
    if(!ok)throw std::runtime_error(message);

}

int main(int argc,char** argv){
    try{

        juce::ScopedJuceInitialiser_GUI juce;

        auto p=std::make_unique<packrat::app::Processor>();

        auto document=p->snapshot();
        document.part=packrat::Part::Arrangement;
        document.sound=3;
        document.locks={
            true,true,true
        };
        document.output_db=-12;
        p->install(document,true);

        juce::MemoryBlock state;
        p->getStateInformation(state);
        auto restored=std::make_unique<packrat::app::Processor>();
        restored->setStateInformation(state.getData(),static_cast<int>(state.getSize()));

        check(restored->snapshot()==document,"Plugin exact state restoration");
        check(!restored->play->get(),"State restores stopped");

        const std::string bad="not a session";
        restored->setStateInformation(bad.data(),static_cast<int>(bad.size()));
        check(restored->snapshot()==document,"Bad state retains document");

        for(double rate:{
            44100.0,48000.0,96000.0
        }) {

            std::vector<float> reference;

            for(int block:{
                1,17,64,511,1024
            }) {

                p->set_playing(false);
                p->prepareToPlay(rate,block);
                p->set_playing(true);

                juce::AudioBuffer<float> audio(2,block);
                juce::MidiBuffer midi;
                midi.ensureSize(1024);
                std::vector<float> render;
                render.reserve(16384);

                for(int i=0;i<16384;i+=block) {

                    const int n=std::min(block,16384-i);
                    audio.setSize(2,n,false,false,true);

                    count_audio=true;
                    p->processBlock(audio,midi);
                    count_audio=false;

                    for(int s=0;s<n;++s){
                        check(std::isfinite(audio.getSample(0,s)),"Finite plugin output");
                        check(audio.getSample(0,s)==audio.getSample(1,s),"Stereo consistency");
                        render.push_back(audio.getSample(0,s));

                    }

                }

                if(reference.empty())reference=render;
                else check(reference==render,"Plugin buffer-size independence");

                double energy=0;
                for(auto x:render)energy+=x*x;
                check(energy>.01,"Audible plugin output");

                p->set_playing(false);
                for(int i=0;i<static_cast<int>(rate*4);i+=block)p->processBlock(audio,midi);

                check(audio.getMagnitude(0,audio.getNumSamples())<.0001,"Clean preview stop");

            }

        }

        p->prepareToPlay(48000,256);
        p->set_playing(true);

        juce::AudioBuffer<float> concurrent(2,256);
        juce::MidiBuffer concurrent_midi;
        concurrent_midi.ensureSize(2048);

        const std::array<unsigned char,8> sysex={
            0xf0,1,2,3,4,5,6,0xf7
        };
        concurrent_midi.addEvent(sysex.data(),8,23);

        std::atomic<bool> done{
            false
        };
        std::thread producer([&]{
            for(int i=0;i<64;++i){
                auto next=p->snapshot();
                auto config=next.idea.harmony.config;
                config.seed+=static_cast<std::uint64_t>(i+1);
                next.idea=packrat::make_idea(config,p->profiles(),"liquid-dnb");
                p->install(std::move(next));
                juce::MemoryBlock saved;
                p->getStateInformation(saved);

            }
            done=true;

        });

        count_audio=true;
        do {
            p->processBlock(concurrent,concurrent_midi);
            check(std::isfinite(concurrent.getMagnitude(0,256)),"Concurrent publication finite audio");

        }
        while(!done);
        count_audio=false;
        producer.join();

        p->set_playing(false);

        check(audio_new==0,"No C++ audio callback allocations");

        std::ifstream legacy_stream(std::filesystem::path(RIFFCALC_SOURCE_DIR)/"tests/fixtures/legacy-0.2.packrat.json");
        std::string legacy((std::istreambuf_iterator<char>(legacy_stream)),{});
        const auto legacy_document=packrat::deserialize(legacy,p->profiles());
        p->setStateInformation(legacy.data(),static_cast<int>(legacy.size()));
        check(p->snapshot()==legacy_document,"Legacy PACKRAT 0.2 host state remains exact");

        // Exercise real editor callbacks, history, locks and both layouts on message thread.
        std::unique_ptr<juce::AudioProcessorEditor> editor(p->createEditor());

        auto click=[&](const juce::String& label){
            for(int i=0;i<editor->getNumChildComponents();++i)if(auto* b=dynamic_cast<juce::Button*>(editor->getChildComponent(i)))if(b->getButtonText()==label){
                b->onClick();
                return;

            }
            throw std::runtime_error("Missing functional button");

        };

        click("RIFF MACHINE +");
        editor->setSize(960,850);
        check(editor->getHeight()==850,"Expanded UI");
        editor->setSize(960,790);

        click("RIFF MACHINE -");
        editor->setSize(1280,900);

        auto* classic_editor=dynamic_cast<packrat::app::Editor*>(editor.get());
        check(classic_editor&&classic_editor->getMenuBarNames().size()==6,"Classic functional menu system");
        auto* selector=dynamic_cast<juce::ComboBox*>(editor->findChildWithID("pattern-selector"));
        auto* count_label=dynamic_cast<juce::Label*>(editor->findChildWithID("event-count"));
        check(selector&&count_label,"Inspectable pattern and status controls");
        const int original_part=selector->getSelectedId();selector->setSelectedId(2,juce::sendNotificationSync);
        check(count_label->getText()==juce::String(p->snapshot().idea.parts[1].notes.size())+" events","Status event count follows selected part");
        selector->setSelectedId(original_part,juce::sendNotificationSync);
        classic_editor->perform(packrat::app::Editor::Events);
        auto before_edit=p->snapshot();classic_editor->edit_velocity(0,101);
        check(packrat::audition_clip(p->snapshot().idea,p->snapshot().part).notes[0].velocity==101,"Event Sheet velocity editing");
        classic_editor->perform(packrat::app::Editor::Previous);check(p->snapshot()==before_edit,"Event edit history restore");
        classic_editor->perform(packrat::app::Editor::Next);
        auto custom_channels=p->snapshot();custom_channels.part=packrat::Part::Arrangement;
        for(auto& note:custom_channels.idea.parts[1].notes)note.channel=9;
        p->install(custom_channels);classic_editor->edit_velocity(0,102);
        check(packrat::audition_clip(p->snapshot().idea,packrat::Part::Arrangement).notes[0].velocity==102,"Arrangement velocity edit respects imported channel assignments");
        classic_editor->perform(packrat::app::Editor::Piano);
        classic_editor->show_about();
        bool found_about=false;for(int i=0;i<juce::Component::getNumCurrentlyModalComponents();++i)if(auto* modal=juce::Component::getCurrentlyModalComponent(i)){if(auto* window=dynamic_cast<juce::DialogWindow*>(modal)){found_about=window->getName().contains("About RiffCalc")&&window->isOnDesktop()&&window->getPeer()!=nullptr;window->exitModalState(0);window->setVisible(false);}}
        check(found_about,"Real classic About dialog opens");
        check(classic_editor->keyPressed(juce::KeyPress(juce::KeyPress::F5Key)),"Seed regeneration shortcut");
        click("GENERATE");
        click("MUTATE");
        click("<");
        click(">");
        click("PLAY");
        click("STOP");

        p->prepareToPlay(48000,256);
        p->set_playing(true);
        juce::AudioBuffer<float> audio(2,256);
        juce::MidiBuffer midi;

        if(argc>1){
            auto directory=juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
            if(juce::File::isAbsolutePath(argv[1]))directory=juce::File(argv[1]);
            directory.createDirectory();

            juce::WavAudioFormat wav;
            auto file=directory.getChildFile("packrat-preview.wav");
            file.deleteFile();

            std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(file.createOutputStream().release(),48000,2,16,{

            },0));
            check(writer!=nullptr,"WAV writer");

            const auto begin=std::chrono::steady_clock::now();

            for(int n=0;n<48000*16;n+=256){
                p->processBlock(audio,midi);
                writer->writeFromAudioSampleBuffer(audio,0,audio.getNumSamples());

            }

            std::cout<<"16-second arrangement render: "<<std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count()<<" seconds\n";

        }

        editor.reset();
        std::cout<<"PASS: actual processor state, 15 block/rate cases, zero callback new, editor actions, WAV rendering\n";
        return 0;

    } catch(const std::exception& e){
        count_audio=false;
        std::cerr<<e.what()<<'\n';
        return 1;

    }

}
