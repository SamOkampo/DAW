#include "flowdaw/Wav.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <cmath>
#include <fstream>
#include <stdexcept>

namespace flowdaw {
namespace {
template <typename T> T readLE(std::istream& s) {
    T v{}; s.read(reinterpret_cast<char*>(&v), sizeof(T));
    if (!s) throw std::runtime_error("Unexpected end of WAV");
    return v;
}
void writeU16(std::ostream& s, std::uint16_t v) { s.write(reinterpret_cast<const char*>(&v), 2); }
void writeU32(std::ostream& s, std::uint32_t v) { s.write(reinterpret_cast<const char*>(&v), 4); }

std::uint32_t xorshift(std::uint32_t& state) noexcept {
    state^=state<<13;state^=state>>17;state^=state<<5;return state;
}
double uniform01(std::uint32_t& state) noexcept {
    return static_cast<double>(xorshift(state))/static_cast<double>(0xffffffffu);
}
double tpdf(std::uint32_t& state) noexcept {
    return uniform01(state)-uniform01(state);
}
void validateWriteBuffer(const AudioBuffer& audio){
    if(audio.channels<1||audio.channels>2||audio.sampleRate<=0)throw std::invalid_argument("Invalid audio buffer");
}
}
AudioBuffer WavFile::read(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open WAV: " + path.string());
    char riff[4]{}; f.read(riff, 4); (void)readLE<std::uint32_t>(f); char wave[4]{}; f.read(wave, 4);
    if (std::string(riff,4)!="RIFF" || std::string(wave,4)!="WAVE") throw std::runtime_error("Not a RIFF/WAVE file");
    std::uint16_t fmtTag=0, channels=0, bits=0; std::uint32_t sampleRate=0; std::vector<std::uint8_t> data;
    while (f && !f.eof()) {
        char id[4]{}; f.read(id,4); if (f.gcount()!=4) break;
        auto size = readLE<std::uint32_t>(f);
        const std::string sid(id,4);
        if (sid=="fmt ") {
            fmtTag=readLE<std::uint16_t>(f); channels=readLE<std::uint16_t>(f); sampleRate=readLE<std::uint32_t>(f);
            (void)readLE<std::uint32_t>(f); (void)readLE<std::uint16_t>(f); bits=readLE<std::uint16_t>(f);
            if (size>16) f.seekg(size-16, std::ios::cur);
        } else if (sid=="data") {
            data.resize(size); f.read(reinterpret_cast<char*>(data.data()), size);
        } else f.seekg(size, std::ios::cur);
        if (size & 1U) f.seekg(1, std::ios::cur);
    }
    if (!channels || !sampleRate || data.empty()) throw std::runtime_error("WAV missing fmt/data");
    if (channels>2) throw std::runtime_error("Phase 0 supports mono/stereo WAV");
    const std::size_t bytesPerSample=bits/8; if (!bytesPerSample) throw std::runtime_error("Invalid WAV bit depth");
    const std::size_t sampleCount=data.size()/bytesPerSample;
    AudioBuffer out; out.sampleRate=static_cast<int>(sampleRate); out.channels=channels; out.interleaved.resize(sampleCount);
    for (std::size_t i=0;i<sampleCount;++i) {
        const auto* p=data.data()+i*bytesPerSample; float v=0.0f;
        if (fmtTag==1 && bits==16) { std::int16_t x{}; std::memcpy(&x,p,2); v=static_cast<float>(x)/32768.0f; }
        else if (fmtTag==1 && bits==24) { std::int32_t x=(static_cast<std::int32_t>(p[0]) | (static_cast<std::int32_t>(p[1])<<8) | (static_cast<std::int32_t>(p[2])<<16)); if (x&0x800000) x|=~0xffffff; v=static_cast<float>(x)/8388608.0f; }
        else if (fmtTag==1 && bits==32) { std::int32_t x{}; std::memcpy(&x,p,4); v=static_cast<float>(static_cast<double>(x)/2147483648.0); }
        else if (fmtTag==3 && bits==32) std::memcpy(&v,p,4);
        else throw std::runtime_error("Unsupported WAV encoding (supports PCM16/24/32 and float32)");
        out.interleaved[i]=std::clamp(v,-1.0f,1.0f);
    }
    return out;
}
void WavFile::writeFloat32(const std::filesystem::path& path, const AudioBuffer& audio) {
    validateWriteBuffer(audio);
    std::ofstream f(path,std::ios::binary); if(!f) throw std::runtime_error("Cannot write WAV");
    const std::uint32_t dataBytes=static_cast<std::uint32_t>(audio.interleaved.size()*sizeof(float));
    f.write("RIFF",4); writeU32(f,36+dataBytes); f.write("WAVEfmt ",8); writeU32(f,16); writeU16(f,3); writeU16(f,static_cast<std::uint16_t>(audio.channels));
    writeU32(f,static_cast<std::uint32_t>(audio.sampleRate)); writeU32(f,static_cast<std::uint32_t>(audio.sampleRate*audio.channels*4)); writeU16(f,static_cast<std::uint16_t>(audio.channels*4)); writeU16(f,32);
    f.write("data",4); writeU32(f,dataBytes); f.write(reinterpret_cast<const char*>(audio.interleaved.data()),dataBytes);
}

void WavFile::writePcm24(const std::filesystem::path& path,const AudioBuffer& audio,bool tpdfDither){
    validateWriteBuffer(audio);
    std::ofstream f(path,std::ios::binary);if(!f)throw std::runtime_error("Cannot write WAV");
    constexpr double scale=8388608.0;
    const std::uint32_t dataBytes=static_cast<std::uint32_t>(audio.interleaved.size()*3);
    f.write("RIFF",4);writeU32(f,36+dataBytes);f.write("WAVEfmt ",8);writeU32(f,16);writeU16(f,1);writeU16(f,static_cast<std::uint16_t>(audio.channels));
    writeU32(f,static_cast<std::uint32_t>(audio.sampleRate));writeU32(f,static_cast<std::uint32_t>(audio.sampleRate*audio.channels*3));writeU16(f,static_cast<std::uint16_t>(audio.channels*3));writeU16(f,24);
    f.write("data",4);writeU32(f,dataBytes);
    std::uint32_t rng=0x6d2b79f5u;
    for(float sample:audio.interleaved){
        double q=std::clamp(static_cast<double>(sample),-1.0,1.0-1.0/scale)*scale;
        if(tpdfDither)q+=tpdf(rng);
        const auto value=static_cast<std::int32_t>(std::clamp<long long>(std::llround(q),-8388608LL,8388607LL));
        const std::uint32_t u=static_cast<std::uint32_t>(value)&0x00ffffffu;
        const std::array<char,3> bytes{static_cast<char>(u&0xffu),static_cast<char>((u>>8)&0xffu),static_cast<char>((u>>16)&0xffu)};
        f.write(bytes.data(),3);
    }
}

void WavFile::writePcm16(const std::filesystem::path& path,const AudioBuffer& audio,bool tpdfDither){
    validateWriteBuffer(audio);
    std::ofstream f(path,std::ios::binary);if(!f)throw std::runtime_error("Cannot write WAV");
    constexpr double scale=32768.0;
    const std::uint32_t dataBytes=static_cast<std::uint32_t>(audio.interleaved.size()*2);
    f.write("RIFF",4);writeU32(f,36+dataBytes);f.write("WAVEfmt ",8);writeU32(f,16);writeU16(f,1);writeU16(f,static_cast<std::uint16_t>(audio.channels));
    writeU32(f,static_cast<std::uint32_t>(audio.sampleRate));writeU32(f,static_cast<std::uint32_t>(audio.sampleRate*audio.channels*2));writeU16(f,static_cast<std::uint16_t>(audio.channels*2));writeU16(f,16);
    f.write("data",4);writeU32(f,dataBytes);
    std::uint32_t rng=0x243f6a88u;
    for(float sample:audio.interleaved){
        double q=std::clamp(static_cast<double>(sample),-1.0,1.0-1.0/scale)*scale;
        if(tpdfDither)q+=tpdf(rng);
        const auto value=static_cast<std::int16_t>(std::clamp<long long>(std::llround(q),-32768LL,32767LL));
        writeU16(f,static_cast<std::uint16_t>(value));
    }
}
}
