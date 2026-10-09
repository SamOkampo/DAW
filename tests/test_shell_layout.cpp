#include "flowdaw/ShellLayout.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>

// Do not use FLOWDAW_CHECK(): Release builds define NDEBUG and silently skip it.
// Geometry regressions must fail CTest in both Debug and Release.
#define FLOWDAW_CHECK(expr) do { \
    if (!(expr)) { \
        std::fprintf(stderr, "FLOWDAW layout check failed at %s:%d: %s\\n", __FILE__, __LINE__, #expr); \
        std::abort(); \
    } \
} while (false)

using flowdaw::ui::AutomationAssistLayoutMetrics;
using flowdaw::ui::PanelLayoutMetrics;
using flowdaw::ui::ShellLayoutMetrics;
using flowdaw::ui::ShellRect;
using flowdaw::ui::ShellWidthMode;

static bool contains(const ShellRect&outer,const ShellRect&inner){
    return inner.valid()&&inner.x>=outer.x&&inner.y>=outer.y&&inner.right()<=outer.right()&&inner.bottom()<=outer.bottom();
}

static bool overlaps(const ShellRect&a,const ShellRect&b){
    if(a.width<=0||a.height<=0||b.width<=0||b.height<=0)return false;
    return a.x<b.right()&&a.right()>b.x&&a.y<b.bottom()&&a.bottom()>b.y;
}

static void assertPanelContract(const ShellRect&area,const PanelLayoutMetrics&m){
    FLOWDAW_CHECK(contains(area,m.browser));
    FLOWDAW_CHECK(contains(area,m.editor));
    FLOWDAW_CHECK(!overlaps(m.browser,m.editor));
    FLOWDAW_CHECK(m.editor.width>=PanelLayoutMetrics::editorMinWidth);
    FLOWDAW_CHECK(m.editor.height>=PanelLayoutMetrics::editorMinHeight);
    if(m.mixerVisible){
        FLOWDAW_CHECK(contains(area,m.mixer));
        FLOWDAW_CHECK(!overlaps(m.editor,m.mixer));
        FLOWDAW_CHECK(!overlaps(m.browser,m.mixer));
    }else{
        FLOWDAW_CHECK(m.mixer.width==0&&m.mixer.height==0);
    }
    if(m.utilityVisible){
        FLOWDAW_CHECK(contains(area,m.utility));
        FLOWDAW_CHECK(!overlaps(m.editor,m.utility));
        FLOWDAW_CHECK(!overlaps(m.browser,m.utility));
        FLOWDAW_CHECK(!overlaps(m.mixer,m.utility));
    }else{
        FLOWDAW_CHECK(m.utility.width==0&&m.utility.height==0);
    }
}

int main(){
    struct Case{int w,h;ShellWidthMode mode;};
    constexpr std::array<Case,4> cases{{{1180,720,ShellWidthMode::compact},{1280,800,ShellWidthMode::compact},{1440,1040,ShellWidthMode::wide},{1920,1080,ShellWidthMode::wide}}};
    for(const auto&c:cases){
        const auto m=ShellLayoutMetrics::calculate(c.w,c.h);
        FLOWDAW_CHECK(m.mode==c.mode);
        FLOWDAW_CHECK(m.content.width==c.w-2*ShellLayoutMetrics::inset);
        FLOWDAW_CHECK(m.content.height==c.h-2*ShellLayoutMetrics::inset);
        const std::array rows{m.header,m.transport,m.workspace,m.context,m.pluginTools,m.instrumentTools,m.rackTools,m.sampleTools,m.recordTools};
        for(const auto&r:rows){FLOWDAW_CHECK(m.contains(r));FLOWDAW_CHECK(r.width>0);FLOWDAW_CHECK(r.height>0);}
        for(std::size_t i=1;i<rows.size();++i)FLOWDAW_CHECK(rows[i-1].bottom()<=rows[i].y);
    }

    const auto clamped=ShellLayoutMetrics::calculate(900,500);
    FLOWDAW_CHECK(clamped.content.width==ShellLayoutMetrics::minimumWidth-2*ShellLayoutMetrics::inset);
    FLOWDAW_CHECK(clamped.content.height==ShellLayoutMetrics::minimumHeight-2*ShellLayoutMetrics::inset);

    // Outer windows fit smaller PCs; studio controls remain readable inside
    // a scrollable canvas. A large or maximized window uses all available area.
    for(const auto&size:std::array<std::pair<int,int>,6>{{{640,480},{800,600},{1024,768},{1366,768},{1920,1080},{3840,2160}}}){
        const auto canvas=flowdaw::ui::DesktopCanvasMetrics::calculate(size.first,size.second);
        FLOWDAW_CHECK(canvas.canvasWidth==std::max(size.first,flowdaw::ui::DesktopCanvasMetrics::minimumCanvasWidth));
        FLOWDAW_CHECK(canvas.canvasHeight==std::max(size.second,flowdaw::ui::DesktopCanvasMetrics::minimumCanvasHeight));
        FLOWDAW_CHECK(canvas.horizontalScroll==(size.first<canvas.canvasWidth));
        FLOWDAW_CHECK(canvas.verticalScroll==(size.second<canvas.canvasHeight));
        const auto shell=ShellLayoutMetrics::calculate(canvas.canvasWidth,canvas.canvasHeight);
        FLOWDAW_CHECK(shell.contains(shell.header));
        FLOWDAW_CHECK(shell.contains(shell.recordTools));
        // Reserve enough space below Phase 13/14 control rows for the editor.
        constexpr int reserved=2*ShellLayoutMetrics::inset+46+48+44+28+24+6+38+38+34+88+38+38+4;
        FLOWDAW_CHECK(canvas.canvasHeight-reserved>=PanelLayoutMetrics::editorMinHeight);
    }

    // Browser controls must remain in the available panel, not vanish beyond
    // its right edge when a 220-420px Browser is next to the editor.
    for(const int browserWidth:{220,260,280,320,400,420}){
        const int available=browserWidth-20;
        const bool wrapped=flowdaw::ui::BrowserToolbarMetrics::useTwoRows(available);
        FLOWDAW_CHECK(wrapped==(available<396));
        FLOWDAW_CHECK(flowdaw::ui::BrowserToolbarMetrics::height(available)==(wrapped?68:32));
        if(wrapped){
            constexpr int primaryActions=54+62+48;
            FLOWDAW_CHECK(primaryActions<=available);
            const int first=available*21/100,second=available*31/100,third=available*25/100;
            FLOWDAW_CHECK(first>0&&second>0&&third>0&&available-first-second-third>0);
        }
    }

    const ShellRect minPanelArea{0,0,1148,286};
    const auto minPanels=PanelLayoutMetrics::calculate(minPanelArea);
    FLOWDAW_CHECK(minPanels.browser.width>=PanelLayoutMetrics::browserMinWidth);
    FLOWDAW_CHECK(minPanels.browser.width<=PanelLayoutMetrics::browserMaxWidth);
    FLOWDAW_CHECK(minPanels.editor.width>=PanelLayoutMetrics::editorMinWidth);
    FLOWDAW_CHECK(minPanels.editor.height>=PanelLayoutMetrics::editorMinHeight);
    FLOWDAW_CHECK(!minPanels.mixerVisible);
    FLOWDAW_CHECK(!minPanels.utilityVisible);
    FLOWDAW_CHECK(contains(minPanelArea,minPanels.browser));
    FLOWDAW_CHECK(contains(minPanelArea,minPanels.editor));

    const ShellRect largePanelArea{0,0,1408,606};
    const auto largePanels=PanelLayoutMetrics::calculate(largePanelArea,600,176,220,true,true);
    FLOWDAW_CHECK(largePanels.browser.width==PanelLayoutMetrics::browserMaxWidth);
    FLOWDAW_CHECK(largePanels.editor.width>=PanelLayoutMetrics::editorMinWidth);
    FLOWDAW_CHECK(largePanels.editor.height>=PanelLayoutMetrics::editorMinHeight);
    FLOWDAW_CHECK(largePanels.mixerVisible);
    FLOWDAW_CHECK(largePanels.mixer.height>=PanelLayoutMetrics::mixerMinHeight);
    FLOWDAW_CHECK(largePanels.mixer.height<=PanelLayoutMetrics::mixerMaxHeight);
    FLOWDAW_CHECK(largePanels.utilityVisible);
    FLOWDAW_CHECK(largePanels.utility.height<=PanelLayoutMetrics::utilityMaxHeight);
    FLOWDAW_CHECK(contains(largePanelArea,largePanels.browser));
    FLOWDAW_CHECK(contains(largePanelArea,largePanels.editor));
    FLOWDAW_CHECK(contains(largePanelArea,largePanels.mixer));
    FLOWDAW_CHECK(contains(largePanelArea,largePanels.utility));

    assertPanelContract(minPanelArea,minPanels);
    assertPanelContract(largePanelArea,largePanels);

    // Phase 11.8.1 visual/layout regression matrix: secondary surfaces may
    // consume spare space, but never at the expense of the minimum creative
    // editor contract or by overlapping another visible region.
    constexpr std::array<ShellRect,4> panelAreas{{
        {0,0,1148,286},   // supported minimum shell content
        {0,0,1248,366},   // compact desktop
        {0,0,1408,606},   // default desktop
        {0,0,1888,646}    // wide desktop
    }};
    for(const auto&area:panelAreas){
        for(const bool mixerRequested:{false,true}){
            for(const bool utilityRequested:{false,true}){
                const auto panels=PanelLayoutMetrics::calculate(area,320,220,180,mixerRequested,utilityRequested);
                assertPanelContract(area,panels);
                FLOWDAW_CHECK(panels.browser.width>=PanelLayoutMetrics::browserMinWidth);
                FLOWDAW_CHECK(panels.browser.width<=PanelLayoutMetrics::browserMaxWidth);
                if(!mixerRequested)FLOWDAW_CHECK(!panels.mixerVisible);
                if(!utilityRequested)FLOWDAW_CHECK(!panels.utilityVisible);
            }
        }
    }

    // At constrained height, creative editing wins deterministically: Mixer
    // and utility collapse instead of shrinking the editor below its minimum.
    const ShellRect constrained{0,0,1248,PanelLayoutMetrics::editorMinHeight+PanelLayoutMetrics::gap+PanelLayoutMetrics::mixerMinHeight-1};
    const auto collapsed=PanelLayoutMetrics::calculate(constrained,320,220,180,true,true);
    FLOWDAW_CHECK(!collapsed.mixerVisible);
    FLOWDAW_CHECK(!collapsed.utilityVisible);
    FLOWDAW_CHECK(collapsed.editor.height==constrained.height);
    assertPanelContract(constrained,collapsed);

    const auto narrowBrowser=PanelLayoutMetrics::calculate(largePanelArea,100,176,0,true,false);
    FLOWDAW_CHECK(narrowBrowser.browser.width==PanelLayoutMetrics::browserMinWidth);
    const auto editorOnly=PanelLayoutMetrics::calculate(minPanelArea,280,176,180,false,false);
    FLOWDAW_CHECK(!editorOnly.mixerVisible&&!editorOnly.utilityVisible);
    FLOWDAW_CHECK(editorOnly.editor.height==minPanelArea.height);

    for(const auto&size:std::array<std::pair<int,int>,4>{{{620,240},{820,320},{1000,420},{1400,600}}}){
        const auto a=AutomationAssistLayoutMetrics::calculate(size.first,size.second);
        const ShellRect whole{0,0,size.first,size.second};
        FLOWDAW_CHECK(a.left.width>0&&a.middle.width>0&&a.right.width>0);
        FLOWDAW_CHECK(contains(whole,a.left));
        FLOWDAW_CHECK(contains(whole,a.middle));
        FLOWDAW_CHECK(contains(whole,a.right));
        FLOWDAW_CHECK(a.left.right()<=a.middle.x);
        FLOWDAW_CHECK(a.middle.right()<=a.right.x);
        FLOWDAW_CHECK(contains(a.left,a.graph));
        FLOWDAW_CHECK(a.graph.height>=0);
    }
}
