#include "flowdaw/ExportWorkflow.hpp"
#include <cassert>
#include <string>

using flowdaw::ui::ExportDeliveryMode;
using flowdaw::ui::ExportDeliveryStage;
using flowdaw::ui::ExportWorkflowState;

int main(){
    ExportWorkflowState s;
    assert(s.stage==ExportDeliveryStage::idle);
    assert(!s.active());
    assert(std::string(s.modeLabel())=="Master Mix");

    s.begin(ExportDeliveryMode::stems);
    assert(s.stage==ExportDeliveryStage::choosingDestination);
    assert(s.active());
    assert(std::string(s.modeLabel())=="Track Stems");
    assert(s.statusText().find("choose destination")!=std::string::npos);

    s.startRendering("/tmp/stems");
    assert(s.stage==ExportDeliveryStage::rendering);
    assert(s.active());
    assert(s.statusText().find("/tmp/stems")!=std::string::npos);

    s.complete("4 stems");
    assert(s.stage==ExportDeliveryStage::completed);
    assert(!s.active());
    assert(s.statusText().find("4 stems")!=std::string::npos);

    s.begin(ExportDeliveryMode::masterMix);
    s.fail("destination is not writable");
    assert(s.stage==ExportDeliveryStage::failed);
    assert(!s.active());
    assert(s.statusText().find("destination is not writable")!=std::string::npos);

    s.begin(ExportDeliveryMode::masterMix);
    s.cancel();
    assert(s.stage==ExportDeliveryStage::cancelled);
    assert(!s.active());
    assert(s.statusText().find("No files were written")!=std::string::npos);
}
