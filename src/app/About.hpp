// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "ClassicLook.hpp"
namespace packrat::app {
    class AboutPanel final : public juce::Component {
        juce::TextButton ok_{
            "OK"
        },license_{
            "License..."
        },repository_{
            "Project Repository"
        };

        public:
        std::function<void()> onLicense;

        AboutPanel();

        void paint(juce::Graphics&) override;

        void resized() override;
    };

    class TextPanel final : public juce::Component {
        juce::TextEditor text_;

        juce::TextButton ok_{
            "OK"
        };

        public:
        explicit TextPanel(const juce::String& text);

        void resized() override;
    };

    class ClassicDialog final : public juce::DialogWindow {
        public:
        ClassicDialog(const juce::String& title,juce::Component* content,juce::LookAndFeel& look,juce::Component* parent);

        ~ClassicDialog() override {
            exitModalState(0);
            setLookAndFeel(nullptr);
        }

        void closeButtonPressed() override {
            exitModalState(0);
            setVisible(false);
        }
    };
}
