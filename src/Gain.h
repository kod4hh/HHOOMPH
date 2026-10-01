#pragma once
#include <cmath>
#include <juce_dsp/juce_dsp.h>

class ParameterSmoother
{
public:
    ParameterSmoother(float smoothingTimeMs, float sampleRate)
    {
        float dt = 1.0f / sampleRate;
        float tau = smoothingTimeMs * 0.001f;
        alpha = dt / (tau + dt);
        smoothedValue = 0.0f;
        initialized = false;
    }

    float process(float input)
    {
        if (!initialized)
        {
            smoothedValue = input;
            initialized = true;
        }
        else
        {
            smoothedValue += alpha * (input - smoothedValue);
        }
        return smoothedValue;
    }

private:
    float alpha;
    float smoothedValue;
    bool initialized;
};

// Simple DC blocker (high-pass ~20Hz)
class DCBlocker
{
public:
    DCBlocker() : x1(0.0f), y1(0.0f) {}

    void setCoefficient(float sampleRate)
    {
        // ~20Hz high-pass coefficient
        R = 1.0f - (126.0f / sampleRate);
    }

    float process(float input)
    {
        // y[n] = x[n] - x[n-1] + R * y[n-1]
        float output = input - x1 + R * y1;
        x1 = input;
        y1 = output;
        return output;
    }

    void reset()
    {
        x1 = 0.0f;
        y1 = 0.0f;
    }

private:
    float R = 0.995f;
    float x1, y1;
};

// Simple utility functions for DSP
namespace DSPUtils
{
    inline float lerp(float a, float b, float t)
    {
        return a + t * (b - a);
    }

    inline float tau2pole(float tau, float sampleRate)
    {
        return std::exp(-1.0f / (tau * sampleRate));
    }

    // Soft clipper - keeps signal below ceiling with smooth knee
    inline float softClip(float input, float ceiling = 1.0f)
    {
        float x = input / ceiling;
        if (x > 1.0f)
            return ceiling;
        else if (x < -1.0f)
            return -ceiling;
        else
            return ceiling * std::tanh(x);
    }
}

class Gain {
public:
  Gain(float sample_rate, int samples_per_block, int num_channels, float default_gain_);
  ~Gain();
  void process(float *const *buffer, const int numSamples, const int numChannels, const float gain);
  void setState(const float gain);
  int getLatencySamples() const noexcept;

private:
  float smooth_gain;
  const float smooth_pole, default_gain;

  // Parameter smoothing to prevent zipper noise
  ParameterSmoother gainSmoother;

  // v0.93: 2x oversampling to reduce aliasing
  static constexpr int oversamplingStages = 1; // 1 stage = 2x oversampling
  std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;

  // v0.93b: DC blockers per channel (removes sub-20Hz rumble)
  DCBlocker dcBlockerL;
  DCBlocker dcBlockerR;

  // Harmonic processing functions
  float processOddHarmonics(float input, float drive);
  float processEvenHarmonics(float input, float drive);
  float calculateCompensation(float drive);
};
