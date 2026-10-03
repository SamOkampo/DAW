#pragma once
#include <string>
#include <utility>

namespace flowdaw::ui {

enum class ExportDeliveryMode { masterMix, stems };
enum class ExportDeliveryStage { idle, choosingDestination, rendering, completed, failed, cancelled };

struct ExportWorkflowState {
    ExportDeliveryMode mode=ExportDeliveryMode::masterMix;
    ExportDeliveryStage stage=ExportDeliveryStage::idle;
    std::string detail;

    void begin(ExportDeliveryMode nextMode) {
        mode=nextMode;stage=ExportDeliveryStage::choosingDestination;detail.clear();
    }
    void cancel() {
        stage=ExportDeliveryStage::cancelled;detail="No files were written";
    }
    void startRendering(std::string destination) {
        stage=ExportDeliveryStage::rendering;detail=std::move(destination);
    }
    void complete(std::string summary) {
        stage=ExportDeliveryStage::completed;detail=std::move(summary);
    }
    void fail(std::string error) {
        stage=ExportDeliveryStage::failed;detail=std::move(error);
    }
    [[nodiscard]] bool active() const noexcept {
        return stage==ExportDeliveryStage::choosingDestination||stage==ExportDeliveryStage::rendering;
    }
    [[nodiscard]] const char* modeLabel() const noexcept {
        return mode==ExportDeliveryMode::masterMix?"Master Mix":"Track Stems";
    }
    [[nodiscard]] std::string statusText() const {
        switch(stage){
            case ExportDeliveryStage::choosingDestination:return std::string("Export • ")+modeLabel()+" • choose destination";
            case ExportDeliveryStage::rendering:return std::string("Export • ")+modeLabel()+" • rendering"+(detail.empty()?"":" → "+detail);
            case ExportDeliveryStage::completed:return std::string("Export complete • ")+modeLabel()+(detail.empty()?"":" • "+detail);
            case ExportDeliveryStage::failed:return std::string("Export failed • ")+modeLabel()+(detail.empty()?"":" • "+detail);
            case ExportDeliveryStage::cancelled:return std::string("Export cancelled • ")+modeLabel()+" • "+detail;
            default:return "Export ready";
        }
    }
};

} // namespace flowdaw::ui
