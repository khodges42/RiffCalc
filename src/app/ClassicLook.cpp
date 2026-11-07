// SPDX-License-Identifier: GPL-3.0-or-later
#include "ClassicLook.hpp"
namespace packrat::app {
    void classic::bevel(juce::Graphics& g,juce::Rectangle<int> r,bool down) {
        g.setColour(down?juce::Colours::darkgrey:juce::Colours::white);

        g.drawHorizontalLine(r.getY(),float(r.getX()),float(r.getRight()));

        g.drawVerticalLine(r.getX(),float(r.getY()),float(r.getBottom()));

        g.setColour(down?juce::Colours::white:juce::Colours::darkgrey);

        g.drawHorizontalLine(r.getBottom()-1,float(r.getX()),float(r.getRight()));

        g.drawVerticalLine(r.getRight()-1,float(r.getY()),float(r.getBottom()));
    }

    void classic::group(juce::Graphics& g,juce::Rectangle<int> r,const juce::String& name) {
        auto border=r.withTrimmedTop(7);
        g.setColour(juce::Colour(0xff808080));
        g.drawRect(border);
        g.setColour(juce::Colours::white);
        g.drawRect(border.translated(1,1));

        g.setFont(12.0f);
        const int w=std::max(220,juce::Font(12.0f).getStringWidth(name)+32);
        g.setColour(face);
        g.fillRect(r.getX()+9,r.getY(),w,16);
        g.setColour(ink);
        g.drawText(name,r.getX()+15,r.getY(),w,16,juce::Justification::centredLeft);
    }

    void classic::logo(juce::Graphics& g,juce::Rectangle<int> r) {
        g.setColour(paper);
        g.fillRect(r);
        g.setColour(ink);
        g.drawRect(r);

        const float cell=float(r.getWidth())/5;
        g.setColour(navy);
        g.fillRect(float(r.getX()+1),float(r.getY()+1),float(r.getWidth()-2),cell);

        for(int i=1;i<5;++i){
            g.setColour(face);
            g.drawVerticalLine(r.getX()+int(i*cell),r.getY()+cell,float(r.getBottom()-1));
            g.drawHorizontalLine(r.getY()+int((i+1)*cell),float(r.getX()+1),float(r.getRight()-1));
        }

        for(int i=0;i<4;++i){
            g.setColour(green);
            g.fillRect(r.getX()+3+i*cell,r.getY()+r.getHeight()-cell*(i+1)+2,cell*1.35f,cell*.48f);
        }
    }

    RetroLook::RetroLook() {
        setDefaultSansSerifTypefaceName("Tahoma");

        setColour(juce::ResizableWindow::backgroundColourId,classic::face);

        setColour(juce::Label::textColourId,classic::ink);
        setColour(juce::TextButton::textColourOffId,classic::ink);
        setColour(juce::TextButton::textColourOnId,classic::ink);

        setColour(juce::ComboBox::textColourId,classic::ink);
        setColour(juce::ComboBox::backgroundColourId,classic::paper);
        setColour(juce::ComboBox::outlineColourId,juce::Colours::grey);

        setColour(juce::PopupMenu::backgroundColourId,classic::face);
        setColour(juce::PopupMenu::textColourId,classic::ink);
        setColour(juce::PopupMenu::highlightedBackgroundColourId,classic::navy);
        setColour(juce::PopupMenu::highlightedTextColourId,juce::Colours::white);

        setColour(juce::Slider::textBoxBackgroundColourId,classic::paper);
        setColour(juce::Slider::textBoxTextColourId,classic::ink);
        setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::grey);

        setColour(juce::ToggleButton::textColourId,classic::ink);
        setColour(juce::ToggleButton::tickColourId,classic::ink);

        setColour(juce::TextEditor::backgroundColourId,classic::paper);
        setColour(juce::TextEditor::textColourId,classic::ink);
        setColour(juce::TextEditor::highlightColourId,classic::navy);
        setColour(juce::TextEditor::highlightedTextColourId,juce::Colours::white);

        setColour(juce::ListBox::backgroundColourId,classic::paper);
        setColour(juce::ListBox::outlineColourId,juce::Colours::grey);

        setColour(juce::TableHeaderComponent::backgroundColourId,classic::face);
        setColour(juce::TableHeaderComponent::textColourId,classic::ink);
        setColour(juce::TableHeaderComponent::outlineColourId,juce::Colours::grey);

        setColour(juce::AlertWindow::backgroundColourId,classic::face);
        setColour(juce::AlertWindow::textColourId,classic::ink);

        setColour(juce::TooltipWindow::backgroundColourId,juce::Colour(0xffffffe1));
        setColour(juce::TooltipWindow::textColourId,classic::ink);
    }

    void RetroLook::drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour&,bool,bool down) {
        g.fillAll(b.getToggleState()?juce::Colour(0xffdedede):classic::face);
        classic::bevel(g,b.getLocalBounds(),down||b.getToggleState());

        if(b.hasKeyboardFocus(true)){
            g.setColour(classic::ink);
            g.drawRect(b.getLocalBounds().reduced(4));
        }
    }

    void RetroLook::drawComboBox(juce::Graphics& g,int w,int h,bool,int,int,int,int,juce::ComboBox& b) {
        g.fillAll(b.isEnabled()?classic::paper:classic::face);
        classic::bevel(g,{
            0,0,w,h
        },true);
        g.setColour(classic::face);
        g.fillRect(w-20,2,18,h-4);
        classic::bevel(g,{
            w-20,2,18,h-4
        });

        juce::Path p;
        p.addTriangle(float(w-15),float(h/2-2),float(w-7),float(h/2-2),float(w-11),float(h/2+2));
        g.setColour(b.isEnabled()?classic::ink:juce::Colours::grey);
        g.fillPath(p);
    }

    void RetroLook::drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float,float,juce::Slider::SliderStyle,juce::Slider&) {
        const int mid=y+h/2;
        g.setColour(juce::Colours::grey);
        g.fillRect(x,mid-2,w,4);
        g.setColour(classic::paper);
        g.drawHorizontalLine(mid+2,float(x),float(x+w));

        juce::Rectangle<int> thumb(int(pos)-5,mid-9,11,19);
        g.setColour(classic::face);
        g.fillRect(thumb);
        classic::bevel(g,thumb);
        g.setColour(juce::Colours::grey);
        g.drawVerticalLine(int(pos),float(mid-5),float(mid+5));
    }

    void RetroLook::drawToggleButton(juce::Graphics& g,juce::ToggleButton& b,bool,bool) {
        const auto r=juce::Rectangle<int>(2,(b.getHeight()-13)/2,13,13);
        g.setColour(b.isEnabled()?classic::paper:classic::face);
        g.fillRect(r);
        classic::bevel(g,r,true);

        if(b.getToggleState()){
            g.setColour(b.isEnabled()?classic::ink:juce::Colours::grey);
            juce::Path p;
            p.startNewSubPath(float(r.getX()+3),float(r.getY()+6));
            p.lineTo(float(r.getX()+6),float(r.getBottom()-3));
            p.lineTo(float(r.getRight()-3),float(r.getY()+3));
            g.strokePath(p,juce::PathStrokeType(1.7f));
        }

        g.setColour(b.isEnabled()?classic::ink:juce::Colours::grey);
        g.setFont(12.0f);
        g.drawText(b.getButtonText(),21,0,b.getWidth()-23,b.getHeight(),juce::Justification::centredLeft);

        if(b.hasKeyboardFocus(true))g.drawRect(19,2,b.getWidth()-20,b.getHeight()-4);
    }

    juce::Button* RetroLook::createDocumentWindowButton(int type) {
        auto* button=new juce::TextButton(type==juce::DocumentWindow::closeButton?"x":type==juce::DocumentWindow::minimiseButton?"_":"[]");

        button->setWantsKeyboardFocus(false);
        return button;
    }

    void RetroLook::drawDocumentWindowTitleBar(juce::DocumentWindow& w,juce::Graphics& g,int width,int height,int title_x,int title_width,const juce::Image*,bool active) {
        g.fillAll(active?classic::navy:juce::Colours::grey);
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(13.0f,juce::Font::bold));
        const int x=w.getName()=="RiffCalc 2000 Pro"?std::max(74,title_x+5):title_x+5;
        g.drawText(w.getName(),x,0,std::min(title_width,width-x-65),height,juce::Justification::centredLeft);
    }

    void ToolButton::paintButton(juce::Graphics& g,bool over,bool down) {
        getLookAndFeel().drawButtonBackground(g,*this,classic::face,over,down);
        g.saveState();
        g.addTransform(juce::AffineTransform::translation(8.0f,float((getHeight()-16)/2)));

        g.setColour(isEnabled()?classic::navy:juce::Colours::grey);

        switch(icon_){
            case Open:g.drawRect(0,5,16,11);
            g.drawRect(1,2,7,4);
            g.drawLine(1,11,15,11,2);
            break;

            case Save:g.drawRect(0,0,16,16);
            g.fillRect(3,0,9,6);
            g.drawRect(3,9,10,6);
            break;

            case Export:g.drawRect(0,5,10,11);
            g.drawLine(5,3,16,3,2);
            g.drawLine(12,0,16,3,2);
            g.drawLine(12,6,16,3,2);
            break;

            case Previous:case Next:{
                const bool next=icon_==Next;
                juce::Path p;
                p.addTriangle(next?15.0f:0.0f,8,next?4.0f:11.0f,1,next?4.0f:11.0f,15);
                g.fillPath(p);
                break;
            }

            case New:g.drawRect(2,0,12,16);
            g.drawLine(5,5,11,5);
            g.drawLine(5,8,11,8);
            g.drawLine(5,11,9,11);
            break;

            case Mutate:g.drawRect(1,1,14,14);
            for(auto xy:{
                juce::Point<int>(4,4),{
                    10,4
                },{
                    7,7
                },{
                    4,10
                },{
                    10,10
                }
            })g.fillRect(xy.x,xy.y,2,2);
            break;

            case Help:g.setFont(juce::Font(18.0f,juce::Font::bold));
            g.drawText("?",0,-2,16,20,juce::Justification::centred);
            break;
        }

        g.restoreState();
        g.setColour(isEnabled()?classic::ink:juce::Colours::grey);
        g.setFont(12.0f);
        g.drawText(getButtonText(),30,0,getWidth()-33,getHeight(),juce::Justification::centredLeft);
    }
}
