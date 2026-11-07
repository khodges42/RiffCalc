// SPDX-License-Identifier: GPL-3.0-or-later
#include "Editor.hpp"
#include "BuildInfo.hpp"
#include <BinaryData.h>
#include <charconv>
namespace packrat::app {
    namespace {
        const auto beige=classic::face,panel=classic::face,ink=classic::ink,green=classic::green,paper=classic::paper;

        using classic::bevel;

        std::filesystem::path path(const juce::File& f) {
#ifdef _WIN32
            return std::filesystem::path(f.getFullPathName().toWideCharPointer());

#else
            return std::filesystem::path(f.getFullPathName().toStdString());

#endif
        }

        void write_text(const juce::File& file,const std::string& text){
            write_new(path(file),{
                reinterpret_cast<const std::uint8_t*>(text.data()),text.size()
            });
        }

        juce::File settings(){
            return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Voidmutt Enterprises").getChildFile("RiffCalc");
        }
    }

    void PianoRoll::set(const Idea& idea,Part part){
        idea_=idea;

        clip_=audition_clip(idea,part);

        repaint();
    }

    void PianoRoll::paint(juce::Graphics& g) {
        g.fillAll(paper);

        const auto bounds=getLocalBounds();

        bevel(g,bounds,true);

        if(clip_.length<=0)return;

        const int keyboard=46,top=47,bottom=getHeight()-20;

        const float width=static_cast<float>(getWidth()-keyboard-4),height=static_cast<float>(bottom-top);

        int lowest=127,highest=0;

        for(const auto& n:clip_.notes){
            lowest=std::min(lowest,n.pitch);

            highest=std::max(highest,n.pitch);
        }

        lowest=std::max(0,lowest-3);

        highest=std::min(127,std::max(lowest+23,highest+3));

        const float row=height/static_cast<float>(highest-lowest+1);

        for(int n=lowest;n<=highest;++n) {
            const float y=top+(highest-n)*row;

            const int pc=pitch_class(n);

            const bool black=pc==1||pc==3||pc==6||pc==8||pc==10;

            g.setColour(black?juce::Colour(0xffdeddd2):paper);

            g.fillRect(static_cast<float>(keyboard),y,width,row);

            g.setColour(black?juce::Colour(0xff787d76):juce::Colour(0xfff9f6e9));

            g.fillRect(2.0f,y,38.0f,row);

            if(pc==0 && row>=5){
                g.setColour(ink);

                g.setFont(10.0f);

                g.drawText("C"+juce::String(n/12-1),4,static_cast<int>(y),34,std::max(10,static_cast<int>(row)),juce::Justification::centredLeft);
            }

            g.setColour(juce::Colour(0xffcecec3));

            g.drawHorizontalLine(static_cast<int>(y+row),static_cast<float>(keyboard),keyboard+width);
        }

        const int bars=static_cast<int>(clip_.length/bar_ticks);

        for(int beat=0;beat<=bars*4;++beat) {
            const float x=keyboard+width*static_cast<float>(beat)/static_cast<float>(bars*4);

            g.setColour(beat%4==0?juce::Colour(0xff929a90):juce::Colour(0xffc4c7bc));

            g.drawVerticalLine(static_cast<int>(x),static_cast<float>(top),static_cast<float>(bottom));
        }

        for(const auto& n:clip_.notes) {
            const float x=keyboard+width*static_cast<float>(n.start)/static_cast<float>(clip_.length);

            const float y=top+(highest-n.pitch)*row;

            const float length=std::max(2.0f,width*static_cast<float>(n.duration)/static_cast<float>(clip_.length));

            constexpr std::array<juce::uint32,4> colors={
                0xff526e5a,0xff76634b,0xff526b7d,0xff8a6875
            };

            g.setColour(juce::Colour(colors[static_cast<std::size_t>(n.channel)%4]));

            g.fillRect(x,y+.5f,length,std::max(1.5f,row-1));

            g.setColour(juce::Colour(0x70ffffff));

            g.drawHorizontalLine(static_cast<int>(y+1),x,x+length);
        }

        for(std::size_t i=0;i<idea_.harmony.chords.size();++i) {
            const int x=keyboard+static_cast<int>(width*static_cast<float>(i)/bars),w=static_cast<int>(width/bars);

            g.setColour(panel);

            g.fillRect(x,2,w-1,42);

            bevel(g,{
                x,2,w-1,42
            });

            g.setColour(ink);

            g.setFont(juce::Font(13.0f,juce::Font::bold));

            g.drawFittedText(juce::String::charToString(static_cast<juce::juce_wchar>('A'+i))+"    "+juce::String(i+1)+"   "+idea_.harmony.chords[i].symbol,x+4,5,w-8,19,juce::Justification::centredLeft,1);

            g.setFont(10.0f);

            g.drawText(juce::String(i+1)+"  "+roman(idea_.harmony.chords[i].spec),x+4,26,w-8,14,juce::Justification::centredLeft);
        }

        g.setColour(juce::Colour(0xffad5646));

        g.drawVerticalLine(keyboard+static_cast<int>(position_*width),static_cast<float>(top),static_cast<float>(bottom));

        g.setColour(ink);

        g.setFont(11.0f);

        g.drawText("960 PPQ   /   4/4     "+juce::String(static_cast<int>(clip_.notes.size()))+" notes",keyboard,bottom+3,getWidth()-keyboard-8,16,juce::Justification::centredLeft);
    }

    void Editor::label(juce::String text,juce::Component& c) {
        auto l=std::make_unique<juce::Label>();

        l->setText(text,juce::dontSendNotification);

        l->setFont(11.0f);

        l->attachToComponent(&c,false);

        addAndMakeVisible(*l);

        labels_.push_back(std::move(l));
    }

    Editor::Editor(Processor& p):AudioProcessorEditor(p),processor_(p) {
        juce::String arguments;

        if(p.wrapperType==juce::AudioProcessor::wrapperType_Standalone)arguments=juce::JUCEApplication::getCommandLineParameters();

        demo_=arguments.containsWholeWord("--demo");

        setLookAndFeel(&look_);

        setWantsKeyboardFocus(true);

        setResizable(true,true);

        setResizeLimits(960,680,1800,1300);

        setSize(1040,720);

        for(auto* c:std::initializer_list<juce::Component*>{
            &root_,&scale_,&genre_,&part_,&sound_,&bars_,&arp_,&count_,&complexity_,&bpm_,&volume_,&low_,&high_,&velocity_,&humanize_,&seed_,&generate_,&mutate_,&preview_,&stop_,&export_,&drag_,&advanced_,&save_,&load_,&pack_,&back_,&forward_,&apply_,&harmony_,&voicing_,&rhythm_,&host_,&resolve_,&colour_,&status_,&seed_label_,&result_,&roll_
        }) addAndMakeVisible(c);

        int id=1;

        for(auto name:{
            "C","C#","D","Eb","E","F","F#","G","Ab","A","Bb","B"
        })root_.addItem(name,id++);

        id=1;

        for(const auto& s:scales())scale_.addItem(juce::String(s.id.data(),s.id.size()).replaceCharacter('-',' '),id++);

        id=1;

        for(const auto& g:p.profiles().profiles)genre_.addItem(g.name,id++);

        part_.addItemList({
            "Chords","Bassline","Arpeggio","Melody","Arrangement"
        },1);

        id=1;

        for(auto s:preview::sounds)sound_.addItem(s,id++);

        for(int i=1;i<=16;++i)bars_.addItem(juce::String(i),i);

        arp_.addItemList({
            "Up","Down","Up / down","Random"
        },1);

        count_.addItemList({
            "8 ideas","16 ideas","32 ideas","64 ideas"
        },1);

        count_.setSelectedId(1);

        for(auto* s:{
            &complexity_,&bpm_,&volume_,&low_,&high_,&velocity_,&humanize_
        }){
            s->setSliderStyle(juce::Slider::LinearHorizontal);

            s->setTextBoxStyle(juce::Slider::TextBoxRight,false,43,23);
        }

        complexity_.setRange(0,100,1);

        bpm_.setRange(20,400,1);

        volume_.setRange(-48,0,1);

        volume_.setTextValueSuffix(" dB");

        low_.setRange(24,96,1);

        high_.setRange(36,108,1);

        velocity_.setRange(1,127,1);

        humanize_.setRange(0,60,1);

        seed_.setInputRestrictions(20,"0123456789");

        seed_.setSelectAllWhenFocused(true);

        seed_label_.setText("SEED",juce::dontSendNotification);

        seed_label_.setFont(11.0f);

        label("ROOT",root_);

        label("SCALE / MODE",scale_);

        label("GENRE",genre_);

        label("PATTERN",part_);

        label("PREVIEW SOUND",sound_);

        label("BARS",bars_);

        label("COMPLEXITY",complexity_);

        label("TEMPO",bpm_);

        label("PREVIEW LEVEL",volume_);

        label("ARP ORDER",arp_);

        label("CHORD LOW",low_);

        label("CHORD HIGH",high_);

        label("VELOCITY",velocity_);

        label("HUMANIZE / TICKS",humanize_);

        label("PACK SIZE",count_);

        generate_.setTooltip("New deterministic seed, new candidate. G. Locked stages are retained.");

        mutate_.setTooltip("Variation of this idea; preserve the rhythmic motif and all but one chord. M.");

        apply_.setTooltip("Generate from the seed shown, without advancing it. Enter also works.");

        preview_.setTooltip("Loop the selected part. Space toggles preview; Escape stops.");

        harmony_.setTooltip("Keep chord roots and qualities. Unlock to change key, scale or bars.");

        voicing_.setTooltip("Keep exact chord pitches. Also locks harmony.");

        rhythm_.setTooltip("Keep every part's note onsets and durations; pitches may change.");

        drag_.setTooltip("Drag a real MIDI file into a compatible DAW. Export MIDI is the file-based fallback.");

        host_.setTooltip("Use valid host BPM for preview. Standalone and hosts without tempo use this idea's BPM.");

        generate_.onClick=[this]{
            generate(false);
        };

        mutate_.onClick=[this]{
            generate(true);
        };

        apply_.onClick=[this]{
            generate(false,false);
        };

        seed_.onReturnKey=apply_.onClick;

        preview_.onClick=[this]{
            processor_.set_playing(!processor_.play->get());
        };

        stop_.onClick=[this]{
            processor_.set_playing(false);
        };

        export_.onClick=[this]{
            choose_file(0);
        };

        save_.onClick=[this]{
            choose_file(1);
        };

        load_.onClick=[this]{
            choose_file(2);
        };

        pack_.onClick=[this]{
            export_pack();
        };

        drag_.drag=[this]{
            drag_midi();
        };

        advanced_.onClick=[this]{
            expanded_=!expanded_;

            setResizeLimits(960,expanded_?790:680,1800,1300);

            advanced_.setButtonText(expanded_?"RIFF MACHINE -":"RIFF MACHINE +");

            setSize(getWidth(),expanded_?850:720);

            resized();
        };

        back_.onClick=[this]{
            if(history_index_>0){
                --history_index_;

                processor_.install(history_[history_index_],true);

                refresh();
            }
        };

        forward_.onClick=[this]{
            if(history_index_+1<history_.size()){
                ++history_index_;

                processor_.install(history_[history_index_],true);

                refresh();
            }
        };

        harmony_.onClick=voicing_.onClick=rhythm_.onClick=[this]{
            locks_changed();
        };

        part_.onChange=[this]{
            if(!refreshing_)select_part();
        };

        sound_.onChange=[this]{
            if(!refreshing_){
                processor_.sound->beginChangeGesture();

                processor_.sound->setValueNotifyingHost(processor_.sound->convertTo0to1(static_cast<float>(sound_.getSelectedId()-1)));

                processor_.sound->endChangeGesture();
            }
        };

        volume_.onValueChange=[this]{
            if(!refreshing_){
                processor_.output->setValueNotifyingHost(processor_.output->convertTo0to1(static_cast<float>(volume_.getValue())));
            }
        };

        bpm_.onValueChange=[this]{
            if(!refreshing_)processor_.tempo->setValueNotifyingHost(processor_.tempo->convertTo0to1(static_cast<float>(bpm_.getValue())));
        };

        bpm_.onDragStart=[this]{
            processor_.tempo->beginChangeGesture();
        };

        bpm_.onDragEnd=[this]{
            processor_.tempo->endChangeGesture();
        };

        volume_.onDragStart=[this]{
            processor_.output->beginChangeGesture();
        };

        volume_.onDragEnd=[this]{
            processor_.output->endChangeGesture();
        };

        host_.onClick=[this]{
            processor_.host_tempo->setValueNotifyingHost(host_.getToggleState()?1.0f:0.0f);
        };

        genre_.onChange=[this]{
            if(!refreshing_){
                const auto& g=processor_.profiles().profiles.at(static_cast<std::size_t>(genre_.getSelectedId()-1));

                bpm_.setValue(g.bpm_default);

                constexpr std::array<const char*,10> ids={
                    "piano","electric-piano","warm-pad","supersaw","reese","hoover","rave-stab","pluck","acid-bass","sine-sub"
                };

                for(std::size_t i=0;i<ids.size();++i)if(g.preview==ids[i])sound_.setSelectedId(static_cast<int>(i+1));
            }
        };

        for(auto* c:{
            &root_,&scale_,&bars_,&genre_
        })c->setTooltip("Select then Generate. Locked stages are preserved; unlock before changing key, scale or length.");

        drag_directory_=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("RiffCalc-midi");

        drag_directory_.createDirectory();

        // Only this product's old MIDI files; retain recent drops for asynchronous importers.
        for(const auto& file:drag_directory_.findChildFiles(juce::File::findFiles,false,"RiffCalc-*.mid"))if(file.getLastModificationTime()<juce::Time::getCurrentTime()-juce::RelativeTime::days(7))file.deleteFile();

        if(p.wrapperType==juce::AudioProcessor::wrapperType_Standalone&&!demo_) {
            auto last=settings().getChildFile("last-session.riffcalc.json");

            if(!last.existsAsFile())last=juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("PACKRAT/last-session.packrat.json");

            if(last.existsAsFile()&&last.getSize()<=2*1024*1024)run([&]{
                p.install(deserialize(last.loadFileAsString().toStdString(),p.profiles()),true);
            });
        }

        for(auto* c:std::initializer_list<juce::Component*>{
            &menu_,&open_tool_,&save_tool_,&export_tool_,&previous_tool_,&next_tool_,&generate_tool_,&mutate_tool_,&help_tool_,&piano_tab_,&events_tab_,&sheet_,&context_status_,&selection_status_
        })addAndMakeVisible(c);

        int focus=1;
        for(auto* c:std::initializer_list<juce::Component*>{
            &root_,&scale_,&genre_,&bars_,&part_,&sound_,&complexity_,&bpm_,&seed_,&apply_,&host_,&volume_,&piano_tab_,&events_tab_,&advanced_,&generate_,&mutate_,&preview_,&stop_,&export_,&drag_
        })c->setExplicitFocusOrder(focus++);

        setFocusContainerType(juce::Component::FocusContainerType::keyboardFocusContainer);

        open_tool_.onClick=[this]{
            perform(Open);
        };
        save_tool_.onClick=[this]{
            perform(Save);
        };
        export_tool_.onClick=[this]{
            perform(Export);
        };
        previous_tool_.onClick=[this]{
            perform(Previous);
        };
        next_tool_.onClick=[this]{
            perform(Next);
        };
        generate_tool_.onClick=[this]{
            perform(New);
        };
        mutate_tool_.onClick=[this]{
            perform(Mutate);
        };
        help_tool_.onClick=[this]{
            perform(QuickStart);
        };

        piano_tab_.onClick=[this]{
            view_events(false);
        };
        events_tab_.onClick=[this]{
            view_events(true);
        };

        sheet_.onVelocity=[this](std::size_t row,int value){
            edit_velocity(row,value);
        };
        sheet_.onInspect=[this](const juce::String& text){
            status_.setText(text,juce::dontSendNotification);
        };

        open_tool_.setTooltip("Open Idea (Ctrl+O)");
        save_tool_.setTooltip("Save Idea As (Ctrl+Shift+S)");
        export_tool_.setTooltip("Export MIDI (Ctrl+E)");
        previous_tool_.setTooltip("Previous candidate (Alt+Left)");
        next_tool_.setTooltip("Next candidate (Alt+Right)");

        piano_tab_.setComponentID("piano-tab");
        part_.setComponentID("pattern-selector");
        selection_status_.setComponentID("event-count");
        events_tab_.setComponentID("events-tab");
        menu_.setComponentID("application-menu");

        view_events(false);

        if(demo_){
            Document demonstration;
            Config c;
            c.seed=1999;
            c.complexity=75;

            demonstration.idea=make_idea(c,p.profiles(),"liquid-dnb");
            p.install(std::move(demonstration),true);
        }

        history_.push_back(p.snapshot());

        refresh();

        if(status_.getText().isEmpty())status_.setText("Ready. Pick a key and genre, then Generate.",juce::dontSendNotification);

        resized();

        startTimerHz(24);

        if(demo_){
            if(arguments.contains("--events"))view_events(true);

            if(arguments.contains("--advanced"))perform(Advanced);

            if(arguments.contains("--about")){
                auto safe=juce::Component::SafePointer<Editor>(this);
                juce::MessageManager::callAsync([safe]{
                    if(safe)safe->show_about();
                });
            }
        }
    }

    Editor::~Editor() {
        stopTimer();

        dialog_.reset();

        if(themed_parent_)themed_parent_->setLookAndFeel(nullptr);

        menu_.setModel(nullptr);

        if(processor_.wrapperType==juce::AudioProcessor::wrapperType_Standalone&&!demo_) {
            try {
                settings().createDirectory();

                settings().getChildFile("last-session.riffcalc.json").replaceWithText(serialize(processor_.snapshot()));
            } catch(const std::exception&){
            }
        }

        setLookAndFeel(nullptr);
    }

    void Editor::run(const std::function<void()>& fn) {
        try{
            fn();
        } catch(const std::exception& e){
            status_.setText(juce::String("Could not complete: ")+e.what(),juce::dontSendNotification);
        }
        status_.setTooltip(status_.getText());
    }

    Config Editor::config() const {
        auto c=processor_.snapshot().idea.harmony.config;

        c.key=parse_key(root_.getText().toStdString());

        c.scale=scales().at(static_cast<std::size_t>(scale_.getSelectedId()-1)).scale;

        c.bars=bars_.getSelectedId();

        c.complexity=static_cast<int>(complexity_.getValue());

        c.bpm=static_cast<int>(bpm_.getValue());

        c.low=static_cast<int>(low_.getValue());

        c.high=static_cast<int>(high_.getValue());

        c.velocity=static_cast<int>(velocity_.getValue());

        c.humanize=static_cast<int>(humanize_.getValue());

        c.resolve=resolve_.getToggleState();

        c.colour=colour_.getToggleState();

        const auto text=seed_.getText().toStdString();

        auto [end,error]=std::from_chars(text.data(),text.data()+text.size(),c.seed);

        if(text.empty()||error!=std::errc{
        }

        ||end!=text.data()+text.size())throw std::invalid_argument("Seed must be an unsigned 64-bit decimal integer");

        validate(c);

        return c;
    }

    void Editor::remember(Document d) {
        if(history_index_+1<history_.size())history_.erase(history_.begin()+static_cast<std::ptrdiff_t>(history_index_+1),history_.end());

        processor_.install(d);

        history_.push_back(std::move(d));

        if(history_.size()>32)history_.erase(history_.begin());

        history_index_=history_.size()-1;

        refresh();
    }

    void Editor::generate(bool mutation,bool new_seed) {
        run([&]{
            auto d=processor_.snapshot();

            auto c=config();

            if(new_seed){
                Random random(c.seed);

                c.seed=random.next();
            }

            const auto& genre=processor_.profiles().profiles.at(static_cast<std::size_t>(genre_.getSelectedId()-1));

            auto idea=make_idea(c,processor_.profiles(),genre.id,static_cast<ArpOrder>(arp_.getSelectedId()-1),&d.idea,d.locks,mutation);

            d.idea=std::move(idea);

            remember(std::move(d));

            status_.setText(mutation?"Variation ready. Previous candidates remain in history.":"New idea ready. Preview it, mutate it, or drag the MIDI.",juce::dontSendNotification);
        });
    }

    void Editor::select_part(){
        run([&]{
            auto d=processor_.snapshot();

            d.part=static_cast<Part>(part_.getSelectedId()-1);

            processor_.install(d);

            roll_.set(d.idea,d.part);

            sheet_.set(audition_clip(d.idea,d.part));
            selection_status_.setText(juce::String(audition_clip(d.idea,d.part).notes.size())+" events",juce::dontSendNotification);

            revision_=processor_.revision.load();
        });
    }

    void Editor::locks_changed(){
        run([&]{
            auto d=processor_.snapshot();

            d.locks={
                harmony_.getToggleState(),voicing_.getToggleState(),rhythm_.getToggleState()
            };

            if(d.locks.voicing)d.locks.harmony=true;

            processor_.install(std::move(d));

            refresh();
        });
    }

    void Editor::refresh() {
        refreshing_=true;

        const auto d=processor_.snapshot();

        const auto& c=d.idea.harmony.config;

        root_.setSelectedId(c.key.root+1,juce::dontSendNotification);

        scale_.setSelectedId(static_cast<int>(c.scale)+1,juce::dontSendNotification);

        for(std::size_t i=0;i<processor_.profiles().profiles.size();++i)if(processor_.profiles().profiles[i].id==d.idea.harmony.genre)genre_.setSelectedId(static_cast<int>(i+1),juce::dontSendNotification);

        part_.setSelectedId(static_cast<int>(d.part)+1,juce::dontSendNotification);

        sound_.setSelectedId(d.sound+1,juce::dontSendNotification);

        bars_.setSelectedId(c.bars,juce::dontSendNotification);

        arp_.setSelectedId(static_cast<int>(d.idea.arp)+1,juce::dontSendNotification);

        complexity_.setValue(c.complexity,juce::dontSendNotification);

        bpm_.setValue(c.bpm,juce::dontSendNotification);

        volume_.setValue(d.output_db,juce::dontSendNotification);

        low_.setValue(c.low,juce::dontSendNotification);

        high_.setValue(c.high,juce::dontSendNotification);

        velocity_.setValue(c.velocity,juce::dontSendNotification);

        humanize_.setValue(c.humanize,juce::dontSendNotification);

        seed_.setText(std::to_string(c.seed),false);

        harmony_.setToggleState(d.locks.harmony,juce::dontSendNotification);

        voicing_.setToggleState(d.locks.voicing,juce::dontSendNotification);

        rhythm_.setToggleState(d.locks.rhythm,juce::dontSendNotification);

        host_.setToggleState(d.follow_host,juce::dontSendNotification);

        resolve_.setToggleState(c.resolve,juce::dontSendNotification);

        colour_.setToggleState(c.colour,juce::dontSendNotification);

        root_.setEnabled(!d.locks.harmony);

        scale_.setEnabled(!d.locks.harmony);

        bars_.setEnabled(!d.locks.harmony&&!d.locks.rhythm);

        low_.setEnabled(!d.locks.voicing);

        high_.setEnabled(!d.locks.voicing);

        roll_.set(d.idea,d.part);

        sheet_.set(audition_clip(d.idea,d.part));

        result_.setText(juce::String(c.key.name())+" / "+juce::String(scale_info(c.scale).id.data())+"    |    "+juce::String(c.bars)+" bars",juce::dontSendNotification);

        context_status_.setText(c.key.name()+" / "+std::string(scale_info(c.scale).id),juce::dontSendNotification);

        selection_status_.setText(juce::String(audition_clip(d.idea,d.part).notes.size())+" events",juce::dontSendNotification);

        previous_tool_.setEnabled(history_index_>0);
        next_tool_.setEnabled(history_index_+1<history_.size());

        back_.setEnabled(history_index_>0);

        forward_.setEnabled(history_index_+1<history_.size());

        revision_=processor_.revision.load();

        refreshing_=false;

        repaint();
    }

    void Editor::timerCallback() {
        if(processor_.wrapperType==juce::AudioProcessor::wrapperType_Standalone&&!themed_parent_)if(auto* parent=findParentComponentOfClass<juce::DocumentWindow>()){
            themed_parent_=parent;
            parent->setLookAndFeel(&look_);
            parent->repaint();
        }

        if(revision_!=processor_.revision.load())refresh();

        const bool playing=processor_.play->get();

        preview_.setButtonText(playing?"PLAYING":"PLAY");

        preview_.setToggleState(playing,juce::dontSendNotification);

        roll_.position(playing?processor_.position.load(std::memory_order_relaxed):0);

        if(!sound_.isPopupActive()){
            refreshing_=true;

            sound_.setSelectedId(processor_.sound->getIndex()+1,juce::dontSendNotification);

            volume_.setValue(processor_.output->get(),juce::dontSendNotification);

            bpm_.setValue(processor_.tempo->get(),juce::dontSendNotification);

            host_.setToggleState(processor_.host_tempo->get(),juce::dontSendNotification);

            refreshing_=false;
        }

        if(processor_.state_error.exchange(false))status_.setText("Session could not be read; current idea was retained.",juce::dontSendNotification);

        repaint(juce::Rectangle<int>(getWidth()-155,68,120,12));
    }

    void Editor::choose_file(int action) {
        auto initial=juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(action==0?"RiffCalc.mid":"idea.riffcalc.json");

        chooser_=std::make_unique<juce::FileChooser>(action==2?"Open RiffCalc idea":action==1?"Save a new RiffCalc idea":"Export MIDI (choose a new filename)",initial,action==0?"*.mid":"*.riffcalc.json;*.packrat.json",true);

        auto safe=juce::Component::SafePointer<Editor>(this);

        chooser_->launchAsync((action==2?juce::FileBrowserComponent::openMode:juce::FileBrowserComponent::saveMode)|juce::FileBrowserComponent::canSelectFiles,[safe,action](const juce::FileChooser& fc){
            if(!safe)return;

            const auto file=fc.getResult();

            if(file==juce::File{
            })return;

            safe->run([&]{
                if(action==2){
                    if(file.getSize()>2*1024*1024)throw std::runtime_error("Idea file exceeds 2 MiB");

                    auto d=deserialize(file.loadFileAsString().toStdString(),safe->processor_.profiles());

                    safe->processor_.install(d,true);

                    safe->remember(std::move(d));
                } else if(action==1)write_text(file.getFileName().endsWithIgnoreCase(".riffcalc.json")||file.getFileName().endsWithIgnoreCase(".packrat.json")?file:file.getSiblingFile(file.getFileNameWithoutExtension()+".riffcalc.json"),serialize(safe->processor_.snapshot()));

                else {
                    const auto bytes=export_bytes(safe->processor_.snapshot());

                    write_new(path(file.withFileExtension("mid")),bytes);
                }

                safe->status_.setText(action==2?"Idea opened.":"Saved: "+file.getFileName(),juce::dontSendNotification);
            });
        });
    }

    void Editor::drag_midi(){
        run([&]{
            auto file=drag_directory_.getChildFile("RiffCalc-"+juce::Uuid().toString()+".mid");

            auto bytes=export_bytes(processor_.snapshot());

            write_new(path(file),bytes);

            if(!juce::DragAndDropContainer::performExternalDragDropOfFiles({
                file.getFullPathName()
            },false,&drag_))throw std::runtime_error("Native drag did not start. Use Export MIDI instead.");

            status_.setText("MIDI drag started. Recent drag files remain available for seven days.",juce::dontSendNotification);
        });
    }

    void Editor::export_pack() {
        chooser_=std::make_unique<juce::FileChooser>("Choose the parent folder for a new MIDI pack",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),"",true);

        auto safe=juce::Component::SafePointer<Editor>(this);

        chooser_->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectDirectories,[safe](const juce::FileChooser& fc){
            if(!safe)return;

            auto parent=fc.getResult();

            if(parent==juce::File{
            })return;

            safe->run([&]{
                auto d=safe->processor_.snapshot();

                const int count=8<<(safe->count_.getSelectedId()-1);

                auto folder=parent.getChildFile("RiffCalc_"+juce::String(d.idea.harmony.genre)+"_"+juce::Time::getCurrentTime().formatted("%Y%m%d-%H%M%S"));

                packrat::export_pack(path(folder),d,safe->processor_.profiles(),count);

                safe->status_.setText("Exported "+juce::String(count)+" ideas + companion MIDI and metadata to "+folder.getFileName(),juce::dontSendNotification);
            });
        });
    }
}
