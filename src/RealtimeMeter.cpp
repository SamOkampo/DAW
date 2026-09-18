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

float RealtimeMeterState::interpolate4x(float p0,float p1,float p2,float p3,float t) noexcept {
    // Four-point Lagrange interpolation on samples at x=-1,0,1,2.
    // Evaluating at 1/4, 1/2 and 3/4 gives a cheap fixed 4x inter-sample
    // estimate without allocating or touching host/plugin state.
    const float l0=-(t)*(t-1.0f)*(t-2.0f)/6.0f;
    const float l1=(t+1.0f)*(t-1.0f)*(t-2.0f)/2.0f;
    const float l2=-(t+1.0f)*t*(t-2.0f)/2.0f;
    const float l3=(t+1.0f)*t*(t-1.0f)/6.0f;
    return p0*l0+p1*l1+p2*l2+p3*l3;
}

void RealtimeMeterState::pushHistory(std::array<float,3>& history,float value) noexcept {
    history[0]=history[1];
    history[1]=history[2];
    history[2]=value;
}

RealtimeMeterState::RealtimeMeterState() noexcept { reset(); }

void RealtimeMeterState::reset() noexcept {
    const auto zero=encode(0.0f);
    peakLeft_.store(zero,std::memory_order_relaxed);
    peakRight_.store(zero,std::memory_order_relaxed);
    truePeakLeft_.store(zero,std::memory_order_relaxed);
    truePeakRight_.store(zero,std::memory_order_relaxed);
    rmsLeft_.store(zero,std::memory_order_relaxed);
    rmsRight_.store(zero,std::memory_order_relaxed);
    historyLeft_.fill(0.0f);
    historyRight_.fill(0.0f);
    historyCount_=0;
}

void RealtimeMeterState::process(const float* interleaved,SampleIndex frames,int channels) noexcept {
    if(!interleaved||frames<=0||channels<=0){reset();return;}

    float peakL=0.0f,peakR=0.0f;
    float truePeakL=0.0f,truePeakR=0.0f;
    double squaresL=0.0,squaresR=0.0;
    for(SampleIndex frame=0;frame<frames;++frame){
        const auto base=static_cast<std::size_t>(frame)*static_cast<std::size_t>(channels);
        const float left=interleaved[base];
        const float right=channels>1?interleaved[base+1]:left;
        peakL=std::max(peakL,std::abs(left));
        peakR=std::max(peakR,std::abs(right));
        truePeakL=std::max(truePeakL,std::abs(left));
        truePeakR=std::max(truePeakR,std::abs(right));
        squaresL+=static_cast<double>(left)*static_cast<double>(left);
        squaresR+=static_cast<double>(right)*static_cast<double>(right);

        if(historyCount_>=3){
            constexpr float phases[]{0.25f,0.5f,0.75f};
            for(const float phase:phases){
                truePeakL=std::max(truePeakL,std::abs(interpolate4x(historyLeft_[0],historyLeft_[1],historyLeft_[2],left,phase)));
                truePeakR=std::max(truePeakR,std::abs(interpolate4x(historyRight_[0],historyRight_[1],historyRight_[2],right,phase)));
            }
        }

        if(historyCount_<3){
            historyLeft_[historyCount_]=left;
            historyRight_[historyCount_]=right;
            ++historyCount_;
        }else{
            pushHistory(historyLeft_,left);
            pushHistory(historyRight_,right);
        }
    }

    const float rmsL=static_cast<float>(std::sqrt(squaresL/static_cast<double>(frames)));
    const float rmsR=static_cast<float>(std::sqrt(squaresR/static_cast<double>(frames)));
    peakLeft_.store(encode(peakL),std::memory_order_relaxed);
    peakRight_.store(encode(peakR),std::memory_order_relaxed);
    truePeakLeft_.store(encode(truePeakL),std::memory_order_relaxed);
    truePeakRight_.store(encode(truePeakR),std::memory_order_relaxed);
    rmsLeft_.store(encode(rmsL),std::memory_order_relaxed);
    rmsRight_.store(encode(rmsR),std::memory_order_relaxed);
}

AudioMeterReading RealtimeMeterState::snapshot() const noexcept {
    AudioMeterReading out;
    out.samplePeakLeft=decode(peakLeft_.load(std::memory_order_relaxed));
    out.samplePeakRight=decode(peakRight_.load(std::memory_order_relaxed));
    out.truePeakLeft=decode(truePeakLeft_.load(std::memory_order_relaxed));
    out.truePeakRight=decode(truePeakRight_.load(std::memory_order_relaxed));
    out.rmsLeft=decode(rmsLeft_.load(std::memory_order_relaxed));
    out.rmsRight=decode(rmsRight_.load(std::memory_order_relaxed));
    return out;
}

} // namespace flowdaw
