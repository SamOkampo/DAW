#include "flowdaw/ShellLayout.hpp"
#include <array>
#include <cassert>

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
    assert(contains(area,m.browser));
    assert(contains(area,m.editor));
    assert(!overlaps(m.browser,m.editor));
    assert(m.editor.width>=PanelLayoutMetrics::editorMinWidth);
    assert(m.editor.height>=PanelLayoutMetrics::editorMinHeight);
    if(m.mixerVisible){
        assert(contains(area,m.mixer));
        assert(!overlaps(m.editor,m.mixer));
        assert(!overlaps(m.browser,m.mixer));
    }else{
        assert(m.mixer.width==0&&m.mixer.height==0);
    }
    if(m.utilityVisible){
        assert(contains(area,m.utility));
        assert(!overlaps(m.editor,m.utility));
        assert(!overlaps(m.browser,m.utility));
        assert(!overlaps(m.mixer,m.utility));
    }else{
        assert(m.utility.width==0&&m.utility.height==0);
    }
}

int main(){
    struct Case{int w,h;ShellWidthMode mode;};
    constexpr std::array<Case,4> cases{{{1180,720,ShellWidthMode::compact},{1280,800,ShellWidthMode::compact},{1440,1040,ShellWidthMode::wide},{1920,1080,ShellWidthMode::wide}}};
    for(const auto&c:cases){
        const auto m=ShellLayoutMetrics::calculate(c.w,c.h);
        assert(m.mode==c.mode);
        assert(m.content.width==c.w-2*ShellLayoutMetrics::inset);
        assert(m.content.height==c.h-2*ShellLayoutMetrics::inset);
        const std::array rows{m.header,m.transport,m.workspace,m.context,m.pluginTools,m.instrumentTools,m.rackTools,m.sampleTools,m.recordTools};
        for(const auto&r:rows){assert(m.contains(r));assert(r.width>0);assert(r.height>0);}
        for(std::size_t i=1;i<rows.size();++i)assert(rows[i-1].bottom()<=rows[i].y);
    }

    const auto clamped=ShellLayoutMetrics::calculate(900,500);
    assert(clamped.content.width==ShellLayoutMetrics::minimumWidth-2*ShellLayoutMetrics::inset);
    assert(clamped.content.height==ShellLayoutMetrics::minimumHeight-2*ShellLayoutMetrics::inset);

    // Outer windows fit smaller PCs; studio controls remain readable inside
    // a scrollable canvas. A large or maximized window uses all available area.
    for(const auto&size:std::array<std::pair<int,int>,6>{{{640,480},{800,600},{1024,768},{1366,768},{1920,1080},{3840,2160}}}){
        const auto canvas=flowdaw::ui::DesktopCanvasMetrics::calculate(size.first,size.second);
        assert(canvas.canvasWidth==std::max(size.first,flowdaw::ui::DesktopCanvasMetrics::minimumCanvasWidth));
        assert(canvas.canvasHeight==std::max(size.second,flowdaw::ui::DesktopCanvasMetrics::minimumCanvasHeight));
        assert(canvas.horizontalScroll==(size.first<canvas.canvasWidth));
        assert(canvas.verticalScroll==(size.second<canvas.canvasHeight));
        const auto shell=ShellLayoutMetrics::calculate(canvas.canvasWidth,canvas.canvasHeight);
        assert(shell.contains(shell.header));
        assert(shell.contains(shell.recordTools));
        // Reserve enough space below Phase 13/14 control rows for the editor.
        constexpr int reserved=2*ShellLayoutMetrics::inset+46+48+44+28+24+6+38+38+34+88+38+38+4;
        assert(canvas.canvasHeight-reserved>=PanelLayoutMetrics::editorMinHeight);
    }

    const ShellRect minPanelArea{0,0,1148,286};
    const auto minPanels=PanelLayoutMetrics::calculate(minPanelArea);
    assert(minPanels.browser.width>=PanelLayoutMetrics::browserMinWidth);
    assert(minPanels.browser.width<=PanelLayoutMetrics::browserMaxWidth);
    assert(minPanels.editor.width>=PanelLayoutMetrics::editorMinWidth);
    assert(minPanels.editor.height>=PanelLayoutMetrics::editorMinHeight);
    assert(!minPanels.mixerVisible);
    assert(!minPanels.utilityVisible);
    assert(contains(minPanelArea,minPanels.browser));
    assert(contains(minPanelArea,minPanels.editor));

    const ShellRect largePanelArea{0,0,1408,606};
    const auto largePanels=PanelLayoutMetrics::calculate(largePanelArea,600,176,220,true,true);
    assert(largePanels.browser.width==PanelLayoutMetrics::browserMaxWidth);
    assert(largePanels.editor.width>=PanelLayoutMetrics::editorMinWidth);
    assert(largePanels.editor.height>=PanelLayoutMetrics::editorMinHeight);
    assert(largePanels.mixerVisible);
    assert(largePanels.mixer.height>=PanelLayoutMetrics::mixerMinHeight);
    assert(largePanels.mixer.height<=PanelLayoutMetrics::mixerMaxHeight);
    assert(largePanels.utilityVisible);
    assert(largePanels.utility.height<=PanelLayoutMetrics::utilityMaxHeight);
    assert(contains(largePanelArea,largePanels.browser));
    assert(contains(largePanelArea,largePanels.editor));
    assert(contains(largePanelArea,largePanels.mixer));
    assert(contains(largePanelArea,largePanels.utility));

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
                assert(panels.browser.width>=PanelLayoutMetrics::browserMinWidth);
                assert(panels.browser.width<=PanelLayoutMetrics::browserMaxWidth);
                if(!mixerRequested)assert(!panels.mixerVisible);
                if(!utilityRequested)assert(!panels.utilityVisible);
            }
        }
    }

    // At constrained height, creative editing wins deterministically: Mixer
    // and utility collapse instead of shrinking the editor below its minimum.
    const ShellRect constrained{0,0,1248,PanelLayoutMetrics::editorMinHeight+PanelLayoutMetrics::gap+PanelLayoutMetrics::mixerMinHeight-1};
    const auto collapsed=PanelLayoutMetrics::calculate(constrained,320,220,180,true,true);
    assert(!collapsed.mixerVisible);
    assert(!collapsed.utilityVisible);
    assert(collapsed.editor.height==constrained.height);
    assertPanelContract(constrained,collapsed);

    const auto narrowBrowser=PanelLayoutMetrics::calculate(largePanelArea,100,176,0,true,false);
    assert(narrowBrowser.browser.width==PanelLayoutMetrics::browserMinWidth);
    const auto editorOnly=PanelLayoutMetrics::calculate(minPanelArea,280,176,180,false,false);
    assert(!editorOnly.mixerVisible&&!editorOnly.utilityVisible);
    assert(editorOnly.editor.height==minPanelArea.height);

    for(const auto&size:std::array<std::pair<int,int>,4>{{{620,240},{820,320},{1000,420},{1400,600}}}){
        const auto a=AutomationAssistLayoutMetrics::calculate(size.first,size.second);
        const ShellRect whole{0,0,size.first,size.second};
        assert(a.left.width>0&&a.middle.width>0&&a.right.width>0);
        assert(contains(whole,a.left));
        assert(contains(whole,a.middle));
        assert(contains(whole,a.right));
        assert(a.left.right()<=a.middle.x);
        assert(a.middle.right()<=a.right.x);
        assert(contains(a.left,a.graph));
        assert(a.graph.height>=0);
    }
}
