#ifndef SPECTRUM_CPP_TEMPLATE_UTILS_H
#define SPECTRUM_CPP_TEMPLATE_UTILS_H

#include "Common.h"
#include <type_traits>

namespace Spectrum {
    namespace Utils {

        template<typename TEnum>
        [[nodiscard]] inline TEnum CycleEnum(TEnum current, int direction) {
            static_assert(std::is_enum_v<TEnum>);
            using TUnderlying = std::underlying_type_t<TEnum>;

            const auto count = static_cast<TUnderlying>(TEnum::Count);
            auto next = static_cast<TUnderlying>(current) + direction;

            return static_cast<TEnum>((next % count + count) % count);
        }

    } // namespace Utils
} // namespace Spectrum

#endif