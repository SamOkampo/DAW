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

} // namespace flowdaw::ui
