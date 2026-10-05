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

} // namespace

std::unique_ptr<IPluginProcessor> createBuiltinPluginProcessor(const PluginInstance&plugin,std::string&error){
    error.clear();
    if(plugin.format!="builtin"){error="Plugin is not a FLOWDAW builtin";return{};}
    if(plugin.identifier=="flow.gain")return std::make_unique<GainProcessor>(std::clamp(pluginParameterValue(plugin,"gain",1.0f),0.0f,4.0f));
    if(plugin.identifier=="flow.softclip")return std::make_unique<SoftClipProcessor>(pluginParameterValue(plugin,"drive",0.25f));
    if(plugin.identifier=="flow.width")return std::make_unique<WidthProcessor>(pluginParameterValue(plugin,"width",1.0f));
    if(plugin.identifier=="flow.eq")return std::make_unique<EqProcessor>(plugin);
    if(plugin.identifier=="flow.compressor")return std::make_unique<CompressorProcessor>(plugin);
    error="Unknown FLOWDAW builtin plugin: "+plugin.identifier;
    return{};
}

} // namespace flowdaw
