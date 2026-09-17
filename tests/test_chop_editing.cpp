#include "flowdaw/ChopEditing.hpp"
#include <iostream>
#include <stdexcept>

using namespace flowdaw;
static void require(bool v,const char*m){if(!v)throw std::runtime_error(m);}

int main(){
    try{
        Pattern original;ChopEvent a;a.tick=100;ChopEvent b;b.tick=350;original.chopEvents={a,b};
        auto untouched=original;quantizeChopEvents(untouched,240,0.0f);require(untouched.chopEvents[0].tick==100&&untouched.chopEvents[1].tick==350,"0% quantize must preserve performance");
        auto half=original;quantizeChopEvents(half,240,0.5f);require(half.chopEvents[0].tick==50&&half.chopEvents[1].tick==295,"50% quantize should move halfway to grid");
        auto full=original;quantizeChopEvents(full,240,1.0f);require(full.chopEvents[0].tick==0&&full.chopEvents[1].tick==240,"100% quantize should snap to 1/16 grid");
        std::cout<<"FLOWDAW ChopEditing tests: PASS\n";return 0;
    }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
