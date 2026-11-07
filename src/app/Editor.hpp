// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "Processor.hpp"
#include "ClassicLook.hpp"
#include "About.hpp"
#include "NoteSheet.hpp"
namespace packrat::app {
    class PianoRoll final : public juce::Component {
        Idea idea_;
        Clip clip_;
        float position_=0;

        public:
        void set(const Idea&,Part);

        void position(float p){
            position_=p;
            repaint();
        }

        void paint(juce::Graphics&) override;
    };

    class DragButton final : public juce::TextButton {
        bool dragging_=false;

        public:
        std::function<void()> drag;

        DragButton():TextButton("Drag MIDI"){
        }

        void mouseDown(const juce::MouseEvent& e) override {
            dragging_=false;
            TextButton::mouseDown(e);
        }

        void mouseDrag(const juce::MouseEvent& e) override {
            if(!dragging_&&e.getDistanceFromDragStart()>5){
                dragging_=true;
                if(drag)drag();
            }
        }
    };

    class Editor final : public juce::AudioProcessorEditor,private juce::Timer,public juce::MenuBarModel {
        Processor& processor_;
        RetroLook look_;

        juce::TooltipWindow tips_{
            this,600
        };

        juce::MenuBarComponent menu_{
            this
        };

        ToolButton open_tool_{
            "Open",ToolButton::Open
        },save_tool_{
            "Save As",ToolButton::Save
        },export_tool_{
            "MIDI",ToolButton::Export
        },previous_tool_{
            "Previous",ToolButton::Previous
        },next_tool_{
            "Next",ToolButton::Next
        },generate_tool_{
            "Generate",ToolButton::New
        },mutate_tool_{
            "Mutate",ToolButton::Mutate
        },help_tool_{
            "Help",ToolButton::Help
        };

        juce::ComboBox root_,scale_,genre_,part_,sound_,bars_,arp_,count_;

        juce::Slider complexity_,bpm_,volume_,low_,high_,velocity_,humanize_;

        juce::TextEditor seed_;

        juce::TextButton generate_{
            "GENERATE"
        },mutate_{
            "MUTATE"
        },preview_{
            "PLAY"
        },stop_{
            "STOP"
        },export_{
            "EXPORT MIDI..."
        };

        DragButton drag_;

        juce::TextButton advanced_{
            "RIFF MACHINE +"
        },save_{
            "SAVE IDEA..."
        },load_{
            "OPEN IDEA..."
        },pack_{
            "EXPORT PACK..."
        },back_{
            "<"
        },forward_{
            ">"
        },apply_{
            "USE SEED"
        },piano_tab_{
            "Piano Roll"
        },events_tab_{
            "Event Sheet"
        };

        juce::ToggleButton harmony_{
            "Harmony"
        },voicing_{
            "Voicing"
        },rhythm_{
            "Rhythm"
        },host_{
            "Host tempo"
        },resolve_{
            "Resolve to tonic"
        },colour_{
            "Chromatic colour"
        };

        juce::Label status_,seed_label_,result_,context_status_,selection_status_;

        PianoRoll roll_;
        NoteSheet sheet_;

        std::vector<std::unique_ptr<juce::Label>> labels_;

        std::vector<Document> history_;
        std::size_t history_index_=0;

        std::unique_ptr<juce::FileChooser> chooser_;

        std::unique_ptr<ClassicDialog> dialog_;

        juce::Component::SafePointer<juce::DocumentWindow> themed_parent_;

        juce::File drag_directory_;

        unsigned revision_=0;

        bool expanded_=false,refreshing_=false,event_view_=false,demo_=false;

        void timerCallback() override;

        void refresh();

        void run(const std::function<void()>&);

        Config config() const;

        void generate(bool mutation,bool new_seed=true);

        void remember(Document);

        void select_part();

        void locks_changed();

        void choose_file(int action);

        void drag_midi();

        void export_pack();

        void label(juce::String,juce::Component&);

        void show_text(const juce::String&,const juce::String&);

        void view_events(bool);

        public:
        enum Command {
            Open=1,Save,Export,Pack,Previous,Next,New,Mutate,UseSeed,Play,Stop,Piano,Events,Advanced,QuickStart,About,License,Repository
        };

        explicit Editor(Processor&);
        ~Editor() override;

        void paint(juce::Graphics&) override;
        void resized() override;

        bool keyPressed(const juce::KeyPress&) override;

        juce::StringArray getMenuBarNames() override;

        juce::PopupMenu getMenuForIndex(int,const juce::String&) override;

        void menuItemSelected(int,int) override;

        void perform(Command);

        void show_about();

        void edit_velocity(std::size_t row,int velocity);
    };
}
