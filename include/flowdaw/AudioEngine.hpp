#pragma once
#include "flowdaw/Project.hpp"
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
private:
    struct RenderClip { SampleIndex start=0,sourceStart=0,length=0; float gain=1; float pan=0; std::shared_ptr<AudioBuffer> audio; };
    struct Graph { float master=1; std::vector<RenderClip> clips; };
    std::atomic<Graph*> current_{nullptr};
    std::vector<std::unique_ptr<Graph>> retired_; // graphs never freed while callback can observe them
    mutable std::mutex publishMutex_;
    std::atomic<bool> playing_{false};
    std::atomic<SampleIndex> playhead_{0};
    int sampleRate_=48000;
    unsigned long framesPerBuffer_=256;
    void* stream_=nullptr;
    std::string error_;
    static int paCallback(const void*,void*,unsigned long,const void*,unsigned long,void*);
    int process(float* out,unsigned long frames);
};
}
