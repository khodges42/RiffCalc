// SPDX-License-Identifier: GPL-3.0-or-later
#include "Editor.hpp"
namespace packrat::app {
    void Editor::paint(juce::Graphics& g) {
        g.fillAll(classic::face);
        classic::bevel(g,{
            2,25,getWidth()-4,29
        });

        g.setColour(classic::navy);
        g.fillRect(8,59,getWidth()-16,34);
        classic::logo(g,{
            14,64,24,24
        });
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(19.0f,juce::Font::bold));
        g.drawText("RiffCalc 2000 Pro",48,59,300,34,juce::Justification::centredLeft);

        g.setFont(11.0f);
        g.drawText("Workgroup Edition   /   Voidmutt Enterprises",354,59,getWidth()-535,34,juce::Justification::centredLeft);

        g.setColour(juce::Colours::black);
        g.fillRect(getWidth()-155,70,131,11);
        g.setColour(juce::Colour(0xff008040));
        g.fillRect(getWidth()-153,72,int(127*std::min(1.0f,processor_.meter.load()*3)),7);

        classic::group(g,{
            8,100,getWidth()-16,155
        },"Pattern Specifications");

        const int bottom=getHeight()-25,left=getWidth()-400;

        for(auto r:{
            juce::Rectangle<int>(3,bottom,left-3,22),juce::Rectangle<int>(left+2,bottom,204,22),juce::Rectangle<int>(left+210,bottom,187,22)
        })classic::bevel(g,r,true);

        if(expanded_)classic::group(g,{
            8,getHeight()-215,getWidth()-16,178
        },"Riff Machine Properties");

        g.setColour(juce::Colours::grey);
        g.drawHorizontalLine(292,8,float(getWidth()-8));
        g.setColour(juce::Colours::white);
        g.drawHorizontalLine(293,8,float(getWidth()-8));
    }

    void Editor::resized() {
        const int w=getWidth(),margin=22,gap=10,cell=(w-2*margin-5*gap)/6;
        menu_.setBounds(3,1,w-6,23);

        int tx=7;
        for(auto* b:{
            &open_tool_,&save_tool_,&export_tool_,&previous_tool_,&next_tool_,&generate_tool_,&mutate_tool_,&help_tool_
        }){
            const int width=(b==&previous_tool_||b==&generate_tool_)?100:86;
            b->setBounds(tx,28,width,23);
            tx+=width+3;
        }

        int x=margin;
        auto place=[&](juce::Component& c,int units){
            c.setBounds(x,132,cell*units+gap*(units-1),24);
            x+=cell*units+gap*units;
        };
        place(root_,1);
        place(scale_,2);
        place(genre_,2);
        place(bars_,1);

        const int quarter=(w-2*margin-3*gap)/4;

        part_.setBounds(margin,180,quarter,24);
        sound_.setBounds(margin+quarter+gap,180,quarter,24);
        complexity_.setBounds(margin+2*(quarter+gap),180,quarter,24);
        bpm_.setBounds(margin+3*(quarter+gap),180,quarter,24);

        seed_label_.setBounds(margin,221,34,23);
        seed_.setBounds(margin+39,221,184,23);
        apply_.setBounds(margin+232,221,85,23);
        host_.setBounds(margin+330,221,115,23);
        volume_.setBounds(w-257,221,233,23);

        back_.setBounds(12,264,28,23);
        forward_.setBounds(44,264,28,23);
        result_.setBounds(84,262,w-536,26);

        piano_tab_.setBounds(w-450,264,110,24);
        events_tab_.setBounds(w-336,264,110,24);
        advanced_.setBounds(w-212,264,200,24);

        const int extra=expanded_?185:0;
        roll_.setBounds(8,300,w-16,getHeight()-300-80-extra);
        sheet_.setBounds(roll_.getBounds());

        const int actions=roll_.getBottom()+11,width=(w-44-5*8)/6;
        std::array<juce::Component*,6> buttons={
            &generate_,&mutate_,&preview_,&stop_,&export_,&drag_
        };
        for(int i=0;i<6;++i)buttons[static_cast<std::size_t>(i)]->setBounds(22+i*(width+8),actions,width,28);

        for(auto* c:std::initializer_list<juce::Component*>{
            &arp_,&count_,&low_,&high_,&velocity_,&humanize_,&harmony_,&voicing_,&rhythm_,&resolve_,&colour_,&save_,&load_,&pack_
        })c->setVisible(expanded_);

        if(expanded_){
            const int y=getHeight()-215,col=(w-52)/4;

            harmony_.setBounds(22,y+24,105,23);
            voicing_.setBounds(137,y+24,105,23);
            rhythm_.setBounds(252,y+24,105,23);
            resolve_.setBounds(375,y+24,161,23);
            colour_.setBounds(551,y+24,174,23);

            arp_.setBounds(22,y+72,col-12,24);
            low_.setBounds(22+col,y+72,col-12,24);
            high_.setBounds(22+2*col,y+72,col-12,24);
            velocity_.setBounds(22+3*col,y+72,col-12,24);

            humanize_.setBounds(22,y+129,col-12,24);
            count_.setBounds(22+col,y+129,col-12,24);
            pack_.setBounds(22+2*col,y+127,col-12,27);
            save_.setBounds(22+3*col,y+108,col-12,24);
            load_.setBounds(22+3*col,y+137,col-12,24);
        }

        for(auto& l:labels_)if(auto* c=l->getAttachedComponent()){
            l->setVisible(c->isVisible());
            l->setBounds(c->getX(),c->getY()-16,c->getWidth(),14);
        }

        const int left=w-400;
        status_.setBounds(7,getHeight()-25,left-12,22);
        status_.setFont(11.0f);
        context_status_.setBounds(left+8,getHeight()-25,194,22);
        context_status_.setFont(11.0f);
        selection_status_.setBounds(left+216,getHeight()-25,177,22);
        selection_status_.setFont(11.0f);
    }
}
