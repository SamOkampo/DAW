#include "flowdaw/SampleEditing.hpp"
#include <iostream>
#include <stdexcept>

using namespace flowdaw;
static void require(bool v,const char*m){if(!v)throw std::runtime_error(m);}
static SampleSlice slice(const char*name,SampleIndex a,SampleIndex b){SampleSlice s;s.name=name;s.startFrame=a;s.endFrame=b;return s;}

int main(){
    try{
        SampleAsset s;s.slices={slice("A",0,1000),slice("B",1000,2000),slice("C",2000,3000)};
        require(moveSliceBoundary(s,0,1200,100),"move boundary");
        require(s.slices[0].endFrame==1200&&s.slices[1].startFrame==1200,"moved slices stay contiguous");
        require(moveSliceBoundary(s,0,1990,100),"clamped boundary move");
        require(s.slices[0].endFrame==1900&&s.slices[1].startFrame==1900,"minimum slice length enforced");
        require(insertSliceBoundary(s,2400,100),"insert manual boundary");
        require(s.slices.size()==4&&s.slices[2].endFrame==2400&&s.slices[3].startFrame==2400,"insert creates contiguous slices");
        require(!insertSliceBoundary(s,2450,100),"reject boundary too close to edge");
        require(removeSliceBoundary(s,2),"remove manual boundary");
        require(s.slices.size()==3&&s.slices[2].startFrame==1900&&s.slices[2].endFrame==3000,"remove merges adjacent slices");
        std::cout<<"FLOWDAW SampleEditing tests: PASS\n";return 0;
    }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
