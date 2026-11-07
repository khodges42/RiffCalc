// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
namespace packrat::app {
    namespace classic {
        inline const juce::Colour face{
            0xffc0c0c0
        }, paper{
            0xffffffff
        }, ink{
            0xff000000
        }, navy{
            0xff000080
        }, green{
            0xff286344
        };

        void bevel(juce::Graphics&,juce::Rectangle<int>,bool down=false);

        void group(juce::Graphics&,juce::Rectangle<int>,const juce::String&);

        void logo(juce::Graphics&,juce::Rectangle<int>);
    }

    class RetroLook final : public juce::LookAndFeel_V4 {
        public:
        RetroLook();

        void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;

        void drawComboBox(juce::Graphics&,int,int,bool,int,int,int,int,juce::ComboBox&) override;

        void drawLinearSlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider::SliderStyle,juce::Slider&) override;

        void drawToggleButton(juce::Graphics&,juce::ToggleButton&,bool,bool) override;

        juce::Button* createDocumentWindowButton(int) override;

        void drawDocumentWindowTitleBar(juce::DocumentWindow&,juce::Graphics&,int,int,int,int,const juce::Image*,bool) override;
    };

    class ToolButton final : public juce::TextButton {
        public:
        enum Icon {
            Open, Save, Export, Previous, Next, New, Mutate, Help
        };

        ToolButton(const juce::String& name,Icon icon):TextButton(name),icon_(icon){
        }

        void paintButton(juce::Graphics&,bool,bool) override;

        private: Icon icon_;
    };
}
