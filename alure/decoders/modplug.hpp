#ifndef DECODERS_MODPLUG_HPP
#define DECODERS_MODPLUG_HPP

#include "alure2.h"

namespace alure {

class ModPlugDecoderFactory final : public DecoderFactory {
public:
    SharedPtr<Decoder> createDecoder(UniquePtr<std::istream> &file) noexcept override;
};

} // namespace alure

#endif /* DECODERS_MODPLUG_HPP */
