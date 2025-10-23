#ifndef SPECTRUM_CPP_RANDOM_H
#define SPECTRUM_CPP_RANDOM_H

#include "Common/Common.h"
#include <random>

namespace Spectrum::Helpers::Utils {

    class Random {
    public:
        static Random& Instance();

        float Float(float min = 0.0f, float max = 1.0f);
        int   Int(int min, int max);
        bool  Bool(float probability = 0.5f);
        Random();

    private:
        std::mt19937 m_generator;
        std::uniform_real_distribution<float> m_unitDist;
    };

} // namespace Spectrum::Helpers::Utils

#endif // SPECTRUM_CPP_RANDOM_H