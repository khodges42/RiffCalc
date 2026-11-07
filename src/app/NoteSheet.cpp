// SPDX-License-Identifier: GPL-3.0-or-later
#include "NoteSheet.hpp"
namespace packrat::app {
    NoteSheet::NoteSheet() {
        addAndMakeVisible(table_);
        table_.setRowHeight(21);
        table_.setHeaderHeight(24);
        table_.setOutlineThickness(1);
        table_.setMultipleSelectionEnabled(false);

        int id=1;
        for(const auto& c:std::initializer_list<std::pair<const char*,int>>{
            {
                "#",45
            },{
                "Note",115
            },{
                "Position",150
            },{
                "Start (ticks)",135
            },{
                "Length (ticks)",135
            },{
                "Velocity",105
            },{
                "Channel",95
            }
        })table_.getHeader().addColumn(c.first,id++,c.second,40,400,juce::TableHeaderComponent::visible|juce::TableHeaderComponent::resizable);

        table_.setTooltip("Inspect MIDI note events. Double-click Velocity to edit; Enter commits, Escape cancels.");
    }

    void NoteSheet::set(Clip clip){
        ++epoch_;
        clip_=std::move(clip);
        table_.updateContent();
        table_.repaint();
    }

    void NoteSheet::resized(){
        table_.setBounds(getLocalBounds());
        table_.getHeader().resizeAllColumnsToFit(getWidth()-16);
    }

    void NoteSheet::paintRowBackground(juce::Graphics& g,int row,int width,int height,bool selected) {
        g.fillAll(selected?juce::Colour(0xffe4e8f0):juce::Colours::white);
        g.setColour(juce::Colour(0xffd8d8d8));
        g.drawHorizontalLine(height-1,0,float(width));
        g.setColour(classic::face);
        g.fillRect(0,0,table_.getHeader().getColumnWidth(1),height);
    }

    void NoteSheet::paintCell(juce::Graphics& g,int row,int col,int width,int height,bool) {
        if(row<0||row>=getNumRows())return;
        const auto& n=clip_.notes[static_cast<std::size_t>(row)];
        juce::String text;

        switch(col){
            case 1:text=juce::String(row+1);
            break;
            case 2:text=juce::MidiMessage::getMidiNoteName(n.pitch,true,true,4);
            break;
            case 3:text=juce::String(n.start/bar_ticks+1)+":"+juce::String(n.start%bar_ticks/ppq+1)+":"+juce::String(n.start%ppq).paddedLeft('0',3);
            break;
            case 4:text=juce::String(n.start);
            break;
            case 5:text=juce::String(n.duration);
            break;
            case 7:text=juce::String(n.channel+1);
            break;
            default:break;
        }

        g.setColour(classic::ink);
        g.setFont(juce::Font("Tahoma",12.0f,juce::Font::plain));
        g.drawText(text,5,0,width-10,height,juce::Justification::centredLeft);
        g.setColour(juce::Colour(0xffc0c0c0));
        g.drawVerticalLine(width-1,0,float(height));
    }

    juce::Component* NoteSheet::refreshComponentForCell(int row,int col,bool,juce::Component* existing) {
        if(col!=6){
            delete existing;
            return nullptr;
        }
        auto* label=dynamic_cast<juce::Label*>(existing);
        if(!label)label=new juce::Label;

        label->onTextChange=nullptr;
        label->setEditable(false,true,false);
        label->setJustificationType(juce::Justification::centredLeft);
        label->setColour(juce::Label::outlineColourId,juce::Colour(0xffd8d8d8));

        if(row>=0&&row<getNumRows()){
            label->setText(juce::String(clip_.notes[static_cast<std::size_t>(row)].velocity),juce::dontSendNotification);
            label->onTextChange=[this,label,row]{
                const auto text=label->getText();
                if(text.containsOnly("0123456789")&&text.isNotEmpty()&&text.getIntValue()>=1&&text.getIntValue()<=127){
                    auto safe=juce::Component::SafePointer<NoteSheet>(this);
                    const auto epoch=epoch_;
                    const int value=text.getIntValue();
                    juce::MessageManager::callAsync([safe,epoch,row,value]{
                        if(safe&&safe->epoch_==epoch&&safe->onVelocity)safe->onVelocity(static_cast<std::size_t>(row),value);
                    });
                } else label->setText(juce::String(clip_.notes[static_cast<std::size_t>(row)].velocity),juce::dontSendNotification);
            };
        }

        return label;
    }

    void NoteSheet::selectedRowsChanged(int row){
        if(row>=0&&row<getNumRows()&&onInspect){
            const auto& n=clip_.notes[static_cast<std::size_t>(row)];
            onInspect("Event "+juce::String(row+1)+"  /  "+juce::MidiMessage::getMidiNoteName(n.pitch,true,true,4)+"  /  velocity "+juce::String(n.velocity));
        }
    }
}
