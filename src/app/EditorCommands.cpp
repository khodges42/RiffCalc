// SPDX-License-Identifier: GPL-3.0-or-later
#include "Editor.hpp"
#include <BinaryData.h>
namespace packrat::app {
    void Editor::view_events(bool events){
        event_view_=events;
        roll_.setVisible(!events);
        sheet_.setVisible(events);
        piano_tab_.setToggleState(!events,juce::dontSendNotification);
        events_tab_.setToggleState(events,juce::dontSendNotification);
    }

    void Editor::edit_velocity(std::size_t row,int value){
        run([&]{
            if(value<1||value>127)throw std::invalid_argument("Velocity must be 1-127");
            auto d=processor_.snapshot();
            const auto clip=audition_clip(d.idea,d.part);
            if(row>=clip.notes.size())return;

            const auto note=clip.notes[row];
            std::size_t part=static_cast<std::size_t>(d.part);

            if(d.part==Part::Arrangement){
                part=4;
                for(std::size_t i=0;i<4;++i)if(std::find(d.idea.parts[i].notes.begin(),d.idea.parts[i].notes.end(),note)!=d.idea.parts[i].notes.end()){
                    part=i;
                    break;
                }
            }

            if(part>=4)return;

            auto& notes=d.idea.parts[part].notes;
            auto it=std::find(notes.begin(),notes.end(),note);
            if(it==notes.end())return;
            it->velocity=value;
            if(part==0)d.idea.harmony.clip=d.idea.parts[0];
            remember(std::move(d));
            status_.setText("Velocity updated. Previous candidate remains in history.",juce::dontSendNotification);
        });
    }

    juce::StringArray Editor::getMenuBarNames(){
        return {
            "File","Edit","View","Generate","Tools","Help"
        };
    }

    juce::PopupMenu Editor::getMenuForIndex(int index,const juce::String&) {
        juce::PopupMenu m;
        switch(index){
            case 0:m.addItem(Open,"Open Idea...\tCtrl+O");
            m.addItem(Save,"Save Idea As...\tCtrl+Shift+S");
            m.addSeparator();
            m.addItem(Export,"Export MIDI...\tCtrl+E");
            m.addItem(Pack,"Export Pack...");
            break;

            case 1:m.addItem(Previous,"Previous Candidate\tAlt+Left",history_index_>0);
            m.addItem(Next,"Next Candidate\tAlt+Right",history_index_+1<history_.size());
            m.addSeparator();
            m.addItem(Events,"Edit Note Velocities...");
            break;

            case 2:m.addItem(Piano,"Piano Roll",true,!event_view_);
            m.addItem(Events,"Event Sheet",true,event_view_);
            m.addSeparator();
            m.addItem(Advanced,"Riff Machine Properties",true,expanded_);
            break;

            case 3:m.addItem(New,"New Candidate\tG");
            m.addItem(Mutate,"Mutate Candidate\tM");
            m.addItem(UseSeed,"Regenerate from Seed\tF5");
            break;

            case 4:m.addItem(Play,"Play / Stop Preview\tSpace",true,processor_.play->get());
            m.addItem(Stop,"Stop Preview\tEsc");
            m.addSeparator();
            m.addItem(Advanced,"Generation Properties...");
            m.addItem(Pack,"Pack Factory...");
            break;

            case 5:m.addItem(QuickStart,"RiffCalc Help\tF1");
            m.addItem(License,"License Information...");
            m.addItem(Repository,"Project Repository");
            m.addSeparator();
            m.addItem(About,"About RiffCalc...");
            break;

            default:break;
        }
        return m;
    }

    void Editor::menuItemSelected(int id,int){
        if(id>=Open&&id<=Repository)perform(static_cast<Command>(id));
    }

    void Editor::perform(Command c){
        switch(c){
            case Open:choose_file(2);
            break;
            case Save:choose_file(1);
            break;
            case Export:choose_file(0);
            break;
            case Pack:export_pack();
            break;

            case Previous:back_.onClick();
            break;
            case Next:forward_.onClick();
            break;
            case New:generate(false);
            break;
            case Mutate:generate(true);
            break;
            case UseSeed:generate(false,false);
            break;

            case Play:processor_.set_playing(!processor_.play->get());
            break;
            case Stop:processor_.set_playing(false);
            break;
            case Piano:view_events(false);
            break;
            case Events:view_events(true);
            break;
            case Advanced:advanced_.onClick();
            break;

            case QuickStart:show_text("RiffCalc Help",juce::String::fromUTF8(BinaryData::quickstart_md,BinaryData::quickstart_mdSize));
            break;

            case License:show_text("RiffCalc License",juce::String::fromUTF8(BinaryData::LICENSE,BinaryData::LICENSESize));
            break;

            case Repository:juce::URL("https://github.com/khodges42/RiffCalc").launchInDefaultBrowser();
            break;
            case About:show_about();
            break;
        }
    }

    void Editor::show_text(const juce::String& title,const juce::String& text){
        dialog_=std::make_unique<ClassicDialog>(title,new TextPanel(text),look_,this);
        dialog_->enterModalState(true,nullptr,false);
    }

    void Editor::show_about(){
        auto panel=std::make_unique<AboutPanel>();
        auto safe=juce::Component::SafePointer<Editor>(this);
        panel->onLicense=[safe]{
            juce::MessageManager::callAsync([safe]{
                if(safe)safe->perform(License);
            });
        };
        dialog_=std::make_unique<ClassicDialog>("About RiffCalc 2000 Pro",panel.release(),look_,this);
        dialog_->enterModalState(true,nullptr,false);
    }

    bool Editor::keyPressed(const juce::KeyPress& key) {
        const auto mods=key.getModifiers();
        const int code=key.getKeyCode();

        if(mods.isCommandDown()){
            if(code=='O'){
                perform(Open);
                return true;
            }
            if(code=='S'&&mods.isShiftDown()){
                perform(Save);
                return true;
            }
            if(code=='E'){
                perform(Export);
                return true;
            }
        }

        if(code==juce::KeyPress::F1Key){
            perform(QuickStart);
            return true;
        }
        if(code==juce::KeyPress::F5Key){
            perform(UseSeed);
            return true;
        }

        if(mods.isAltDown()&&code==juce::KeyPress::leftKey){
            perform(Previous);
            return true;
        }
        if(mods.isAltDown()&&code==juce::KeyPress::rightKey){
            perform(Next);
            return true;
        }

        if(dynamic_cast<juce::TextEditor*>(juce::Component::getCurrentlyFocusedComponent()))return false;

        if(code==juce::KeyPress::spaceKey){
            perform(Play);
            return true;
        }
        if(code==juce::KeyPress::escapeKey){
            perform(Stop);
            return true;
        }

        if(!mods.isCommandDown()&&!mods.isAltDown()){
            const auto ch=key.getTextCharacter();
            if(ch=='g'||ch=='G'){
                perform(New);
                return true;
            }
            if(ch=='m'||ch=='M'){
                perform(Mutate);
                return true;
            }
        }
        return false;
    }
}
