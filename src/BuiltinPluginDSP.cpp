#include "flowdaw/BuiltinPluginDSP.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace flowdaw {
namespace {

constexpr float kPi=3.14159265358979323846f;
constexpr int kEqBands=6;

class GainProcessor final:public IPluginProcessor{
public:
    explicit GainProcessor(float gain):gain_(gain){}
    bool prepare(int,int,std::string&)override{return true;}
    void setState(const std::string&)override{}
    std::string state()const override{return{};}
    void process(AudioBuffer&b)override{(void)processRealtime(b.interleaved.data(),b.frames(),b.channels);}
    bool supportsRealtimeProcessing()const noexcept override{return true;}
    bool processRealtime(float*data,SampleIndex frames,int channels)noexcept override{
        if(!data||frames<=0||channels<=0)return false;
        const auto count=static_cast<std::size_t>(frames)*static_cast<std::size_t>(channels);
        for(std::size_t i=0;i<count;++i)data[i]*=gain_;
        return true;
    }
private:
    float gain_=1.0f;
};

class SoftClipProcessor final:public IPluginProcessor{
public:
    explicit SoftClipProcessor(float drive):drive_(std::clamp(drive,0.0f,1.0f)){
        amount_=1.0f+drive_*9.0f;
        norm_=std::max(0.0001f,std::tanh(amount_));
    }
    bool prepare(int,int,std::string&)override{return true;}
    void setState(const std::string&)override{}
    std::string state()const override{return{};}
    void process(AudioBuffer&b)override{(void)processRealtime(b.interleaved.data(),b.frames(),b.channels);}
    bool supportsRealtimeProcessing()const noexcept override{return true;}
    bool processRealtime(float*data,SampleIndex frames,int channels)noexcept override{
        if(!data||frames<=0||channels<=0)return false;
        const auto count=static_cast<std::size_t>(frames)*static_cast<std::size_t>(channels);
        for(std::size_t i=0;i<count;++i)data[i]=std::tanh(data[i]*amount_)/norm_;
        return true;
    }
private:
    float drive_=0.25f,amount_=3.25f,norm_=1.0f;
};

class WidthProcessor final:public IPluginProcessor{
public:
    explicit WidthProcessor(float width):width_(std::clamp(width,0.0f,2.0f)){}
    bool prepare(int,int channels,std::string&error)override{
        if(channels<2){error="FLOW Width requires stereo audio";return false;}
        return true;
    }
    void setState(const std::string&)override{}
    std::string state()const override{return{};}
    void process(AudioBuffer&b)override{(void)processRealtime(b.interleaved.data(),b.frames(),b.channels);}
    bool supportsRealtimeProcessing()const noexcept override{return true;}
    bool processRealtime(float*data,SampleIndex frames,int channels)noexcept override{
        if(!data||frames<=0||channels<2)return false;
        for(SampleIndex frame=0;frame<frames;++frame){
            const auto i=static_cast<std::size_t>(frame)*static_cast<std::size_t>(channels);
            const float l=data[i],r=data[i+1],m=(l+r)*0.5f,s=(l-r)*0.5f*width_;
            data[i]=m+s;data[i+1]=m-s;
        }
        return true;
    }
private:
    float width_=1.0f;
};

struct BiquadCoefficients{
    float b0=1.0f,b1=0.0f,b2=0.0f,a1=0.0f,a2=0.0f;
};

struct BiquadState{
    float z1=0.0f,z2=0.0f;
};

BiquadCoefficients peakCoefficients(float sampleRate,float frequency,float gainDb,float q)noexcept{
    BiquadCoefficients c;
    if(sampleRate<=0.0f||std::abs(gainDb)<0.0001f)return c;
    const float nyquist=sampleRate*0.5f;
    const float f=std::clamp(frequency,20.0f,std::max(20.0f,nyquist*0.90f));
    const float qq=std::clamp(q,0.10f,12.0f);
    const float a=std::pow(10.0f,gainDb/40.0f);
    const float w=2.0f*kPi*f/sampleRate;
    const float alpha=std::sin(w)/(2.0f*qq);
    const float cosw=std::cos(w);
    const float a0=1.0f+alpha/a;
    if(std::abs(a0)<std::numeric_limits<float>::epsilon())return c;
    const float inv=1.0f/a0;
    c.b0=(1.0f+alpha*a)*inv;
    c.b1=(-2.0f*cosw)*inv;
    c.b2=(1.0f-alpha*a)*inv;
    c.a1=(-2.0f*cosw)*inv;
    c.a2=(1.0f-alpha/a)*inv;
    return c;
}

class EqProcessor final:public IPluginProcessor{
public:
    explicit EqProcessor(const PluginInstance& plugin){
        static constexpr std::array<float,kEqBands> defaultFreq{80.0f,200.0f,500.0f,1500.0f,5000.0f,12000.0f};
        for(int band=0;band<kEqBands;++band){
            const auto n=std::to_string(band+1);
            frequencies_[static_cast<std::size_t>(band)]=std::clamp(pluginParameterValue(plugin,"band"+n+"_freq",defaultFreq[static_cast<std::size_t>(band)]),20.0f,20000.0f);
            gainsDb_[static_cast<std::size_t>(band)]=std::clamp(pluginParameterValue(plugin,"band"+n+"_gain_db",0.0f),-18.0f,18.0f);
            q_[static_cast<std::size_t>(band)]=std::clamp(pluginParameterValue(plugin,"band"+n+"_q",0.707f),0.10f,12.0f);
        }
        outputGainDb_=std::clamp(pluginParameterValue(plugin,"output_gain_db",0.0f),-18.0f,18.0f);
    }

    bool prepare(int sampleRate,int channels,std::string&error)override{
        if(sampleRate<8000||sampleRate>384000){error="FLOW EQ sample rate is out of range";return false;}
        if(channels<1||channels>32){error="FLOW EQ channel count is out of range";return false;}
        sampleRate_=sampleRate;channels_=channels;
        for(int band=0;band<kEqBands;++band){
            coefficients_[static_cast<std::size_t>(band)]=peakCoefficients(
                static_cast<float>(sampleRate_),
                frequencies_[static_cast<std::size_t>(band)],
                gainsDb_[static_cast<std::size_t>(band)],
                q_[static_cast<std::size_t>(band)]);
        }
        states_.assign(static_cast<std::size_t>(channels_)*kEqBands,{});
        outputGain_=std::pow(10.0f,outputGainDb_/20.0f);
        return true;
    }

    void setState(const std::string&)override{}
    std::string state()const override{return{};}
    void process(AudioBuffer&b)override{(void)processRealtime(b.interleaved.data(),b.frames(),b.channels);}
    bool supportsRealtimeProcessing()const noexcept override{return true;}

    bool processRealtime(float*data,SampleIndex frames,int channels)noexcept override{
        if(!data||frames<=0||channels!=channels_||states_.size()!=static_cast<std::size_t>(channels_)*kEqBands)return false;
        for(SampleIndex frame=0;frame<frames;++frame){
            const auto frameBase=static_cast<std::size_t>(frame)*static_cast<std::size_t>(channels_);
            for(int channel=0;channel<channels_;++channel){
                float x=data[frameBase+static_cast<std::size_t>(channel)];
                const auto stateBase=static_cast<std::size_t>(channel)*kEqBands;
                for(int band=0;band<kEqBands;++band){
                    const auto& c=coefficients_[static_cast<std::size_t>(band)];
                    auto& s=states_[stateBase+static_cast<std::size_t>(band)];
                    const float y=c.b0*x+s.z1;
                    s.z1=c.b1*x-c.a1*y+s.z2;
                    s.z2=c.b2*x-c.a2*y;
                    x=y;
                }
                data[frameBase+static_cast<std::size_t>(channel)]=x*outputGain_;
            }
        }
        for(auto&state:states_){
            if(std::abs(state.z1)<1.0e-20f)state.z1=0.0f;
            if(std::abs(state.z2)<1.0e-20f)state.z2=0.0f;
        }
        return true;
    }

    void resetRealtime()noexcept override{
        for(auto&state:states_)state={};
    }

private:
    int sampleRate_=48000,channels_=2;
    std::array<float,kEqBands> frequencies_{};
    std::array<float,kEqBands> gainsDb_{};
    std::array<float,kEqBands> q_{};
    std::array<BiquadCoefficients,kEqBands> coefficients_{};
    std::vector<BiquadState> states_;
    float outputGainDb_=0.0f,outputGain_=1.0f;
};


class CompressorProcessor final:public IPluginProcessor{
public:
    explicit CompressorProcessor(const PluginInstance& plugin){
        thresholdDb_=std::clamp(pluginParameterValue(plugin,"threshold_db",-18.0f),-60.0f,0.0f);
        ratio_=std::clamp(pluginParameterValue(plugin,"ratio",4.0f),1.0f,20.0f);
        attackMs_=std::clamp(pluginParameterValue(plugin,"attack_ms",10.0f),0.1f,200.0f);
        releaseMs_=std::clamp(pluginParameterValue(plugin,"release_ms",120.0f),5.0f,2000.0f);
        kneeDb_=std::clamp(pluginParameterValue(plugin,"knee_db",6.0f),0.0f,24.0f);
        makeupDb_=std::clamp(pluginParameterValue(plugin,"makeup_db",0.0f),-12.0f,24.0f);
    }

    bool prepare(int sampleRate,int channels,std::string&error)override{
        if(sampleRate<8000||sampleRate>384000){error="FLOW Compressor sample rate is out of range";return false;}
        if(channels<1||channels>32){error="FLOW Compressor channel count is out of range";return false;}
        sampleRate_=sampleRate;channels_=channels;
        const float attackSeconds=std::max(0.0001f,attackMs_*0.001f);
        const float releaseSeconds=std::max(0.001f,releaseMs_*0.001f);
        attackCoeff_=std::exp(-1.0f/(attackSeconds*sampleRate_));
        releaseCoeff_=std::exp(-1.0f/(releaseSeconds*sampleRate_));
        makeupGain_=std::pow(10.0f,makeupDb_/20.0f);
        envelope_=0.0f;
        return true;
    }

    void setState(const std::string&)override{}
    std::string state()const override{return{};}
    void process(AudioBuffer&b)override{(void)processRealtime(b.interleaved.data(),b.frames(),b.channels);}
    bool supportsRealtimeProcessing()const noexcept override{return true;}

    bool processRealtime(float*data,SampleIndex frames,int channels)noexcept override{
        if(!data||frames<=0||channels!=channels_)return false;
        const float slope=1.0f-(1.0f/ratio_);
        const float halfKnee=kneeDb_*0.5f;
        for(SampleIndex frame=0;frame<frames;++frame){
            const auto base=static_cast<std::size_t>(frame)*static_cast<std::size_t>(channels_);
            float detector=0.0f;
            for(int channel=0;channel<channels_;++channel)
                detector=std::max(detector,std::abs(data[base+static_cast<std::size_t>(channel)]));
            const float coefficient=detector>envelope_?attackCoeff_:releaseCoeff_;
            envelope_=coefficient*envelope_+(1.0f-coefficient)*detector;
            const float levelDb=20.0f*std::log10(std::max(envelope_,1.0e-9f));
            const float over=levelDb-thresholdDb_;
            float gainReductionDb=0.0f;
            if(kneeDb_<=0.0001f){
                if(over>0.0f)gainReductionDb=-slope*over;
            }else if(over<=-halfKnee){
                gainReductionDb=0.0f;
            }else if(over>=halfKnee){
                gainReductionDb=-slope*over;
            }else{
                const float x=over+halfKnee;
                gainReductionDb=-slope*x*x/(2.0f*kneeDb_);
            }
            const float gain=std::pow(10.0f,gainReductionDb/20.0f)*makeupGain_;
            for(int channel=0;channel<channels_;++channel)
                data[base+static_cast<std::size_t>(channel)]*=gain;
        }
        if(envelope_<1.0e-20f)envelope_=0.0f;
        return true;
    }

    void resetRealtime()noexcept override{envelope_=0.0f;}

private:
    int sampleRate_=48000,channels_=2;
    float thresholdDb_=-18.0f,ratio_=4.0f,attackMs_=10.0f,releaseMs_=120.0f,kneeDb_=6.0f,makeupDb_=0.0f;
    float attackCoeff_=0.0f,releaseCoeff_=0.0f,makeupGain_=1.0f,envelope_=0.0f;
};


class LimiterProcessor final:public IPluginProcessor{
public:
    explicit LimiterProcessor(const PluginInstance& plugin){
        ceilingDb_=std::clamp(pluginParameterValue(plugin,"ceiling_db",-1.0f),-12.0f,0.0f);
        inputGainDb_=std::clamp(pluginParameterValue(plugin,"input_gain_db",0.0f),-12.0f,24.0f);
        lookaheadMs_=std::clamp(pluginParameterValue(plugin,"lookahead_ms",3.0f),0.0f,10.0f);
        releaseMs_=std::clamp(pluginParameterValue(plugin,"release_ms",80.0f),5.0f,500.0f);
    }

    bool prepare(int sampleRate,int channels,std::string&error)override{
        if(sampleRate<8000||sampleRate>384000){error="FLOW Limiter sample rate is out of range";return false;}
        if(channels<1||channels>32){error="FLOW Limiter channel count is out of range";return false;}
        sampleRate_=sampleRate;channels_=channels;
        lookaheadSamples_=std::max(0,static_cast<int>(std::lround(lookaheadMs_*0.001f*sampleRate_)));
        const auto delaySamples=static_cast<std::size_t>(lookaheadSamples_)*static_cast<std::size_t>(channels_);
        delay_.assign(delaySamples,0.0f);
        writeFrame_=0;
        gain_=1.0f;
        ceilingLinear_=std::pow(10.0f,ceilingDb_/20.0f);
        inputGainLinear_=std::pow(10.0f,inputGainDb_/20.0f);
        const float releaseSeconds=std::max(0.001f,releaseMs_*0.001f);
        releaseCoeff_=std::exp(-1.0f/(releaseSeconds*sampleRate_));
        return true;
    }

    void setState(const std::string&)override{}
    std::string state()const override{return{};}
    void process(AudioBuffer&b)override{(void)processRealtime(b.interleaved.data(),b.frames(),b.channels);}
    bool supportsRealtimeProcessing()const noexcept override{return true;}
    int latencySamples()const noexcept override{return lookaheadSamples_;}

    bool processRealtime(float*data,SampleIndex frames,int channels)noexcept override{
        if(!data||frames<=0||channels!=channels_)return false;
        for(SampleIndex frame=0;frame<frames;++frame){
            const auto base=static_cast<std::size_t>(frame)*static_cast<std::size_t>(channels_);
            float peak=0.0f;
            for(int channel=0;channel<channels_;++channel){
                float x=data[base+static_cast<std::size_t>(channel)]*inputGainLinear_;
                if(!std::isfinite(x))x=0.0f;
                peak=std::max(peak,std::abs(x));
            }
            const float target=peak>ceilingLinear_&&peak>1.0e-12f?ceilingLinear_/peak:1.0f;
            if(target<gain_)gain_=target;
            else gain_=releaseCoeff_*gain_+(1.0f-releaseCoeff_)*target;

            if(lookaheadSamples_<=0){
                for(int channel=0;channel<channels_;++channel){
                    float x=data[base+static_cast<std::size_t>(channel)]*inputGainLinear_;
                    if(!std::isfinite(x))x=0.0f;
                    data[base+static_cast<std::size_t>(channel)]=std::clamp(x*gain_,-ceilingLinear_,ceilingLinear_);
                }
                continue;
            }

            const auto ringBase=static_cast<std::size_t>(writeFrame_)*static_cast<std::size_t>(channels_);
            for(int channel=0;channel<channels_;++channel){
                const auto c=static_cast<std::size_t>(channel);
                const float delayed=delay_[ringBase+c];
                float x=data[base+c]*inputGainLinear_;
                if(!std::isfinite(x))x=0.0f;
                delay_[ringBase+c]=x;
                data[base+c]=std::clamp(delayed*gain_,-ceilingLinear_,ceilingLinear_);
            }
            if(++writeFrame_>=lookaheadSamples_)writeFrame_=0;
        }
        return true;
    }

    void resetRealtime()noexcept override{
        std::fill(delay_.begin(),delay_.end(),0.0f);
        writeFrame_=0;gain_=1.0f;
    }

private:
    int sampleRate_=48000,channels_=2,lookaheadSamples_=0,writeFrame_=0;
    float ceilingDb_=-1.0f,inputGainDb_=0.0f,lookaheadMs_=3.0f,releaseMs_=80.0f;
    float ceilingLinear_=0.89125f,inputGainLinear_=1.0f,releaseCoeff_=0.0f,gain_=1.0f;
    std::vector<float> delay_;
};


class SaturatorProcessor final:public IPluginProcessor{
public:
    explicit SaturatorProcessor(const PluginInstance& plugin){
        driveDb_=std::clamp(pluginParameterValue(plugin,"drive_db",6.0f),0.0f,24.0f);
        tone_=std::clamp(pluginParameterValue(plugin,"tone",0.0f),-1.0f,1.0f);
        mode_=std::clamp(static_cast<int>(std::lround(pluginParameterValue(plugin,"mode",0.0f))),0,2);
    }
    bool prepare(int sampleRate,int channels,std::string&error)override{
        if(sampleRate<8000||sampleRate>384000){error="FLOW Saturator sample rate is out of range";return false;}
        if(channels<1||channels>32){error="FLOW Saturator channel count is out of range";return false;}
        channels_=channels;drive_=std::pow(10.0f,driveDb_/20.0f);
        const float cutoff=std::clamp(3500.0f+tone_*3000.0f,500.0f,12000.0f);
        lpCoeff_=std::exp(-2.0f*kPi*cutoff/static_cast<float>(sampleRate));
        toneState_.assign(static_cast<std::size_t>(channels_),0.0f);
        return true;
    }
    void setState(const std::string&)override{}
    std::string state()const override{return{};}
    void process(AudioBuffer&b)override{(void)processRealtime(b.interleaved.data(),b.frames(),b.channels);}
    bool supportsRealtimeProcessing()const noexcept override{return true;}
    bool processRealtime(float*data,SampleIndex frames,int channels)noexcept override{
        if(!data||frames<=0||channels!=channels_||toneState_.size()!=static_cast<std::size_t>(channels_))return false;
        for(SampleIndex frame=0;frame<frames;++frame){
            const auto base=static_cast<std::size_t>(frame)*static_cast<std::size_t>(channels_);
            for(int channel=0;channel<channels_;++channel){
                const auto i=base+static_cast<std::size_t>(channel);
                float x=std::isfinite(data[i])?data[i]:0.0f;
                auto& low=toneState_[static_cast<std::size_t>(channel)];
                low=(1.0f-lpCoeff_)*x+lpCoeff_*low;
                const float shapedInput=(tone_>=0.0f)?(x+tone_*(x-low)):(x+(-tone_)*(low-x));
                const float d=shapedInput*drive_;
                float y=0.0f;
                if(mode_==1)y=(2.0f/kPi)*std::atan(d);
                else if(mode_==2)y=std::clamp(d,-1.0f,1.0f);
                else y=std::tanh(d);
                const float norm=mode_==1?std::max(0.0001f,(2.0f/kPi)*std::atan(drive_)):
                                 mode_==2?std::max(1.0f,drive_):std::max(0.0001f,std::tanh(drive_));
                data[i]=y/norm;
            }
        }
        return true;
    }
    void resetRealtime()noexcept override{std::fill(toneState_.begin(),toneState_.end(),0.0f);}
private:
    int channels_=2,mode_=0;
    float driveDb_=6.0f,tone_=0.0f,drive_=1.995f,lpCoeff_=0.5f;
    std::vector<float> toneState_;
};

class ReverbProcessor final:public IPluginProcessor{
public:
    explicit ReverbProcessor(const PluginInstance& plugin){
        room_=std::clamp(pluginParameterValue(plugin,"room",0.55f),0.0f,1.0f);
        decaySeconds_=std::clamp(pluginParameterValue(plugin,"decay_s",2.0f),0.15f,12.0f);
        damping_=std::clamp(pluginParameterValue(plugin,"damping",0.45f),0.0f,1.0f);
        preDelayMs_=std::clamp(pluginParameterValue(plugin,"predelay_ms",18.0f),0.0f,200.0f);
        width_=std::clamp(pluginParameterValue(plugin,"width",1.0f),0.0f,1.0f);
    }
    bool prepare(int sampleRate,int channels,std::string&error)override{
        if(sampleRate<8000||sampleRate>384000){error="FLOW Reverb sample rate is out of range";return false;}
        if(channels<1||channels>32){error="FLOW Reverb channel count is out of range";return false;}
        sampleRate_=sampleRate;channels_=channels;
        const float leftMs=35.0f+room_*70.0f;
        const float rightMs=42.0f+room_*73.0f;
        delayFramesL_=std::max(1,static_cast<int>(std::lround(leftMs*0.001f*sampleRate_)));
        delayFramesR_=std::max(1,static_cast<int>(std::lround(rightMs*0.001f*sampleRate_)));
        delayL_.assign(static_cast<std::size_t>(delayFramesL_),0.0f);
        delayR_.assign(static_cast<std::size_t>(delayFramesR_),0.0f);
        preDelayFrames_=std::max(0,static_cast<int>(std::lround(preDelayMs_*0.001f*sampleRate_)));
        preDelay_.assign(static_cast<std::size_t>(std::max(1,preDelayFrames_))*2,0.0f);
        writeL_=writeR_=preWrite_=0;dampL_=dampR_=0.0f;
        feedbackL_=std::clamp(std::pow(0.001f,(leftMs*0.001f)/decaySeconds_),0.0f,0.995f);
        feedbackR_=std::clamp(std::pow(0.001f,(rightMs*0.001f)/decaySeconds_),0.0f,0.995f);
        const float cutoff=std::clamp(18000.0f-damping_*16500.0f,1200.0f,18000.0f);
        dampCoeff_=std::exp(-2.0f*kPi*cutoff/static_cast<float>(sampleRate_));
        return true;
    }
    void setState(const std::string&)override{}
    std::string state()const override{return{};}
    void process(AudioBuffer&b)override{(void)processRealtime(b.interleaved.data(),b.frames(),b.channels);}
    bool supportsRealtimeProcessing()const noexcept override{return true;}
    bool processRealtime(float*data,SampleIndex frames,int channels)noexcept override{
        if(!data||frames<=0||channels!=channels_||delayL_.empty()||delayR_.empty())return false;
        for(SampleIndex frame=0;frame<frames;++frame){
            const auto base=static_cast<std::size_t>(frame)*static_cast<std::size_t>(channels_);
            const float inL=std::isfinite(data[base])?data[base]:0.0f;
            const float inR=channels_>1&&std::isfinite(data[base+1])?data[base+1]:inL;
            float pdL=inL,pdR=inR;
            if(preDelayFrames_>0){
                const auto pi=static_cast<std::size_t>(preWrite_)*2;
                pdL=preDelay_[pi];pdR=preDelay_[pi+1];
                preDelay_[pi]=inL;preDelay_[pi+1]=inR;
                if(++preWrite_>=preDelayFrames_)preWrite_=0;
            }
            const float tapL=delayL_[static_cast<std::size_t>(writeL_)];
            const float tapR=delayR_[static_cast<std::size_t>(writeR_)];
            dampL_=(1.0f-dampCoeff_)*tapL+dampCoeff_*dampL_;
            dampR_=(1.0f-dampCoeff_)*tapR+dampCoeff_*dampR_;
            delayL_[static_cast<std::size_t>(writeL_)]=pdL+dampR_*feedbackL_;
            delayR_[static_cast<std::size_t>(writeR_)]=pdR+dampL_*feedbackR_;
            if(++writeL_>=delayFramesL_)writeL_=0;
            if(++writeR_>=delayFramesR_)writeR_=0;
            const float mid=(tapL+tapR)*0.5f;
            const float side=(tapL-tapR)*0.5f*width_;
            data[base]=mid+side;
            if(channels_>1)data[base+1]=mid-side;
            for(int channel=2;channel<channels_;++channel)data[base+static_cast<std::size_t>(channel)]=mid;
        }
        return true;
    }
    void resetRealtime()noexcept override{
        std::fill(delayL_.begin(),delayL_.end(),0.0f);std::fill(delayR_.begin(),delayR_.end(),0.0f);std::fill(preDelay_.begin(),preDelay_.end(),0.0f);
        writeL_=writeR_=preWrite_=0;dampL_=dampR_=0.0f;
    }
private:
    int sampleRate_=48000,channels_=2,delayFramesL_=1,delayFramesR_=1,preDelayFrames_=0,writeL_=0,writeR_=0,preWrite_=0;
    float room_=0.55f,decaySeconds_=2.0f,damping_=0.45f,preDelayMs_=18.0f,width_=1.0f;
    float feedbackL_=0.7f,feedbackR_=0.7f,dampCoeff_=0.5f,dampL_=0.0f,dampR_=0.0f;
    std::vector<float> delayL_,delayR_,preDelay_;
};

class DelayProcessor final:public IPluginProcessor{
public:
    explicit DelayProcessor(const PluginInstance& plugin){
        timeMs_=std::clamp(pluginParameterValue(plugin,"time_ms",375.0f),1.0f,2000.0f);
        feedback_=std::clamp(pluginParameterValue(plugin,"feedback",0.35f),0.0f,0.95f);
        filterHz_=std::clamp(pluginParameterValue(plugin,"filter_hz",9000.0f),500.0f,20000.0f);
        pingPong_=pluginParameterValue(plugin,"ping_pong",0.0f)>=0.5f;
        syncBpm_=std::clamp(pluginParameterValue(plugin,"sync_bpm",0.0f),0.0f,300.0f);
        syncBeats_=std::clamp(pluginParameterValue(plugin,"sync_beats",0.5f),0.125f,4.0f);
    }
    bool prepare(int sampleRate,int channels,std::string&error)override{
        if(sampleRate<8000||sampleRate>384000){error="FLOW Delay sample rate is out of range";return false;}
        if(channels<1||channels>32){error="FLOW Delay channel count is out of range";return false;}
        sampleRate_=sampleRate;channels_=channels;
        const float effectiveMs=syncBpm_>=20.0f?(60000.0f/syncBpm_)*syncBeats_:timeMs_;
        delayFrames_=std::max(1,static_cast<int>(std::lround(std::clamp(effectiveMs,1.0f,2000.0f)*0.001f*sampleRate_)));
        delay_.assign(static_cast<std::size_t>(delayFrames_)*static_cast<std::size_t>(channels_),0.0f);
        filterState_.assign(static_cast<std::size_t>(channels_),0.0f);writeFrame_=0;
        filterCoeff_=std::exp(-2.0f*kPi*std::min(filterHz_,sampleRate_*0.45f)/static_cast<float>(sampleRate_));
        return true;
    }
    void setState(const std::string&)override{}
    std::string state()const override{return{};}
    void process(AudioBuffer&b)override{(void)processRealtime(b.interleaved.data(),b.frames(),b.channels);}
    bool supportsRealtimeProcessing()const noexcept override{return true;}
    bool processRealtime(float*data,SampleIndex frames,int channels)noexcept override{
        if(!data||frames<=0||channels!=channels_||delay_.empty())return false;
        for(SampleIndex frame=0;frame<frames;++frame){
            const auto base=static_cast<std::size_t>(frame)*static_cast<std::size_t>(channels_);
            const auto ringBase=static_cast<std::size_t>(writeFrame_)*static_cast<std::size_t>(channels_);
            float leftTap=delay_[ringBase],rightTap=channels_>1?delay_[ringBase+1]:leftTap;
            for(int channel=0;channel<channels_;++channel){
                const auto ci=static_cast<std::size_t>(channel);
                const float tap=delay_[ringBase+ci];
                auto& filtered=filterState_[ci];
                filtered=(1.0f-filterCoeff_)*tap+filterCoeff_*filtered;
                float feedbackSource=filtered;
                if(pingPong_&&channels_>1)feedbackSource=(channel==0?rightTap:leftTap);
                float input=std::isfinite(data[base+ci])?data[base+ci]:0.0f;
                delay_[ringBase+ci]=input+feedbackSource*feedback_;
                data[base+ci]=tap;
            }
            if(++writeFrame_>=delayFrames_)writeFrame_=0;
        }
        return true;
    }
    void resetRealtime()noexcept override{std::fill(delay_.begin(),delay_.end(),0.0f);std::fill(filterState_.begin(),filterState_.end(),0.0f);writeFrame_=0;}
private:
    int sampleRate_=48000,channels_=2,delayFrames_=1,writeFrame_=0;
    float timeMs_=375.0f,feedback_=0.35f,filterHz_=9000.0f,syncBpm_=0.0f,syncBeats_=0.5f,filterCoeff_=0.5f;
    bool pingPong_=false;
    std::vector<float> delay_,filterState_;
};

class ChorusProcessor final:public IPluginProcessor{
public:
    explicit ChorusProcessor(const PluginInstance& plugin){
        rateHz_=std::clamp(pluginParameterValue(plugin,"rate_hz",0.8f),0.05f,10.0f);
        depthMs_=std::clamp(pluginParameterValue(plugin,"depth_ms",6.0f),0.0f,20.0f);
        baseMs_=std::clamp(pluginParameterValue(plugin,"base_ms",12.0f),1.0f,30.0f);
        feedback_=std::clamp(pluginParameterValue(plugin,"feedback",0.08f),-0.90f,0.90f);
        width_=std::clamp(pluginParameterValue(plugin,"width",1.0f),0.0f,1.0f);
    }
    bool prepare(int sampleRate,int channels,std::string&error)override{
        if(sampleRate<8000||sampleRate>384000){error="FLOW Chorus sample rate is out of range";return false;}
        if(channels<1||channels>32){error="FLOW Chorus channel count is out of range";return false;}
        sampleRate_=sampleRate;channels_=channels;
        maxDelayFrames_=std::max(4,static_cast<int>(std::ceil((baseMs_+depthMs_+2.0f)*0.001f*sampleRate_)));
        delay_.assign(static_cast<std::size_t>(maxDelayFrames_)*static_cast<std::size_t>(channels_),0.0f);
        writeFrame_=0;phase_=0.0f;phaseIncrement_=2.0f*kPi*rateHz_/static_cast<float>(sampleRate_);
        return true;
    }
    void setState(const std::string&)override{}
    std::string state()const override{return{};}
    void process(AudioBuffer&b)override{(void)processRealtime(b.interleaved.data(),b.frames(),b.channels);}
    bool supportsRealtimeProcessing()const noexcept override{return true;}
    bool processRealtime(float*data,SampleIndex frames,int channels)noexcept override{
        if(!data||frames<=0||channels!=channels_||delay_.empty())return false;
        for(SampleIndex frame=0;frame<frames;++frame){
            const auto base=static_cast<std::size_t>(frame)*static_cast<std::size_t>(channels_);
            for(int channel=0;channel<channels_;++channel){
                const float phaseOffset=(channels_>1&&channel==1)?kPi*width_:0.0f;
                const float mod=0.5f+0.5f*std::sin(phase_+phaseOffset);
                const float delayFrames=(baseMs_+depthMs_*mod)*0.001f*sampleRate_;
                float read=static_cast<float>(writeFrame_)-delayFrames;
                while(read<0.0f)read+=static_cast<float>(maxDelayFrames_);
                const int i0=static_cast<int>(read)%maxDelayFrames_;
                const int i1=(i0+1)%maxDelayFrames_;
                const float frac=read-static_cast<float>(static_cast<int>(read));
                const auto cidx=static_cast<std::size_t>(channel);
                const float a=delay_[static_cast<std::size_t>(i0)*channels_+cidx];
                const float b=delay_[static_cast<std::size_t>(i1)*channels_+cidx];
                const float wet=a+(b-a)*frac;
                float input=std::isfinite(data[base+cidx])?data[base+cidx]:0.0f;
                delay_[static_cast<std::size_t>(writeFrame_)*channels_+cidx]=input+wet*feedback_;
                data[base+cidx]=wet;
            }
            if(++writeFrame_>=maxDelayFrames_)writeFrame_=0;
            phase_+=phaseIncrement_;if(phase_>=2.0f*kPi)phase_-=2.0f*kPi;
        }
        return true;
    }
    void resetRealtime()noexcept override{std::fill(delay_.begin(),delay_.end(),0.0f);writeFrame_=0;phase_=0.0f;}
private:
    int sampleRate_=48000,channels_=2,maxDelayFrames_=4,writeFrame_=0;
    float rateHz_=0.8f,depthMs_=6.0f,baseMs_=12.0f,feedback_=0.08f,width_=1.0f,phase_=0.0f,phaseIncrement_=0.0f;
    std::vector<float> delay_;
};

class GateProcessor final:public IPluginProcessor{
public:
    explicit GateProcessor(const PluginInstance& plugin){
        thresholdDb_=std::clamp(pluginParameterValue(plugin,"threshold_db",-36.0f),-80.0f,0.0f);
        rangeDb_=std::clamp(pluginParameterValue(plugin,"range_db",-60.0f),-80.0f,0.0f);
        attackMs_=std::clamp(pluginParameterValue(plugin,"attack_ms",2.0f),0.1f,100.0f);
        holdMs_=std::clamp(pluginParameterValue(plugin,"hold_ms",25.0f),0.0f,500.0f);
        releaseMs_=std::clamp(pluginParameterValue(plugin,"release_ms",100.0f),5.0f,2000.0f);
    }
    bool prepare(int sampleRate,int channels,std::string&error)override{
        if(sampleRate<8000||sampleRate>384000){error="FLOW Gate sample rate is out of range";return false;}
        if(channels<1||channels>32){error="FLOW Gate channel count is out of range";return false;}
        sampleRate_=sampleRate;channels_=channels;threshold_=std::pow(10.0f,thresholdDb_/20.0f);floorGain_=std::pow(10.0f,rangeDb_/20.0f);
        attackCoeff_=std::exp(-1.0f/(std::max(0.0001f,attackMs_*0.001f)*sampleRate_));
        releaseCoeff_=std::exp(-1.0f/(std::max(0.001f,releaseMs_*0.001f)*sampleRate_));
        holdSamples_=std::max(0,static_cast<int>(std::lround(holdMs_*0.001f*sampleRate_)));holdRemaining_=0;gain_=floorGain_;
        return true;
    }
    void setState(const std::string&)override{}
    std::string state()const override{return{};}
    void process(AudioBuffer&b)override{(void)processRealtime(b.interleaved.data(),b.frames(),b.channels);}
    bool supportsRealtimeProcessing()const noexcept override{return true;}
    bool processRealtime(float*data,SampleIndex frames,int channels)noexcept override{
        if(!data||frames<=0||channels!=channels_)return false;
        for(SampleIndex frame=0;frame<frames;++frame){
            const auto base=static_cast<std::size_t>(frame)*static_cast<std::size_t>(channels_);
            float peak=0.0f;for(int ch=0;ch<channels_;++ch)peak=std::max(peak,std::abs(data[base+static_cast<std::size_t>(ch)]));
            float target=floorGain_;
            if(peak>=threshold_){holdRemaining_=holdSamples_;target=1.0f;}
            else if(holdRemaining_>0){--holdRemaining_;target=1.0f;}
            const float coeff=target>gain_?attackCoeff_:releaseCoeff_;
            gain_=coeff*gain_+(1.0f-coeff)*target;
            for(int ch=0;ch<channels_;++ch)data[base+static_cast<std::size_t>(ch)]*=gain_;
        }
        if(gain_<1.0e-20f)gain_=0.0f;return true;
    }
    void resetRealtime()noexcept override{gain_=floorGain_;holdRemaining_=0;}
private:
    int sampleRate_=48000,channels_=2,holdSamples_=0,holdRemaining_=0;
    float thresholdDb_=-36.0f,rangeDb_=-60.0f,attackMs_=2.0f,holdMs_=25.0f,releaseMs_=100.0f;
    float threshold_=0.0158f,floorGain_=0.001f,attackCoeff_=0.0f,releaseCoeff_=0.0f,gain_=0.001f;
};

class UtilityProcessor final:public IPluginProcessor{
public:
    explicit UtilityProcessor(const PluginInstance& plugin){
        gainDb_=std::clamp(pluginParameterValue(plugin,"gain_db",0.0f),-24.0f,24.0f);
        polarity_=pluginParameterValue(plugin,"polarity",0.0f)>=0.5f;
        mono_=pluginParameterValue(plugin,"mono",0.0f)>=0.5f;
        swap_=pluginParameterValue(plugin,"swap",0.0f)>=0.5f;
        balance_=std::clamp(pluginParameterValue(plugin,"balance",0.0f),-1.0f,1.0f);
        width_=std::clamp(pluginParameterValue(plugin,"width",1.0f),0.0f,2.0f);
    }
    bool prepare(int,int channels,std::string&error)override{if(channels<1||channels>32){error="FLOW Utility channel count is out of range";return false;}channels_=channels;gain_=std::pow(10.0f,gainDb_/20.0f);return true;}
    void setState(const std::string&)override{}
    std::string state()const override{return{};}
    void process(AudioBuffer&b)override{(void)processRealtime(b.interleaved.data(),b.frames(),b.channels);}
    bool supportsRealtimeProcessing()const noexcept override{return true;}
    bool processRealtime(float*data,SampleIndex frames,int channels)noexcept override{
        if(!data||frames<=0||channels!=channels_)return false;
        const float polarityGain=polarity_?-1.0f:1.0f;
        const float leftBal=balance_>0.0f?1.0f-balance_:1.0f;
        const float rightBal=balance_<0.0f?1.0f+balance_:1.0f;
        for(SampleIndex frame=0;frame<frames;++frame){
            const auto base=static_cast<std::size_t>(frame)*static_cast<std::size_t>(channels_);
            if(channels_==1){data[base]*=gain_*polarityGain;continue;}
            float l=data[base],r=data[base+1];if(swap_)std::swap(l,r);
            if(mono_)l=r=(l+r)*0.5f;
            else{const float m=(l+r)*0.5f,s=(l-r)*0.5f*width_;l=m+s;r=m-s;}
            data[base]=l*gain_*polarityGain*leftBal;data[base+1]=r*gain_*polarityGain*rightBal;
            for(int ch=2;ch<channels_;++ch)data[base+static_cast<std::size_t>(ch)]*=gain_*polarityGain;
        }
        return true;
    }
private:
    int channels_=2;float gainDb_=0.0f,balance_=0.0f,width_=1.0f,gain_=1.0f;bool polarity_=false,mono_=false,swap_=false;
};

} // namespace

std::unique_ptr<IPluginProcessor> createBuiltinPluginProcessor(const PluginInstance&plugin,std::string&error){
    error.clear();
    if(plugin.format!="builtin"){error="Plugin is not a FLOWDAW builtin";return{};}
    if(plugin.identifier=="flow.gain")return std::make_unique<GainProcessor>(std::clamp(pluginParameterValue(plugin,"gain",1.0f),0.0f,4.0f));
    if(plugin.identifier=="flow.softclip")return std::make_unique<SoftClipProcessor>(pluginParameterValue(plugin,"drive",0.25f));
    if(plugin.identifier=="flow.width")return std::make_unique<WidthProcessor>(pluginParameterValue(plugin,"width",1.0f));
    if(plugin.identifier=="flow.eq")return std::make_unique<EqProcessor>(plugin);
    if(plugin.identifier=="flow.compressor")return std::make_unique<CompressorProcessor>(plugin);
    if(plugin.identifier=="flow.limiter")return std::make_unique<LimiterProcessor>(plugin);
    if(plugin.identifier=="flow.saturator")return std::make_unique<SaturatorProcessor>(plugin);
    if(plugin.identifier=="flow.reverb")return std::make_unique<ReverbProcessor>(plugin);
    if(plugin.identifier=="flow.delay")return std::make_unique<DelayProcessor>(plugin);
    if(plugin.identifier=="flow.chorus")return std::make_unique<ChorusProcessor>(plugin);
    if(plugin.identifier=="flow.gate")return std::make_unique<GateProcessor>(plugin);
    if(plugin.identifier=="flow.utility")return std::make_unique<UtilityProcessor>(plugin);
    error="Unknown FLOWDAW builtin plugin: "+plugin.identifier;
    return{};
}

} // namespace flowdaw
