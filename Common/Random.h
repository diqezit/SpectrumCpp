#ifndef SPECTRUM_CPP_RANDOM_H
#define SPECTRUM_CPP_RANDOM_H

#include "Common.h"
#include <random>

namespace Spectrum {
    namespace Utils {

        class Random {
        public:
            static Random& Instance();

            float Float(float min = 0.0f, float max = 1.0f);
            int   Int(int min, int max);
            bool  Bool(float probability = 0.5f);

        private:
            Random();
            std::mt19937 m_generator;
            std::uniform_real_distribution<float> m_unitDist;
        };

    } // namespace Utils
} // namespace Spectrum

#endif