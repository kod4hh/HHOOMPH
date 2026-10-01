#include "Gain.h"
#include <juce_dsp/juce_dsp.h> // redundant but bulletproof
#include <cmath>
#include <algorithm>

// v1.0: Inflator-style helper functions
static inline float clampf(float x, float lo, float hi) noexcept
{
    return std::max(lo, std::min(hi, x));
}

static inline float clamp1(float x) noexcept
{
    return clampf(x, -1.0f, 1.0f);
}

static inline float dbToGain(float db) noexcept
{
    return std::pow(10.0f, db / 20.0f);
}

// Memoryless inflator-ish shaper. curve in [-1..+1]
static inline float inflatorShape(float x, float curve) noexcept
{
    x = clamp1(x);
    const float c = clampf(curve, -1.0f, 1.0f);

    const float knee = 1.5f + 6.0f * std::max(0.0f, c); // +curve => denser
    const float yInflate = std::tanh(x * knee) / std::tanh(knee);

    const float edgeAmt = 0.2f + 0.8f * std::max(0.0f, -c); // -curve => edgier
    const float yEdge = (x + edgeAmt * x * x * x) / (1.0f + edgeAmt);

    const float t = 0.5f * (c + 1.0f);
    return (1.0f - t) * yEdge + t * yInflate;
}

Gain::Gain(float sample_rate, int samples_per_block, int num_channels, float default_gain_)
    : smooth_pole(DSPUtils::tau2pole(0.05f, sample_rate)),
      default_gain(default_gain_),
      gainSmoother(15.0f, sample_rate * 2.0f)  // Smoother runs at 2x rate due to oversampling
{
    // v0.93: Initialize 2x oversampling with half-band polyphase IIR anti-aliasing filter
    oversampler = std::make_unique<juce::dsp::Oversampling<float>>(
        num_channels,
        oversamplingStages,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true  // use maximum quality
    );
    oversampler->initProcessing(static_cast<size_t>(samples_per_block));
    oversampler->reset();

    // v0.93b: Initialize DC blockers
    dcBlockerL.setCoefficient(sample_rate);
    dcBlockerR.setCoefficient(sample_rate);

    setState(default_gain);
}

Gain::~Gain() {}

void Gain::process(float *const *buffer, const int numSamples, const int numChannels, const float gain) {
    // Normalize 0-10 input to 0-1 (HHOOMP.cpp passes 0-10, DSP expects 0-1)
    const float normalizedGain = std::max(0.0f, std::min(1.0f, gain / 10.0f));

    // Bypass if dial is below 1
    if (normalizedGain < 0.1f) {
        return; // Pass audio through unchanged
    }

    // Create audio block from buffer
    juce::dsp::AudioBlock<float> block(const_cast<float* const*>(buffer),
                                        static_cast<size_t>(numChannels),
                                        static_cast<size_t>(numSamples));

    // Upsample with anti-aliasing filter
    auto osBlock = oversampler->processSamplesUp(block);

    const int osNumSamples = static_cast<int>(osBlock.getNumSamples());
    const int osNumChannels = static_cast<int>(osBlock.getNumChannels());

    // v1.0: Inflator-style processing (clean restart)
    // Optional: Clip0-like behavior
    constexpr bool clip0 = true;

    for (int c = 0; c < osNumChannels; ++c)
    {
        float* channelData = osBlock.getChannelPointer(static_cast<size_t>(c));

        for (int i = 0; i < osNumSamples; ++i)
        {
            // Per-sample smoothing so "15ms" is actually 15ms
            const float smoothGainParam = gainSmoother.process(normalizedGain);
            
            const float tActive = clampf((smoothGainParam - 0.1f) / 0.9f, 0.0f, 1.0f);
            float effect = tActive;

            // Optional: make low dial more gradual
            // effect = effect * effect;

            // Bake curve into the dial: early = edgy harmonics, later = inflate density
            const float curve = -0.35f + 0.90f * effect;  // -0.35..+0.55

            // Bake "drive" into the dial so harmonics keep changing across the whole range
            const float strength = 1.0f + 6.0f * effect;  // 1..7 (tune: 5..9)

            // Delay trim onset until saturation is adding real energy
            constexpr float compStart = 0.25f;
            const float e = clampf((effect - compStart) / (1.0f - compStart), 0.0f, 1.0f);

            // Plateau-style loudness management (one knob)
            constexpr float K = 8.0f;
            constexpr float a = 7.0f;

            const float comp = (1.0f - std::exp(-a * e)) / (1.0f - std::exp(-a)); // 0..1
            const float trimDb = -K * comp;                                       // 0..-K dB
            const float postGain = dbToGain(trimDb);


            const float x = channelData[i];

            const float driven = clamp1(x * strength);
            const float shaped = inflatorShape(driven, curve);

            // Wet/dry = effect amount (monotonic)
            float y = (1.0f - effect) * x + effect * shaped;

            y *= postGain;

            if (clip0)
                y = DSPUtils::softClip(y, 1.0f);

            channelData[i] = y;
        }
    }

    // Downsample with anti-aliasing filter back into original buffer
    oversampler->processSamplesDown(block);

    // v0.93b: Apply DC blocker after downsampling (removes sub-20Hz rumble)
    for (int i = 0; i < numSamples; ++i) {
        if (numChannels > 0)
            buffer[0][i] = dcBlockerL.process(buffer[0][i]);
        if (numChannels > 1)
            buffer[1][i] = dcBlockerR.process(buffer[1][i]);
    }
}

void Gain::setState(const float gain) {
    // Normalize 0-10 input to 0-1
    smooth_gain = std::max(0.0f, std::min(1.0f, gain / 10.0f));
}

int Gain::getLatencySamples() const noexcept {
    return static_cast<int>(oversampler->getLatencyInSamples());
}

// ODD HARMONICS: Warm, tube-like (3rd, 5th)
float Gain::processOddHarmonics(float input, float drive) {
    // v0.93d: Further reduced preGain (was 4.0f → 2.8f → 1.8f)
    float preGain = 1.0f + (drive * 1.8f);
    float x = input * preGain;

    x = std::tanh(x);

    float x3 = x * x * x;
    float x5 = x3 * x * x;

    float thirdHarmonic = 1.0f + (drive * 1.5f);
    float fifthHarmonic = 0.6f + (drive * 1.0f);

    float output = x + thirdHarmonic * x3 + fifthHarmonic * x5;

    return std::tanh(output * 0.9f);
}

// EVEN HARMONICS: Bright, transistor-like (2nd, 4th)
float Gain::processEvenHarmonics(float input, float drive) {
    // v0.93d: Further reduced preGain (was 5.0f → 3.2f → 2.2f)
    float preGain = 1.0f + (drive * 2.2f);
    float x = input * preGain;

    float x2 = x * x;
    float x4 = x2 * x2;

    float secondHarmonic = 0.6f + (drive * 1.2f);
    float fourthHarmonic = 0.3f + (drive * 0.6f);

    float output;
    if (x > 0.0f) {
        output = x + secondHarmonic * x2 + fourthHarmonic * x4;
        output = std::tanh(output);
    } else {
        output = x + (secondHarmonic * 0.7f) * x2;
        output = std::tanh(output * 0.9f);
    }

    // v0.93c: Match odd harmonics final tanh for consistent RMS
    return std::tanh(output * 0.9f);
}

// Kept for reference but postGain is now used inline
float Gain::calculateCompensation(float drive) {
    return 1.0f; // Not used - loudness controlled by postGain in process()
}
