#pragma once

#include <algorithm>

namespace flowdaw::ui {

enum class ShellWidthMode { compact, wide };

struct ShellRect {
    int x=0,y=0,width=0,height=0;
    [[nodiscard]] constexpr bool valid() const noexcept { return width>=0&&height>=0; }
    [[nodiscard]] constexpr int right() const noexcept { return x+width; }
    [[nodiscard]] constexpr int bottom() const noexcept { return y+height; }
};

struct ShellLayoutMetrics {
    static constexpr int minimumWidth=1180;
    static constexpr int minimumHeight=720;
    static constexpr int wideBreakpoint=1380;
    static constexpr int inset=16;
    static constexpr int headerHeight=46;
    static constexpr int transportHeight=48;
    static constexpr int workspaceHeight=44;
    static constexpr int contextHeight=28;
    static constexpr int toolHeight=38;
    static constexpr int rackHeight=72;

    ShellWidthMode mode=ShellWidthMode::wide;
    ShellRect content{};
    ShellRect header{},transport{},workspace{},context{};
    ShellRect pluginTools{},instrumentTools{},rackTools{},sampleTools{},recordTools{};

    [[nodiscard]] static constexpr ShellLayoutMetrics calculate(int width,int height) noexcept {
        ShellLayoutMetrics m{};
        const int w=std::max(width,minimumWidth);
        const int h=std::max(height,minimumHeight);
        m.mode=w<wideBreakpoint?ShellWidthMode::compact:ShellWidthMode::wide;
        m.content={inset,inset,w-2*inset,h-2*inset};
        int y=inset;
        const int rowWidth=m.content.width;
        auto row=[&](int rowHeight) constexpr {
            ShellRect r{inset,y,rowWidth,rowHeight};
            y+=rowHeight;
            return r;
        };
        m.header=row(headerHeight);
        m.transport=row(transportHeight);
        m.workspace=row(workspaceHeight);
        m.context=row(contextHeight);
        y+=8;
        m.pluginTools=row(toolHeight);
        m.instrumentTools=row(toolHeight);
        m.rackTools=row(rackHeight);
        m.sampleTools=row(toolHeight);
        m.recordTools=row(toolHeight);
        return m;
    }

    [[nodiscard]] constexpr bool contains(const ShellRect&r) const noexcept {
        return r.valid()&&r.x>=content.x&&r.y>=content.y&&r.right()<=content.right()&&r.bottom()<=content.bottom();
    }
};

// The top-level desktop window may shrink below the production canvas size.
// A Viewport keeps every control reachable without shrinking text or clipping
// the editor on laptops, display scaling and small remote desktop sessions.
struct DesktopCanvasMetrics {
    static constexpr int minimumWindowWidth=720;
    static constexpr int minimumWindowHeight=520;
    static constexpr int minimumCanvasWidth=ShellLayoutMetrics::minimumWidth;
    // Phase 13/14 introduced more tool rows; the old 720px shell minimum
    // leaves less than the editor's required 240px of vertical space.
    static constexpr int minimumCanvasHeight=840;

    int canvasWidth=minimumCanvasWidth;
    int canvasHeight=minimumCanvasHeight;
    bool horizontalScroll=false;
    bool verticalScroll=false;

    [[nodiscard]] static constexpr DesktopCanvasMetrics calculate(int viewWidth,int viewHeight) noexcept {
        DesktopCanvasMetrics m{};
        const int visibleWidth=std::max(0,viewWidth);
        const int visibleHeight=std::max(0,viewHeight);
        m.canvasWidth=std::max(visibleWidth,minimumCanvasWidth);
        m.canvasHeight=std::max(visibleHeight,minimumCanvasHeight);
        m.horizontalScroll=m.canvasWidth>visibleWidth;
        m.verticalScroll=m.canvasHeight>visibleHeight;
        return m;
    }
};

struct PanelLayoutMetrics {
    static constexpr int browserMinWidth=220;
    static constexpr int browserMaxWidth=420;
    static constexpr int editorMinWidth=620;
    static constexpr int editorMinHeight=240;
    static constexpr int mixerMinHeight=140;
    static constexpr int mixerMaxHeight=260;
    static constexpr int utilityMaxHeight=220;
    static constexpr int gap=6;

    ShellRect browser{},editor{},mixer{},utility{};
    bool mixerVisible=false;
    bool utilityVisible=false;

    [[nodiscard]] static constexpr PanelLayoutMetrics calculate(
        ShellRect area,
        int preferredBrowserWidth=280,
        int preferredMixerHeight=176,
        int preferredUtilityHeight=180,
        bool mixerRequested=true,
        bool utilityRequested=true) noexcept {
        PanelLayoutMetrics m{};
        if(area.width<=0||area.height<=0)return m;

        const int maxBrowser=std::max(0,area.width-editorMinWidth-gap);
        const int browserWidth=maxBrowser>=browserMinWidth?std::clamp(preferredBrowserWidth,browserMinWidth,std::min(browserMaxWidth,maxBrowser)):maxBrowser;
        const int preferredMixer=std::clamp(preferredMixerHeight,mixerMinHeight,mixerMaxHeight);

        int mixerHeight=0;
        if(mixerRequested&&area.height>=editorMinHeight+gap+mixerMinHeight)
            mixerHeight=std::min(preferredMixer,area.height-editorMinHeight-gap);

        int utilityHeight=0;
        if(utilityRequested&&(!mixerRequested||mixerHeight>0)){
            const int usedByMixer=mixerHeight>0?mixerHeight+gap:0;
            const int maxUtility=std::max(0,area.height-editorMinHeight-usedByMixer-gap);
            utilityHeight=std::min(std::clamp(preferredUtilityHeight,0,utilityMaxHeight),maxUtility);
        }

        const int mixerGap=mixerHeight>0?gap:0;
        const int utilityGap=utilityHeight>0?gap:0;
        const int editorHeight=std::max(0,area.height-mixerHeight-utilityHeight-mixerGap-utilityGap);

        ShellRect editorBand{area.x,area.y,area.width,editorHeight};
        m.browser={editorBand.x,editorBand.y,browserWidth,editorBand.height};
        const int editorX=editorBand.x+browserWidth+(browserWidth>0?gap:0);
        m.editor={editorX,editorBand.y,std::max(0,editorBand.right()-editorX),editorBand.height};

        int y=editorBand.bottom();
        if(mixerHeight>0){
            y+=gap;
            m.mixer={area.x,y,area.width,mixerHeight};
            y+=mixerHeight;
            m.mixerVisible=true;
        }
        if(utilityHeight>0){
            y+=gap;
            m.utility={area.x,y,area.width,utilityHeight};
            m.utilityVisible=true;
        }
        return m;
    }
};

struct AutomationAssistLayoutMetrics {
    static constexpr int inset=6;
    static constexpr int top=24;
    static constexpr int gap=8;
    static constexpr int wideBreakpoint=900;

    ShellRect left{},middle{},right{},graph{};
    ShellWidthMode mode=ShellWidthMode::wide;

    [[nodiscard]] static constexpr AutomationAssistLayoutMetrics calculate(int width,int height) noexcept {
        AutomationAssistLayoutMetrics m{};
        const int usableWidth=std::max(0,width-2*inset);
        const int usableHeight=std::max(0,height-top-inset);
        m.mode=width<wideBreakpoint?ShellWidthMode::compact:ShellWidthMode::wide;
        const int gaps=2*gap;
        const int columns=std::max(0,usableWidth-gaps);
        int leftWidth=m.mode==ShellWidthMode::wide?columns*34/100:columns/3;
        int middleWidth=m.mode==ShellWidthMode::wide?columns*30/100:columns/3;
        const int rightWidth=std::max(0,columns-leftWidth-middleWidth);
        int x=inset;
        m.left={x,top,leftWidth,usableHeight};x+=leftWidth+gap;
        m.middle={x,top,middleWidth,usableHeight};x+=middleWidth+gap;
        m.right={x,top,rightWidth,usableHeight};
        const int graphHeight=std::min(88,std::max(0,usableHeight-154));
        m.graph={m.left.x,m.left.bottom()-graphHeight,m.left.width,graphHeight};
        return m;
    }
};

} // namespace flowdaw::ui
