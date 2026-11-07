// SPDX-License-Identifier: GPL-3.0-or-later
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <iostream>
#include <cmath>
void check(bool ok,const char* message){
    if(!ok)throw std::runtime_error(message);

}

int main(int argc,char** argv){
    try{

        juce::ScopedJuceInitialiser_GUI init;

        check(argc>=2,"Expected VST3 bundle path");
        juce::VST3PluginFormat format;

        juce::OwnedArray<juce::PluginDescription> found;
        format.findAllTypesForFile(found,juce::File(argv[1]).getFullPathName());

        check(found.size()==1,"Scan standalone PACKRAT VST3");
        std::cout<<"Plugin identity: "<<std::hex<<static_cast<unsigned>(found[0]->uniqueId)<<" / "<<static_cast<unsigned>(found[0]->deprecatedUid)<<std::dec<<"\n";
        if(argc>2&&std::string(argv[2])=="--identity-only")return 0;
#ifdef _WIN32
        check(static_cast<unsigned>(found[0]->uniqueId)==0x5bd65fc7u&&static_cast<unsigned>(found[0]->deprecatedUid)==0x38bf9881u,"Legacy VST3 identity unchanged");
#endif
        juce::String error;

        auto plugin=format.createInstanceFromDescription(*found[0],48000,256,error);

        if(!plugin)throw std::runtime_error(error.toStdString());

        check(plugin->getTotalNumOutputChannels()==2 && plugin->getTotalNumInputChannels()==0,"Stereo instrument buses");

        plugin->prepareToPlay(48000,256);

        juce::AudioBuffer<float> audio(2,256);
        juce::MidiBuffer midi;

        midi.addEvent(juce::MidiMessage::noteOn(1,60,static_cast<juce::uint8>(100)),31);

        plugin->processBlock(audio,midi);

        for(int s=0;s<31;++s)check(audio.getSample(0,s)==0,"Mid-block MIDI timing");

        double energy=0;
        for(int i=0;i<100;++i){
            midi.clear();
            plugin->processBlock(audio,midi);
            for(int s=0;s<256;++s){
                const float x=audio.getSample(0,s);
                check(std::isfinite(x)&&std::abs(x)<=1,"Bounded VST3 audio");
                energy+=x*x;

            }

        }

        check(energy>.01,"Actual loaded VST3 produces MIDI audio");

        juce::MemoryBlock state;
        plugin->getStateInformation(state);
        check(state.getSize()>100,"VST3 state written");

        plugin->setStateInformation(state.getData(),static_cast<int>(state.getSize()));

        for(auto* parameter:plugin->getParameters())if(auto* id=dynamic_cast<juce::AudioProcessorParameterWithID*>(parameter))if(id->paramID=="preview")parameter->setValue(1);

        // Hosts may expose hashed VST3 IDs, so use parameter display name as fallback.
        for(auto* parameter:plugin->getParameters())if(parameter->getName(128)=="Preview")parameter->setValueNotifyingHost(1);

        energy=0;
        for(int i=0;i<200;++i){
            midi.clear();
            plugin->processBlock(audio,midi);
            energy+=audio.getRMSLevel(0,0,256);

        }

        check(energy>.01,"Loaded VST3 generated clip preview");

        if(plugin->hasEditor()){
            std::unique_ptr<juce::AudioProcessorEditor> editor(plugin->createEditor());
            check(editor&&editor->getWidth()>=880,"Native VST3 editor construction");

        }

        plugin->releaseResources();
        plugin.reset();
        std::cout<<"PASS: independent JUCE VST3 scan/load, stereo audio, sample-offset MIDI, state and editor\n";
        return 0;

    } catch(const std::exception& e){
        std::cerr<<e.what()<<'\n';
        return 1;

    }

}
