#include "modplug.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

#include <libmodplug/modplug.h>

namespace alure {

class ModPlugDecoder final : public Decoder {
    UniquePtr<std::istream> mFile;
    ModPlugFile *mMod;
    ALuint mFrequency;
    ChannelConfig mChannels;

public:
    ModPlugDecoder(UniquePtr<std::istream> file, ModPlugFile *mod, ALuint frequency, ChannelConfig channels) noexcept
      : mFile(std::move(file)), mMod(mod), mFrequency(frequency), mChannels(channels)
    { }
    ~ModPlugDecoder() override { if(mMod) ModPlug_Unload(mMod); }

    ALuint getFrequency() const noexcept override { return mFrequency; }
    ChannelConfig getChannelConfig() const noexcept override { return mChannels; }
    SampleType getSampleType() const noexcept override { return SampleType::Int16; }

    uint64_t getLength() const noexcept override
    {
        int ms = ModPlug_GetLength(mMod);
        if(ms <= 0) return 0;
        return (uint64_t)ms * mFrequency / 1000;
    }

    bool seek(uint64_t pos) noexcept override
    {
        ModPlug_Seek(mMod, (int)(pos * 1000 / mFrequency));
        return true;
    }

    std::pair<uint64_t,uint64_t> getLoopPoints() const noexcept override
    {
        return {0, 0};
    }

    ALuint read(ALvoid *ptr, ALuint count) noexcept override
    {
        int channels = (mChannels == ChannelConfig::Stereo) ? 2 : 1;
        int bytes = (int)count * channels * (int)sizeof(int16_t);
        int got = ModPlug_Read(mMod, ptr, bytes);
        if(got <= 0) return 0;
        return (ALuint)got / (channels * (int)sizeof(int16_t));
    }
};

SharedPtr<Decoder> ModPlugDecoderFactory::createDecoder(UniquePtr<std::istream> &file) noexcept
{
    if(!file) return nullptr;
    file->clear();
    file->seekg(0, std::ios::end);
    std::streamoff size = file->tellg();
    if(size <= 0 || size > 32 * 1024 * 1024)
        return nullptr;
    file->seekg(0, std::ios::beg);
    std::vector<char> data((size_t)size);
    file->read(data.data(), size);
    if(file->gcount() != size)
        return nullptr;

    static bool settingsDone = false;
    if(!settingsDone)
    {
        ModPlug_Settings settings;
        ModPlug_GetSettings(&settings);
        settings.mChannels = 2;
        settings.mBits = 16;
        settings.mFrequency = 44100;
        settings.mLoopCount = 0;
        ModPlug_SetSettings(&settings);
        settingsDone = true;
    }

    ModPlugFile *mod = ModPlug_Load(data.data(), (int)data.size());
    if(!mod)
        return nullptr;

    file->clear();
    file->seekg(0);
    return MakeShared<ModPlugDecoder>(std::move(file), mod, 44100, ChannelConfig::Stereo);
}

} // namespace alure
