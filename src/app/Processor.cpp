// SPDX-License-Identifier: GPL-3.0-or-later
#include "Processor.hpp"
#include "Editor.hpp"
namespace packrat::app {
    Processor::Processor():AudioProcessor(BusesProperties().withOutput("Stereo",juce::AudioChannelSet::stereo(),true)),bank_(bundled_profiles()) {
        addParameter(play=new juce::AudioParameterBool(juce::ParameterID{
            "preview",1
        },"Preview",false));

        addParameter(host_tempo=new juce::AudioParameterBool(juce::ParameterID{
            "hostTempo",1
        },"Follow host tempo",false));

        addParameter(output=new juce::AudioParameterFloat(juce::ParameterID{
            "output",1
        },"Preview output",-48.0f,0.0f,-9.0f));

        addParameter(tempo=new juce::AudioParameterFloat(juce::ParameterID{
            "tempo",1
        },"Preview tempo",juce::NormalisableRange<float>{
            20,400,1
        },174));

        juce::StringArray names;

        for(auto name:preview::sounds)names.add(name);

        addParameter(sound=new juce::AudioParameterChoice(juce::ParameterID{
            "sound",1
        },"Preview sound",names,1));

        document_.idea=make_idea({
        },bank_,"liquid-dnb");

        mailbox_.publish(preview::prepare(document_.idea.parts[0]));

        setLatencySamples(0);
    }

    Document Processor::snapshot() const {
        std::lock_guard lock(document_mutex_);

        auto d=document_;

        d.sound=sound->getIndex();

        d.follow_host=host_tempo->get();

        d.output_db=output->get();

        const int bpm=static_cast<int>(std::lround(tempo->get()));

        d.idea.harmony.config.bpm=bpm;

        d.idea.harmony.clip.bpm=bpm;

        for(auto& part:d.idea.parts)part.bpm=bpm;

        return d;
    }

    void Processor::install(Document d,bool restore_parameters) {
        validate(d.idea);

        const auto prepared=preview::prepare(audition_clip(d.idea,d.part));

        std::lock_guard lock(document_mutex_);

        document_=std::move(d);

        mailbox_.publish(prepared);

        static_cast<juce::AudioProcessorParameter*>(tempo)->setValue(tempo->convertTo0to1(static_cast<float>(document_.idea.harmony.config.bpm)));

        if(restore_parameters) {
            static_cast<juce::AudioProcessorParameter*>(sound)->setValue(sound->convertTo0to1(static_cast<float>(document_.sound)));

            static_cast<juce::AudioProcessorParameter*>(output)->setValue(output->convertTo0to1(static_cast<float>(document_.output_db)));

            static_cast<juce::AudioProcessorParameter*>(host_tempo)->setValue(document_.follow_host?1.0f:0.0f);

            static_cast<juce::AudioProcessorParameter*>(play)->setValue(0);
        }

        ++revision;
    }

    void Processor::set_playing(bool value) {
        play->beginChangeGesture();

        play->setValueNotifyingHost(value?1.0f:0.0f);

        play->endChangeGesture();
    }

    void Processor::prepareToPlay(double rate,int) {
        player_.prepare(rate);

        output_gain_=juce::Decibels::decibelsToGain(output->get());

        output_smoothing_=static_cast<float>(1-std::exp(-1/(rate*.005)));
    }

    bool Processor::isBusesLayoutSupported(const BusesLayout& l) const {
        return l.getMainInputChannelSet().isDisabled() && l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();
    }

    void Processor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midi) {
        juce::ScopedNoDenormals no_denormals;

        buffer.clear();

        if(const auto* clip=mailbox_.consume())player_.set_clip(clip);

        double bpm=tempo->get();

        if(host_tempo->get())if(auto* head=getPlayHead())if(const auto p=head->getPosition())if(const auto tempo=p->getBpm())bpm=*tempo;

        const bool playing=play->get();

        const int selected_sound=sound->getIndex();

        const float target=juce::Decibels::decibelsToGain(output->get());

        auto event=midi.cbegin();

        float peak=0;

        for(int i=0;i<buffer.getNumSamples();++i) {
            while(event!=midi.cend()&&(*event).samplePosition<=i) {
                const auto raw=*event;

                if(raw.numBytes>3){
                    ++event;

                    continue;
                }

                const auto msg=raw.getMessage();

                if(msg.isNoteOn())player_.note_on(msg.getNoteNumber(),msg.getVelocity(),msg.getChannel()-1,selected_sound);

                else if(msg.isNoteOff())player_.note_off(msg.getNoteNumber(),msg.getChannel()-1);

                else if(msg.isAllNotesOff()||msg.isAllSoundOff())player_.release_all();

                ++event;
            }

            output_gain_+=output_smoothing_*(target-output_gain_);

            const float value=player_.sample(playing,selected_sound,bpm)*output_gain_;

            for(int channel=0;channel<buffer.getNumChannels();++channel)buffer.setSample(channel,i,value);

            peak=std::max(peak,std::abs(value));
        }

        midi.clear();

        position.store(static_cast<float>(player_.position()),std::memory_order_relaxed);

        meter.store(peak,std::memory_order_relaxed);
    }

    void Processor::getStateInformation(juce::MemoryBlock& data) {
        const auto text=serialize(snapshot());

        data.replaceAll(text.data(),text.size());
    }

    void Processor::setStateInformation(const void* bytes,int size) {
        if(size<=0 || size>2*1024*1024 || !bytes){
            state_error=true;

            return;
        }

        try {
            install(deserialize({
                static_cast<const char*>(bytes),static_cast<std::size_t>(size)
            },bank_),true);

            state_error=false;
        } catch(const std::exception&){
            state_error=true;
        }
    }

    juce::AudioProcessorEditor* Processor::createEditor(){
        return new Editor(*this);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){
    return new packrat::app::Processor;
}
