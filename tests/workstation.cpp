// SPDX-License-Identifier: GPL-3.0-or-later
#include "packrat/workstation.hpp"
#include "packrat/preview.hpp"
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <new>
#include <thread>
#include <cmath>
#include <chrono>
std::atomic<bool> counting{
    false
};
std::atomic<int> allocations{
    0
};

void* operator new(std::size_t size){
    if(counting)++allocations;
    if(void* p=std::malloc(size?size:1))return p;
    throw std::bad_alloc();

}

void operator delete(void* p)noexcept{
    std::free(p);

}
void operator delete(void* p,std::size_t)noexcept{
    std::free(p);

}

void* operator new[](std::size_t size){
    return ::operator new(size);

}
void operator delete[](void* p)noexcept{
    ::operator delete(p);

}

void operator delete[](void* p,std::size_t)noexcept{
    ::operator delete(p);

}

void check(bool ok,const char* text){
    if(!ok)throw std::runtime_error(text);

}

int main(int argc,char** argv){
    try {

        using namespace packrat;
        auto bank=bundled_profiles();
        Config c;
        c.seed=42;
        c.complexity=75;

        Document d;
        d.idea=make_idea(c,bank,"liquid-dnb");

        check(d==deserialize(serialize(d),bank),"Exact document round trip");

        check(d.idea==make_idea(c,bank,"liquid-dnb"),"Deterministic workstation");

        for(const auto& genre:bank.profiles)for(int scale=0;scale<10;++scale)for(int bars:{
            1,4,16
        }) {

            c.scale=static_cast<Scale>(scale);
            c.bars=bars;

            auto idea=make_idea(c,bank,genre.id,static_cast<ArpOrder>(scale%4));
            validate(idea);

            for(int i=1;i<4;++i){
                const auto& clip=idea.parts[static_cast<std::size_t>(i)];
                for(const auto& n:clip.notes)check(n.pitch>=(i==1?28:i==2?48:60)&&n.pitch<=(i==1?52:i==2?84:88),"Companion range");

            }

            auto copy=Document{

            };
            copy.idea=idea;
            check(copy==deserialize(serialize(copy),bank),"Round trip all configurations");

        }

        c=d.idea.harmony.config;
        c.seed=888;

        auto locked=make_idea(c,bank,"liquid-dnb",ArpOrder::Up,&d.idea,{
            true,true,true
        },true);

        check(locked.harmony.chords==d.idea.harmony.chords,"Voicing lock");

        for(std::size_t i=0;i<4;++i){
            check(locked.parts[i].notes.size()==d.idea.parts[i].notes.size(),"Rhythm note count");
            for(std::size_t n=0;n<locked.parts[i].notes.size();++n)check(locked.parts[i].notes[n].start==d.idea.parts[i].notes[n].start&&locked.parts[i].notes[n].duration==d.idea.parts[i].notes[n].duration,"Rhythm lock");

        }

        auto mutant=make_idea(c,bank,"liquid-dnb",ArpOrder::Up,&d.idea,{

        },true);
        int changes=0;

        for(std::size_t i=0;i<4;++i)if(mutant.harmony.chords[i].spec!=d.idea.harmony.chords[i].spec)++changes;
        check(changes<=1,"Recognizable harmony mutation");

        auto invalid=serialize(d);
        invalid.replace(invalid.find("packrat-workstation-1"),21,"packrat-workstation-9");
        bool rejected=false;
        try{
            (void)deserialize(invalid,bank);

        } catch(const std::exception&){
            rejected=true;

        }
        check(rejected,"Future schema rejected");

        auto clip=audition_clip(d.idea,Part::Arrangement);
        auto prepared=preview::prepare(clip);

        auto mailbox=std::make_unique<preview::Mailbox>();
        mailbox->publish(prepared);
        const auto* active=mailbox->consume();
        check(active&&active->count==prepared.count,"Mailbox publication");

        std::atomic<bool> done{
            false
        };
        std::thread producer([&]{
            for(int i=0;i<1000;++i){
                auto packet=prepared;
                packet.bpm=20+i%300;
                mailbox->publish(packet);

            }
            done=true;

        });

        do {
            if(const auto* p=mailbox->consume()){
                check(p->count==prepared.count,"Mailbox complete packet");
                for(std::size_t i=0;i<p->count;++i)check(p->events[i].pitch==prepared.events[i].pitch,"Mailbox ownership");

            }

        }
        while(!done);
        producer.join();

        for(double rate:{
            44100.0,48000.0,96000.0
        })for(int sound=0;sound<10;++sound) {

            preview::Player player;
            player.prepare(rate);
            player.set_clip(&prepared);
            double energy=0;

            counting=true;

            for(int sample=0;sample<static_cast<int>(rate*2);++sample){
                const auto value=player.sample(true,sound,174);
                check(std::isfinite(value)&&std::abs(value)<=1,"Finite bounded preview");
                energy+=value*value;

            }

            for(int sample=0;sample<static_cast<int>(rate*4);++sample)(void)player.sample(false,sound,174);

            const auto tail=player.sample(false,sound,174);
            counting=false;

            check(std::abs(tail)<.00001,"Stop terminates voices");
            check(energy>1,"Audible preview palette");

        }

        check(allocations==0,"No preview audio allocations");

        preview::Player a,b;
        a.prepare(48000);
        b.prepare(48000);
        a.set_clip(&prepared);
        b.set_clip(&prepared);

        for(int i=0;i<100000;++i)check(a.sample(true,3,132)==b.sample(true,3,132),"Deterministic sample scheduling");

        if(argc>1) {

            std::filesystem::path dir=argv[1];
            std::filesystem::create_directories(dir);

            auto out=dir/"arrangement.mid";
            auto bytes=arrangement_midi(d.idea.parts);

            if(std::filesystem::exists(out))std::filesystem::remove(out);
            write_new(out,bytes);

            auto batch=dir/("pack-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));

            export_pack(batch,d,bank,8);

            bool refused=false;
            try{
                export_pack(batch,d,bank,8);

            } catch(const std::exception&){
                refused=true;

            }
            check(refused,"Pack overwrite protection");

            check(std::filesystem::is_regular_file(batch/"008"/"melody.mid"),"Complete batch companions");

            auto session=serialize(d);
            auto f=dir/"example.packrat.json";
            if(std::filesystem::exists(f))std::filesystem::remove(f);
            write_new(f,{
                reinterpret_cast<const std::uint8_t*>(session.data()),session.size()
            });

        }

        std::cout<<"PASS: 360 genre/scale/length combinations, exact state, mutation locks, mailbox ownership, 30 audio renders, allocation-free preview\n";
        return 0;

    } catch(const std::exception& e){
        counting=false;
        std::cerr<<e.what()<<'\n';
        return 1;

    }

}
