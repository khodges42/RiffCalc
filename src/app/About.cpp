// SPDX-License-Identifier: GPL-3.0-or-later
#include "About.hpp"
#include "BuildInfo.hpp"
namespace packrat::app {
    namespace {
        void dismiss(juce::Component& c) {
            if(auto* dialog=c.findParentComponentOfClass<juce::DialogWindow>()){
                dialog->exitModalState(0);
                dialog->setVisible(false);
            }
        }
    }

    AboutPanel::AboutPanel() {
        setSize(580,355);
        for(auto* b:{
            &ok_,&license_,&repository_
        })addAndMakeVisible(b);

        ok_.setComponentID("about-ok");
        ok_.onClick=[this]{
            dismiss(*this);
        };
        ok_.addShortcut(juce::KeyPress(juce::KeyPress::returnKey));

        license_.onClick=[this]{
            if(onLicense)onLicense();
        };

        repository_.onClick=[]{
            juce::URL("https://github.com/khodges42/RiffCalc").launchInDefaultBrowser();
        };
    }

    void AboutPanel::paint(juce::Graphics& g) {
        g.fillAll(classic::face);
        classic::logo(g,{
            23,27,68,68
        });
        g.setColour(classic::navy);
        g.setFont(juce::Font(24.0f,juce::Font::bold));
        g.drawText("RiffCalc 2000 Pro",110,20,440,32,juce::Justification::centredLeft);

        g.setColour(classic::ink);
        g.setFont(14.0f);
        g.drawText("(Workgroup Edition)",112,53,430,24,juce::Justification::centredLeft);

        g.setFont(12.0f);
        g.drawText("Advanced Musical Pattern Computation System",112,81,440,21,juce::Justification::centredLeft);

        g.setColour(juce::Colours::grey);
        g.drawHorizontalLine(119,22,558);
        g.setColour(juce::Colours::white);
        g.drawHorizontalLine(120,22,558);

        g.setColour(classic::ink);
        g.setFont(12.0f);

        g.drawText(juce::String("Version ")+riffcalc::version+"    Build "+riffcalc::revision,24,135,530,20,juce::Justification::centredLeft);

        g.drawText("A product of Voidmutt Enterprises",24,166,530,20,juce::Justification::centredLeft);

        g.drawText("Department of Pattern Accounting",24,188,530,20,juce::Justification::centredLeft);

        g.drawText("Original code and procedural recipes: VOIDMUTT, 2026.",24,226,530,20,juce::Justification::centredLeft);

        g.drawText("Free software. GPLv3; original source GPL-3.0-or-later.",24,248,530,20,juce::Justification::centredLeft);

        g.setFont(11.0f);
        g.drawText("JUCE and other contributors retain their original notices.",24,271,530,18,juce::Justification::centredLeft);
    }

    void AboutPanel::resized(){
        license_.setBounds(24,312,100,26);
        repository_.setBounds(134,312,150,26);
        ok_.setBounds(getWidth()-112,312,88,26);
    }

    TextPanel::TextPanel(const juce::String& text) {
        setSize(680,470);
        addAndMakeVisible(text_);
        addAndMakeVisible(ok_);
        text_.setMultiLine(true);
        text_.setReadOnly(true);
        text_.setScrollbarsShown(true);
        text_.setFont(juce::Font("Courier New",12.0f,juce::Font::plain));
        text_.setText(text,false);
        text_.setCaretPosition(0);
        ok_.onClick=[this]{
            dismiss(*this);
        };
        ok_.addShortcut(juce::KeyPress(juce::KeyPress::returnKey));
    }

    void TextPanel::resized(){
        text_.setBounds(10,10,getWidth()-20,getHeight()-55);
        ok_.setBounds(getWidth()-100,getHeight()-35,90,25);
    }

    ClassicDialog::ClassicDialog(const juce::String& title,juce::Component* content,juce::LookAndFeel& look,juce::Component* parent):DialogWindow(title,classic::face,true,true) {
        setLookAndFeel(&look);
        setUsingNativeTitleBar(false);
        setContentOwned(content,true);
        setResizable(false,false);
        centreAroundComponent(parent,getWidth(),getHeight());
        setVisible(true);
    }
}
