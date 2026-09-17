#include "flowdaw/AudioEngine.hpp"
#include "flowdaw/MusicalTime.hpp"
#include "flowdaw/NativeDrums.hpp"
#include "flowdaw/Serialization.hpp"
#include "flowdaw/Undo.hpp"
#include "flowdaw/Wav.hpp"
#include "flowdaw/Waveform.hpp"
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace flowdaw;
namespace {
struct Rect { int x,y,w,h; bool contains(int px,int py) const { return px>=x&&py>=y&&px<x+w&&py<y+h; } };
constexpr int W=1280,H=820;
constexpr int browserW=220,rulerY=116,trackY=150,trackH=104,seqY=310,editorY=548;
constexpr double pxPerBeat=56.0;

unsigned long rgb(Display*,int r,int g,int b){ return (static_cast<unsigned long>(r)<<16)|(static_cast<unsigned long>(g)<<8)|static_cast<unsigned long>(b); }
void fill(Display* d,Window w,GC gc,Rect r,unsigned long c){ XSetForeground(d,gc,c);XFillRectangle(d,w,gc,r.x,r.y,r.w,r.h); }
void line(Display* d,Window w,GC gc,int x1,int y1,int x2,int y2,unsigned long c){ XSetForeground(d,gc,c);XDrawLine(d,w,gc,x1,y1,x2,y2); }
void outline(Display* d,Window w,GC gc,Rect r,unsigned long c){ XSetForeground(d,gc,c);XDrawRectangle(d,w,gc,r.x,r.y,static_cast<unsigned int>(std::max(0,r.w-1)),static_cast<unsigned int>(std::max(0,r.h-1))); }
void text(Display* d,Window w,GC gc,int x,int y,const std::string& s,unsigned long c){ XSetForeground(d,gc,c);XDrawString(d,w,gc,x,y,s.c_str(),static_cast<int>(s.size())); }
int pct(float value){ return static_cast<int>(std::llround(std::clamp(value,0.0f,1.0f)*100.0f)); }

struct App {
    Display* d=nullptr; Window win{}; GC gc{};
    Project project; AudioEngine engine; UndoStack undo;
    std::filesystem::path projectPath="Untitled.flow";
    bool running=true,dragging=false; int dragAnchorX=0; Tick dragStartTick=0; Project dragBefore;
    enum class InputMode{Idle,ImportPath,OpenPath} inputMode=InputMode::Idle; std::string input;
    std::string status="Ready"; std::vector<PeakPair> waveform;
    int selectedLane=0,selectedStep=0;

    Rect play{250,28,66,34},pause{322,28,66,34},stop{394,28,66,34},bpmMinus{496,28,28,34},bpmPlus{602,28,28,34},importBtn{25,92,170,34},saveBtn{25,134,82,30},openBtn{113,134,82,30};
    Rect swingMinus{1010,330,26,26},swingPlus{1162,330,26,26},humanMinus{1010,370,26,26},humanPlus{1162,370,26,26};
    Rect velMinus{425,608,28,28},velPlus{460,608,28,28},probMinus{770,608,28,28},probPlus{805,608,28,28},microMinus{1000,608,28,28},microPlus{1035,608,28,28};

    App(){
        project.name="Untitled Beat"; project.transport.bpm=90.0;
        Track audio; audio.name="Audio 1"; project.tracks.push_back(audio);
        std::vector<std::pair<std::string,std::string>> kit={{"Kick","kick"},{"Snare","snare"},{"Hat","hat"}};
        Pattern pat; pat.name="Pattern 1"; pat.stepCount=16; pat.stepsPerBeat=4;
        for(auto const& [name,key]:kit){
            SampleAsset s; s.name="FLOW "+name;s.nativeKey=key;s.audio=std::make_shared<AudioBuffer>(makeNativeDrum(key,project.sampleRate));auto sid=s.id;project.samples.push_back(s);
            DrumLane lane;lane.name=name;lane.sampleId=sid;lane.steps.resize(16);pat.lanes.push_back(lane);
        }
        pat.lanes[0].steps[0].active=true;pat.lanes[0].steps[8].active=true;
        pat.lanes[1].steps[4].active=true;pat.lanes[1].steps[12].active=true;
        for(int i=0;i<16;i+=2){pat.lanes[2].steps[static_cast<std::size_t>(i)].active=true;pat.lanes[2].steps[static_cast<std::size_t>(i)].velocity=(i%4==0)?0.72f:0.54f;}
        const auto pid=pat.id;project.patterns.push_back(pat);Track drums;drums.name="Drums";PatternPlacement plc;plc.patternId=pid;plc.repeats=8;drums.patternClips.push_back(plc);project.tracks.push_back(drums);
    }

    void publish(){ engine.publish(project); }
    Clip* clip(){ return project.tracks.empty()||project.tracks[0].clips.empty()?nullptr:&project.tracks[0].clips[0]; }
    const SampleAsset* sampleForClip() const { if(project.tracks.empty()||project.tracks[0].clips.empty())return nullptr;return project.findSample(project.tracks[0].clips[0].sampleId); }
    Pattern* pattern(){ return project.patterns.empty()?nullptr:&project.patterns[0]; }
    const Pattern* pattern() const { return project.patterns.empty()?nullptr:&project.patterns[0]; }
    StepEvent* selectedEvent(){
        auto* p=pattern(); if(!p||selectedLane<0||selectedLane>=static_cast<int>(p->lanes.size())) return nullptr;
        auto& steps=p->lanes[static_cast<std::size_t>(selectedLane)].steps; if(selectedStep<0||selectedStep>=static_cast<int>(steps.size())) return nullptr;
        return &steps[static_cast<std::size_t>(selectedStep)];
    }
    int tickToX(Tick t) const { return browserW+30+static_cast<int>((static_cast<double>(t)/kPPQ)*pxPerBeat); }
    Rect clipRect(){auto*c=clip();if(!c)return{0,0,0,0};int x=tickToX(c->startTick);int ww=std::max(40,tickToX(c->startTick+c->lengthTicks)-x);return{x,trackY+18,ww,trackH-36};}
    Rect stepRect(int lane,int step) const { constexpr int sx=366,sw=33,gap=5,row=44;return{sx+step*(sw+gap),seqY+50+lane*row,sw,28}; }
    Rect laneMuteRect(int lane) const { return {302,seqY+50+lane*44,24,28}; }
    Rect laneSoloRect(int lane) const { return {330,seqY+50+lane*44,24,28}; }

    void commitChange(Project before,const std::string& name){ undo.commit(std::move(before),project,name); publish(); }
    void importWav(const std::filesystem::path& path){
        try{auto audio=std::make_shared<AudioBuffer>(WavFile::read(path));Project before=project;SampleAsset s;s.path=std::filesystem::absolute(path);s.name=path.filename().string();s.audio=audio;auto sid=s.id;project.samples.push_back(s);if(project.tracks.empty())project.tracks.push_back(Track{});project.tracks[0].name="Sample";project.tracks[0].clips.clear();Clip c;c.sampleId=sid;c.sourceLength=audio->frames();c.lengthTicks=MusicalTime::samplesToTicks(audio->frames(),project.transport.bpm,audio->sampleRate);project.tracks[0].clips.push_back(c);waveform=buildWaveform(*audio,600);commitChange(std::move(before),"Import WAV");status="Imported "+path.filename().string();}catch(const std::exception&e){status=std::string("Import failed: ")+e.what();}
    }
    void save(){try{ProjectSerializer::save(project,projectPath);status="Saved "+projectPath.string();}catch(const std::exception&e){status=std::string("Save failed: ")+e.what();}}
    void openProject(const std::filesystem::path& p){try{project=ProjectSerializer::load(p);projectPath=p;waveform.clear();if(auto*s=sampleForClip();s&&s->audio)waveform=buildWaveform(*s->audio,600);selectedLane=0;selectedStep=0;publish();status="Opened "+p.filename().string();}catch(const std::exception&e){status=std::string("Open failed: ")+e.what();}}
    void changeBpm(double delta){Project before=project;project.transport.bpm=std::clamp(project.transport.bpm+delta,20.0,300.0);commitChange(std::move(before),"Change BPM");}
    void toggleStep(int lane,int step){
        auto*pat=pattern();if(!pat||lane<0||lane>=static_cast<int>(pat->lanes.size()))return;auto&steps=pat->lanes[static_cast<std::size_t>(lane)].steps;if(step<0||step>=static_cast<int>(steps.size()))return;
        selectedLane=lane;selectedStep=step;Project before=project;steps[static_cast<std::size_t>(step)].active=!steps[static_cast<std::size_t>(step)].active;commitChange(std::move(before),"Toggle drum step");status=pat->lanes[static_cast<std::size_t>(lane)].name+" step "+std::to_string(step+1)+(steps[static_cast<std::size_t>(step)].active?" on":" off");
    }
    void toggleLaneMute(int lane){auto*p=pattern();if(!p||lane<0||lane>=static_cast<int>(p->lanes.size()))return;Project before=project;p->lanes[static_cast<std::size_t>(lane)].mute=!p->lanes[static_cast<std::size_t>(lane)].mute;commitChange(std::move(before),"Toggle lane mute");status=p->lanes[static_cast<std::size_t>(lane)].name+(p->lanes[static_cast<std::size_t>(lane)].mute?" muted":" unmuted");}
    void toggleLaneSolo(int lane){auto*p=pattern();if(!p||lane<0||lane>=static_cast<int>(p->lanes.size()))return;Project before=project;p->lanes[static_cast<std::size_t>(lane)].solo=!p->lanes[static_cast<std::size_t>(lane)].solo;commitChange(std::move(before),"Toggle lane solo");status=p->lanes[static_cast<std::size_t>(lane)].name+(p->lanes[static_cast<std::size_t>(lane)].solo?" solo":" unsolo");}
    void adjustSwing(float delta){auto*p=pattern();if(!p)return;Project before=project;p->swing=std::clamp(p->swing+delta,0.0f,1.0f);commitChange(std::move(before),"Adjust swing");status="Swing "+std::to_string(pct(p->swing))+"%";}
    void adjustHumanize(float delta){auto*p=pattern();if(!p)return;Project before=project;p->humanize=std::clamp(p->humanize+delta,0.0f,1.0f);commitChange(std::move(before),"Adjust humanize");status="Humanize "+std::to_string(pct(p->humanize))+"%";}
    void adjustVelocity(float delta){auto*e=selectedEvent();if(!e)return;Project before=project;e->velocity=std::clamp(e->velocity+delta,0.0f,1.0f);commitChange(std::move(before),"Adjust step velocity");status="Velocity "+std::to_string(pct(e->velocity))+"%";}
    void adjustProbability(float delta){auto*e=selectedEvent();if(!e)return;Project before=project;e->probability=std::clamp(e->probability+delta,0.0f,1.0f);commitChange(std::move(before),"Adjust step probability");status="Probability "+std::to_string(pct(e->probability))+"%";}
    void adjustMicro(int delta){auto*e=selectedEvent();if(!e)return;Project before=project;e->microTicks=std::clamp(e->microTicks+delta,-240,240);commitChange(std::move(before),"Adjust step microtiming");status="Microtiming "+std::to_string(e->microTicks)+" ticks";}

    void drawValueBar(Rect r,float value,unsigned long base,unsigned long accent){fill(d,win,gc,r,base);Rect v=r;v.w=static_cast<int>(std::llround(r.w*std::clamp(value,0.0f,1.0f)));fill(d,win,gc,v,accent);}
    void draw(){
        const auto bg=rgb(d,18,19,22),panel=rgb(d,27,29,34),panel2=rgb(d,35,38,44),grid=rgb(d,54,58,66),txt=rgb(d,225,228,234),muted=rgb(d,145,151,163),clipC=rgb(d,94,78,220),playC=rgb(d,54,198,128),stepOn=rgb(d,239,154,64),stepOff=rgb(d,48,51,58),selectC=rgb(d,112,198,255),warn=rgb(d,222,90,90);
        fill(d,win,gc,{0,0,W,H},bg);fill(d,win,gc,{0,0,W,82},panel);fill(d,win,gc,{0,82,browserW,H-82},panel);text(d,win,gc,24,38,"FLOWDAW",txt);text(d,win,gc,24,58,"Phase 1 - Groove Engine",muted);
        auto button=[&](Rect r,const std::string&s,unsigned long c){fill(d,win,gc,r,c);text(d,win,gc,r.x+10,r.y+21,s,txt);};
        button(play,"PLAY",engine.isPlaying()?playC:panel2);button(pause,"PAUSE",panel2);button(stop,"STOP",panel2);button(bpmMinus,"-",panel2);button(bpmPlus,"+",panel2);text(d,win,gc,532,49,"BPM "+std::to_string(static_cast<int>(std::llround(project.transport.bpm))),txt);button(importBtn,"IMPORT WAV",panel2);button(saveBtn,"SAVE",panel2);button(openBtn,"OPEN",panel2);
        text(d,win,gc,25,198,"BROWSER",muted);text(d,win,gc,25,228,"Sounds",txt);text(d,win,gc,25,252,"Drums",txt);text(d,win,gc,25,276,"Samples",txt);text(d,win,gc,25,300,"Instruments",txt);text(d,win,gc,25,324,"Projects",txt);text(d,win,gc,25,382,"Shortcuts",muted);text(d,win,gc,25,408,"Space  Play/Pause",txt);text(d,win,gc,25,430,"Ctrl+S Save",txt);text(d,win,gc,25,452,"Ctrl+Z Undo",txt);text(d,win,gc,25,474,"Ctrl+Shift+Z Redo",txt);
        fill(d,win,gc,{browserW,82,W-browserW,H-82},bg);text(d,win,gc,browserW+24,104,"ARRANGEMENT",muted);for(int beat=0;beat<=64;++beat){int x=tickToX(static_cast<Tick>(beat)*kPPQ);bool bar=beat%4==0;line(d,win,gc,x,rulerY,x,trackY+trackH,bar?grid:rgb(d,38,41,47));if(bar&&beat<64)text(d,win,gc,x+4,rulerY-6,std::to_string(beat/4+1),muted);}fill(d,win,gc,{browserW+8,trackY-12,W-browserW-18,trackH},panel);text(d,win,gc,browserW+18,trackY+6,"AUDIO 1",muted);
        if(auto*c=clip()){Rect r=clipRect();fill(d,win,gc,r,clipC);text(d,win,gc,r.x+10,r.y+18,sampleForClip()?sampleForClip()->name:"Audio",txt);if(!waveform.empty()){const int mid=r.y+r.h/2;const std::size_t count=std::min<std::size_t>(waveform.size(),static_cast<std::size_t>(std::max(1,r.w-12)));for(std::size_t i=0;i<count;++i){auto idx=i*waveform.size()/count;int x=r.x+6+static_cast<int>(i);int y1=mid-static_cast<int>(waveform[idx].second*(r.h/2-24));int y2=mid-static_cast<int>(waveform[idx].first*(r.h/2-24));line(d,win,gc,x,y1,x,y2,txt);}}(void)c;}else text(d,win,gc,browserW+42,trackY+62,"Import a WAV to add a sample clip",muted);

        fill(d,win,gc,{browserW+18,seqY-10,W-browserW-38,218},panel);text(d,win,gc,browserW+34,seqY+18,"DRUM PATTERN 1 - 16 STEPS",muted);
        if(auto*pat=pattern()){
            text(d,win,gc,1005,seqY+17,"GROOVE",muted);button(swingMinus,"-",panel2);button(swingPlus,"+",panel2);text(d,win,gc,1044,seqY+49,"Swing "+std::to_string(pct(pat->swing))+"%",txt);button(humanMinus,"-",panel2);button(humanPlus,"+",panel2);text(d,win,gc,1044,seqY+89,"Human "+std::to_string(pct(pat->humanize))+"%",txt);
            for(std::size_t li=0;li<pat->lanes.size();++li){
                const int lane=static_cast<int>(li);text(d,win,gc,browserW+34,seqY+69+lane*44,pat->lanes[li].name,txt);button(laneMuteRect(lane),"M",pat->lanes[li].mute?warn:panel2);button(laneSoloRect(lane),"S",pat->lanes[li].solo?playC:panel2);
                for(int st=0;st<std::min(16,static_cast<int>(pat->lanes[li].steps.size()));++st){
                    auto r=stepRect(lane,st);auto const& ev=pat->lanes[li].steps[static_cast<std::size_t>(st)];fill(d,win,gc,r,ev.active?stepOn:stepOff);if(ev.active){Rect vr{r.x+2,r.y+r.h-5,std::max(1,static_cast<int>((r.w-4)*std::clamp(ev.velocity,0.0f,1.0f))),3};fill(d,win,gc,vr,txt);}if(st%4==0)line(d,win,gc,r.x-4,r.y-4,r.x-4,r.y+r.h+4,grid);if(lane==selectedLane&&st==selectedStep)outline(d,win,gc,{r.x-2,r.y-2,r.w+4,r.h+4},selectC);
                }
            }
        }

        fill(d,win,gc,{browserW+18,editorY,W-browserW-38,126},panel);text(d,win,gc,browserW+34,editorY+25,"STEP EDITOR",muted);
        if(auto*ev=selectedEvent()){
            auto*pat=pattern();const std::string laneName=(pat&&selectedLane<static_cast<int>(pat->lanes.size()))?pat->lanes[static_cast<std::size_t>(selectedLane)].name:"Lane";text(d,win,gc,browserW+130,editorY+25,laneName+" / Step "+std::to_string(selectedStep+1),txt);
            text(d,win,gc,270,editorY+71,"Velocity",txt);drawValueBar({340,editorY+55,70,18},ev->velocity,panel2,stepOn);button(velMinus,"-",panel2);button(velPlus,"+",panel2);text(d,win,gc,500,editorY+75,std::to_string(pct(ev->velocity))+"%",txt);
            text(d,win,gc,590,editorY+71,"Probability",txt);drawValueBar({685,editorY+55,70,18},ev->probability,panel2,selectC);button(probMinus,"-",panel2);button(probPlus,"+",panel2);text(d,win,gc,845,editorY+75,std::to_string(pct(ev->probability))+"%",txt);
            text(d,win,gc,930,editorY+71,"Micro",txt);button(microMinus,"-",panel2);button(microPlus,"+",panel2);text(d,win,gc,1080,editorY+75,std::to_string(ev->microTicks)+" ticks",txt);
            text(d,win,gc,270,editorY+105,"Click a step to toggle + select. Edit values here; every change supports Undo.",muted);
        }

        Tick ph=MusicalTime::samplesToTicks(engine.playheadSamples(),project.transport.bpm,engine.sampleRate());int px=tickToX(ph);line(d,win,gc,px,rulerY,px,trackY+trackH,playC);fill(d,win,gc,{0,H-52,W,52},panel);text(d,win,gc,20,H-21,status,muted);auto mp=MusicalTime::fromTicks(ph);text(d,win,gc,W-170,H-21,"Bar "+std::to_string(mp.bar)+" : "+std::to_string(mp.beat),txt);
        if(inputMode!=InputMode::Idle){fill(d,win,gc,{300,520,680,110},panel2);text(d,win,gc,322,550,inputMode==InputMode::ImportPath?"Type WAV path, then Enter":"Type project path, then Enter",txt);fill(d,win,gc,{322,570,636,36},bg);text(d,win,gc,334,594,input+"_",txt);}XFlush(d);
    }

    void handleKey(XKeyEvent& k){
        KeySym ks=XLookupKeysym(&k,0);bool ctrl=(k.state&ControlMask)!=0,shift=(k.state&ShiftMask)!=0;
        if(inputMode!=InputMode::Idle){if(ks==XK_Escape){inputMode=InputMode::Idle;input.clear();return;}if(ks==XK_Return){auto p=input;auto mode=inputMode;inputMode=InputMode::Idle;input.clear();if(mode==InputMode::ImportPath)importWav(p);else openProject(p);return;}if(ks==XK_BackSpace){if(!input.empty())input.pop_back();return;}char buf[32]{};KeySym dummy{};int n=XLookupString(&k,buf,sizeof(buf),&dummy,nullptr);if(n>0)input.append(buf,buf+n);return;}
        if(ks==XK_space){engine.isPlaying()?engine.pause():engine.play();status=engine.isPlaying()?"Playing":"Paused";}else if(ks==XK_Escape){engine.stop();status="Stopped";}else if(ctrl&&ks==XK_s)save();else if(ctrl&&ks==XK_z&&!shift){if(undo.undo(project)){publish();status="Undo";}}else if(ctrl&&shift&&(ks==XK_Z||ks==XK_z)){if(undo.redo(project)){publish();status="Redo";}}else if(ks==XK_i){inputMode=InputMode::ImportPath;input.clear();}
    }
    void handleButton(XButtonEvent& e){
        if(e.button!=1) return;
        if(play.contains(e.x,e.y)){engine.play();status="Playing";return;}if(pause.contains(e.x,e.y)){engine.pause();status="Paused";return;}if(stop.contains(e.x,e.y)){engine.stop();status="Stopped";return;}if(bpmMinus.contains(e.x,e.y)){changeBpm(-1);return;}if(bpmPlus.contains(e.x,e.y)){changeBpm(1);return;}if(importBtn.contains(e.x,e.y)){inputMode=InputMode::ImportPath;input.clear();return;}if(saveBtn.contains(e.x,e.y)){save();return;}if(openBtn.contains(e.x,e.y)){inputMode=InputMode::OpenPath;input.clear();return;}
        if(swingMinus.contains(e.x,e.y)){adjustSwing(-0.05f);return;}if(swingPlus.contains(e.x,e.y)){adjustSwing(0.05f);return;}if(humanMinus.contains(e.x,e.y)){adjustHumanize(-0.05f);return;}if(humanPlus.contains(e.x,e.y)){adjustHumanize(0.05f);return;}if(velMinus.contains(e.x,e.y)){adjustVelocity(-0.05f);return;}if(velPlus.contains(e.x,e.y)){adjustVelocity(0.05f);return;}if(probMinus.contains(e.x,e.y)){adjustProbability(-0.05f);return;}if(probPlus.contains(e.x,e.y)){adjustProbability(0.05f);return;}if(microMinus.contains(e.x,e.y)){adjustMicro(-10);return;}if(microPlus.contains(e.x,e.y)){adjustMicro(10);return;}
        if(auto*pat=pattern()){for(int li=0;li<std::min(3,static_cast<int>(pat->lanes.size()));++li){if(laneMuteRect(li).contains(e.x,e.y)){toggleLaneMute(li);return;}if(laneSoloRect(li).contains(e.x,e.y)){toggleLaneSolo(li);return;}for(int st=0;st<16;++st)if(stepRect(li,st).contains(e.x,e.y)){toggleStep(li,st);return;}}}
        if(auto*c=clip();c&&clipRect().contains(e.x,e.y)){dragging=true;dragAnchorX=e.x;dragStartTick=c->startTick;dragBefore=project;}
    }
    void handleMotion(XMotionEvent& e){if(!dragging)return;if(auto*c=clip()){Tick dt=static_cast<Tick>(std::llround((e.x-dragAnchorX)/pxPerBeat*kPPQ));Tick raw=std::max<Tick>(0,dragStartTick+dt);Tick snap=kPPQ/4;c->startTick=(raw+snap/2)/snap*snap;publish();}}
    void handleRelease(XButtonEvent&){if(dragging){dragging=false;undo.commit(dragBefore,project,"Move clip");status="Moved clip";}}
    int run(int argc,char**argv){
        d=XOpenDisplay(nullptr);if(!d){std::cerr<<"Cannot open X display\n";return 2;}int scr=DefaultScreen(d);win=XCreateSimpleWindow(d,RootWindow(d,scr),40,40,W,H,0,0,rgb(d,18,19,22));XStoreName(d,win,"FLOWDAW - Phase 1 Groove Engine");gc=XCreateGC(d,win,0,nullptr);XSelectInput(d,win,ExposureMask|KeyPressMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask|StructureNotifyMask);XMapWindow(d,win);const bool audioOk=engine.open(project.sampleRate,256);status=audioOk?"Audio device ready":"No physical audio device here; offline engine active";publish();if(argc>1){std::filesystem::path p=argv[1];if(p.extension()==".flow")openProject(p);else importWav(p);}using namespace std::chrono_literals;while(running){while(XPending(d)){XEvent e;XNextEvent(d,&e);if(e.type==Expose)draw();else if(e.type==KeyPress)handleKey(e.xkey);else if(e.type==ButtonPress)handleButton(e.xbutton);else if(e.type==MotionNotify)handleMotion(e.xmotion);else if(e.type==ButtonRelease)handleRelease(e.xbutton);else if(e.type==DestroyNotify)running=false;}draw();std::this_thread::sleep_for(16ms);}engine.close();XFreeGC(d,gc);XDestroyWindow(d,win);XCloseDisplay(d);return 0;
    }
};
}
int main(int argc,char**argv){App a;return a.run(argc,argv);}
