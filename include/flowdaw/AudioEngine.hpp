#pragma once
#include "flowdaw/Project.hpp"
#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

namespace flowdaw {
class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();
    AudioEngine(const AudioEngine&)=delete; AudioEngine& operator=(const AudioEngine&)=delete;

    bool open(int sampleRate=48000,unsigned long framesPerBuffer=256);
    void close();
    void publish(const Project& project);
    void play();
    void pause();
    void stop();
    bool isPlaying() const { return playing_.load(std::memory_order_relaxed); }
    SampleIndex playheadSamples() const { return playhead_.load(std::memory_order_relaxed); }
    void seekSamples(SampleIndex s) { playhead_.store(s,std::memory_order_relaxed); }
    int sampleRate() const { return sampleRate_; }
    std::string lastError() const;
    AudioBuffer renderOffline(SampleIndex frames) const;

    bool triggerPreview(std::shared_ptr<AudioBuffer> audio,SampleIndex sourceStart,SampleIndex sourceLength,float gain=1.0f,float pan=0.0f,int chokeGroup=0);
    void stopPreviews();

    bool beginRecording(SampleIndex maxFrames);
    AudioBuffer finishRecording();
    bool isRecording() const { return recording_.load(std::memory_order_acquire); }
    SampleIndex recordedFrames() const { return recordingWrite_.load(std::memory_order_acquire); }
    bool inputAvailable() const { return inputAvailable_.load(std::memory_order_relaxed); }
    void setInputMonitoring(bool enabled) { inputMonitoring_.store(enabled,std::memory_order_relaxed); }
    bool inputMonitoring() const { return inputMonitoring_.load(std::memory_order_relaxed); }

    AudioBuffer renderDeviceBlockForTest(SampleIndex frames);
    AudioBuffer processInputBlockForTest(const AudioBuffer& monoInput);
private:
    struct RenderClip {
        SampleIndex start=0,sourceStart=0,length=0;
        float gain=1,pan=0;
        int chokeGroup=0;
        std::shared_ptr<AudioBuffer> audio;
        float trackVolume=1.0f,trackPan=0.0f,busVolume=1.0f,busPan=0.0f,sendGain=1.0f;
        int trackVolumeAutomation=-1,trackPanAutomation=-1,busVolumeAutomation=-1,busPanAutomation=-1,sendAutomation=-1;
    };
    struct Graph {
        float master=1;
        float masterEffectGain=1;
        double bpm=90.0;
        int masterVolumeAutomation=-1;
        std::vector<AutomationLane> automation;
        std::vector<PluginInstance> masterPlugins;
        std::vector<RenderClip> clips;
    };
    struct PreviewCommand { const AudioBuffer* audio=nullptr; SampleIndex start=0,length=0; float gain=1,pan=0; int chokeGroup=0; bool stopAll=false; };
    struct PreviewVoice { const AudioBuffer* audio=nullptr; SampleIndex start=0,length=0,position=0; float gain=1,pan=0; int chokeGroup=0; bool active=false; };
    static constexpr std::size_t kPreviewQueueSize=64;
    static constexpr std::size_t kPreviewVoices=16;

    std::atomic<Graph*> current_{nullptr};
    std::vector<std::unique_ptr<Graph>> retired_;
    mutable std::mutex publishMutex_;
    std::atomic<bool> playing_{false};
    std::atomic<SampleIndex> playhead_{0};
    int sampleRate_=48000;
    unsigned long framesPerBuffer_=256;
    void* stream_=nullptr;
    std::string error_;
    std::atomic<bool> inputAvailable_{false};

    std::array<PreviewCommand,kPreviewQueueSize> previewQueue_{};
    std::atomic<std::uint32_t> previewWrite_{0},previewRead_{0};
    std::array<PreviewVoice,kPreviewVoices> previewVoices_{};
    std::vector<std::shared_ptr<AudioBuffer>> previewKeepAlive_;
    std::mutex previewLifetimeMutex_;

    std::vector<float> recordingBuffer_;
    std::atomic<SampleIndex> recordingWrite_{0};
    std::atomic<bool> recording_{false};
    std::atomic<int> recordingWriters_{0};
    std::atomic<bool> inputMonitoring_{false};

    static int paCallback(const void*,void*,unsigned long,const void*,unsigned long,void*);
    int process(const float* in,float* out,unsigned long frames);
    void consumePreviewCommands();
    void mixPreviewVoices(float* out,unsigned long frames);
    void captureInput(const float* in,unsigned long frames);
};
}
