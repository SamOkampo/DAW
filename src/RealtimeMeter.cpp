#include "flowdaw/RealtimeMeter.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace flowdaw {

std::uint32_t RealtimeMeterState::encode(float value) noexcept {
    return std::bit_cast<std::uint32_t>(value);
}

float RealtimeMeterState::decode(std::uint32_t value) noexcept {
    return std::bit_cast<float>(value);
}

RealtimeMeterState::RealtimeMeterState() noexcept { reset(); }

void RealtimeMeterState::reset() noexcept {
    const auto zero=encode(0.0f);
    peakLeft_.store(zero,std::memory_order_relaxed);
    peakRight_.store(zero,std::memory_order_relaxed);
    rmsLeft_.store(zero,std::memory_order_relaxed);
    rmsRight_.store(zero,std::memory_order_relaxed);
}

void RealtimeMeterState::process(const float* interleaved,SampleIndex frames,int channels) noexcept {
    if(!interleaved||frames<=0||channels<=0){reset();return;}

    float peakL=0.0f,peakR=0.0f;
    double squaresL=0.0,squaresR=0.0;
    for(SampleIndex frame=0;frame<frames;++frame){
        const auto base=static_cast<std::size_t>(frame)*static_cast<std::size_t>(channels);
        const float left=interleaved[base];
        const float right=channels>1?interleaved[base+1]:left;
        peakL=std::max(peakL,std::abs(left));
        peakR=std::max(peakR,std::abs(right));
        squaresL+=static_cast<double>(left)*static_cast<double>(left);
        squaresR+=static_cast<double>(right)*static_cast<double>(right);
    }

    const float rmsL=static_cast<float>(std::sqrt(squaresL/static_cast<double>(frames)));
    const float rmsR=static_cast<float>(std::sqrt(squaresR/static_cast<double>(frames)));
    peakLeft_.store(encode(peakL),std::memory_order_relaxed);
    peakRight_.store(encode(peakR),std::memory_order_relaxed);
    rmsLeft_.store(encode(rmsL),std::memory_order_relaxed);
    rmsRight_.store(encode(rmsR),std::memory_order_relaxed);
}

AudioMeterReading RealtimeMeterState::snapshot() const noexcept {
    AudioMeterReading out;
    out.samplePeakLeft=decode(peakLeft_.load(std::memory_order_relaxed));
    out.samplePeakRight=decode(peakRight_.load(std::memory_order_relaxed));
    out.rmsLeft=decode(rmsLeft_.load(std::memory_order_relaxed));
    out.rmsRight=decode(rmsRight_.load(std::memory_order_relaxed));
    return out;
}

} // namespace flowdaw
