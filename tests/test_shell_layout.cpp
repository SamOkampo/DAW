#include "flowdaw/ShellLayout.hpp"
#include <array>
#include <cassert>
using flowdaw::ui::ShellLayoutMetrics;
using flowdaw::ui::ShellWidthMode;
int main(){struct Case{int w,h;ShellWidthMode mode;};constexpr std::array<Case,4> cases{{{1180,720,ShellWidthMode::compact},{1280,800,ShellWidthMode::compact},{1440,1040,ShellWidthMode::wide},{1920,1080,ShellWidthMode::wide}}};for(const auto&c:cases){const auto m=ShellLayoutMetrics::calculate(c.w,c.h);assert(m.mode==c.mode);assert(m.content.width==c.w-2*ShellLayoutMetrics::inset);assert(m.content.height==c.h-2*ShellLayoutMetrics::inset);const std::array rows{m.header,m.transport,m.workspace,m.context,m.pluginTools,m.instrumentTools,m.rackTools,m.sampleTools,m.recordTools};for(const auto&r:rows){assert(m.contains(r));assert(r.width>0);assert(r.height>0);}for(std::size_t i=1;i<rows.size();++i)assert(rows[i-1].bottom()<=rows[i].y);}const auto clamped=ShellLayoutMetrics::calculate(900,500);assert(clamped.content.width==ShellLayoutMetrics::minimumWidth-2*ShellLayoutMetrics::inset);assert(clamped.content.height==ShellLayoutMetrics::minimumHeight-2*ShellLayoutMetrics::inset);}
