#include "HHOOMP.h"
#include "Gain.h"
#include "HHOOMPEditor.h"

HHOOMP::HHOOMP() = default;

void HHOOMP::prepareToPlay (double sample_rate, int samples_per_block)
{
    // Default at 5.0 (middle of 0-10 range)
    gainProcessor = std::make_unique<Gain>(float(sample_rate), samples_per_block,
                                          getTotalNumOutputChannels(), 5.0f);
    // v0.94: Report oversampling latency to host for PDC
    setLatencySamples(gainProcessor->getLatencySamples());
    shouldSnapSmoothedParams.store(true);
}

void HHOOMP::processAudioBlock (juce::AudioBuffer<float>& buffer)
{
    juce::ScopedNoDenormals noDenormals;

    // Hard bypass if toggle is off - audio passes through untouched
    if (state.params.bypass->get())
        return;

    // Get parameter value (0-10 range) - pass directly to Gain
    auto requested_gain = state.params.gain->get();

    // Force parameter update if needed (for state loading)
    if (shouldSnapSmoothedParams.exchange(false)) {
        gainProcessor->setState(requested_gain);
    }

    // Get audio buffer pointers for processing
    float *const *bufferPtrs = buffer.getArrayOfWritePointers();
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Process audio through our DSP
    gainProcessor->process(bufferPtrs, numSamples, numChannels, requested_gain);
}

juce::AudioProcessorEditor* HHOOMP::createEditor()
{
    return new HHOOMPEditor { *this };
}

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HHOOMP();
}
