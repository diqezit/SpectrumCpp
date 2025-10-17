#include "Random.h"
#include "MathUtils.h"

namespace Spectrum {
    namespace Utils {

        Random& Random::Instance() {
            static Random inst;
            return inst;
        }

        Random::Random()
            : m_generator(std::random_device{}())
            , m_unitDist(0.0f, 1.0f) {
        }

        float Random::Float(float min, float max) {
            if (max < min) {
                std::swap(min, max);
            }
            return min + m_unitDist(m_generator) * (max - min);
        }

        int Random::Int(int min, int max) {
            if (max < min) {
                std::swap(min, max);
            }
            std::uniform_int_distribution<int> dist(min, max);
            return dist(m_generator);
        }

        bool Random::Bool(float probability) {
            const float p = Saturate(probability);
            return m_unitDist(m_generator) < p;
        }

    } // namespace Utils
} // namespace Spectrum