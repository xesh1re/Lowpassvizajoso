#pragma once

// Matematica de filtros sin dependencias de JUCE, compartida entre el
// procesador de audio y la pantalla que dibuja la curva.

#include <algorithm>
#include <cmath>
#include <complex>

namespace FilterMath
{
    constexpr double kPi = 3.14159265358979323846;

    struct Coeffs
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    };

    // Biquad en forma transpuesta II
    struct Biquad
    {
        Coeffs c;
        float z1 = 0.0f, z2 = 0.0f;

        void reset() { z1 = z2 = 0.0f; }

        float process (float x)
        {
            const float y = c.b0 * x + z1;
            z1 = c.b1 * x - c.a1 * y + z2;
            z2 = c.b2 * x - c.a2 * y;
            return y;
        }
    };

    inline Coeffs makeLowpass (double fc, double sr, double q)
    {
        fc = std::min (std::max (fc, 10.0), 0.49 * sr);

        const double w0 = 2.0 * kPi * fc / sr;
        const double cosw = std::cos (w0);
        const double sinw = std::sin (w0);
        const double alpha = sinw / (2.0 * q);
        const double a0 = 1.0 + alpha;

        Coeffs c;
        c.b0 = (float) (((1.0 - cosw) * 0.5) / a0);
        c.b1 = (float) ((1.0 - cosw) / a0);
        c.b2 = c.b0;
        c.a1 = (float) ((-2.0 * cosw) / a0);
        c.a2 = (float) ((1.0 - alpha) / a0);
        return c;
    }

    // Shelf de graves (RBJ, pendiente S = 1)
    inline Coeffs makeLowShelf (double f0, double sr, double gainDb)
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w0 = 2.0 * kPi * std::min (f0, 0.49 * sr) / sr;
        const double cosw = std::cos (w0);
        const double sinw = std::sin (w0);
        const double sqrtA = std::sqrt (A);
        const double alpha = sinw * 0.5 * std::sqrt (2.0);

        const double b0 = A * ((A + 1.0) - (A - 1.0) * cosw + 2.0 * sqrtA * alpha);
        const double b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosw);
        const double b2 = A * ((A + 1.0) - (A - 1.0) * cosw - 2.0 * sqrtA * alpha);
        const double a0 = (A + 1.0) + (A - 1.0) * cosw + 2.0 * sqrtA * alpha;
        const double a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cosw);
        const double a2 = (A + 1.0) + (A - 1.0) * cosw - 2.0 * sqrtA * alpha;

        Coeffs c;
        c.b0 = (float) (b0 / a0);
        c.b1 = (float) (b1 / a0);
        c.b2 = (float) (b2 / a0);
        c.a1 = (float) (a1 / a0);
        c.a2 = (float) (a2 / a0);
        return c;
    }

    // |H(f)| del biquad
    inline double magnitude (const Coeffs& c, double freq, double sr)
    {
        const double w = 2.0 * kPi * freq / sr;
        const std::complex<double> z1 = std::polar (1.0, -w);
        const std::complex<double> z2 = std::polar (1.0, -2.0 * w);

        const std::complex<double> num = (double) c.b0 + (double) c.b1 * z1 + (double) c.b2 * z2;
        const std::complex<double> den = 1.0 + (double) c.a1 * z1 + (double) c.a2 * z2;

        return std::abs (num / den);
    }

    // Resonancia 0..1 -> Q (0.5 = sin resonancia, 8 = muy resonante)
    inline float resoToQ (float r)
    {
        return 0.5f + 7.5f * r * r;
    }
}
