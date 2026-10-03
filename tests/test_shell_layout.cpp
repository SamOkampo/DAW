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
