#pragma once

#include <pch.h>

// Forward declarations
class Gain;
class HHOOMPEditor;

struct Params : chowdsp::ParamHolder
{
    chowdsp::FloatParameter::Ptr gain;
    chowdsp::BoolParameter::Ptr bypass; // True bypass parameter

    Params()
    {
        add (gain = chowdsp::FloatParameter::Ptr (new chowdsp::FloatParameter (
            "Gain",
            "Saturation + Tilt Effect",
            juce::NormalisableRange<float> { 0.0f, 10.0f, 0.01f },
            5.0f,
            [](float value) { return juce::String(value, 1); },  // Value to text
            [](const juce::String& text) { return text.getFloatValue(); }  // Text to value
        )));

        add (bypass = chowdsp::BoolParameter::Ptr (new chowdsp::BoolParameter (
            "Bypass",
            "Bypass Effect",
            false // default: effect ON
        )));
    }
};

using State = chowdsp::PluginStateImpl<Params>;

class HHOOMP : public chowdsp::PluginBase<State>
{
public:
    HHOOMP();

    void prepareToPlay (double sample_rate, int samples_per_block) override;
    void releaseResources() override {}
    void processAudioBlock (juce::AudioBuffer<float>& buffer) override;

    juce::AudioProcessorEditor* createEditor() override;

    // Public access to state for editor
    State& getState() { return state; }

private:
    std::unique_ptr<Gain> gainProcessor;
    std::atomic<bool> shouldSnapSmoothedParams { true };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HHOOMP)
};
