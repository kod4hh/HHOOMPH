#pragma once

#include <pch.h>

class HHOOMPEditor : public juce::AudioProcessorEditor, 
                     public juce::Timer,
                     public juce::AudioProcessorParameter::Listener
{
public:
    HHOOMPEditor(HHOOMP& processor);
    ~HHOOMPEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Mouse handling for dial interaction
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    HHOOMP& audioProcessor;

    // Designer's visual assets
    juce::Image backgroundImage;
    juce::Image dialKnobImage;
    juce::Image toggleOffImage;
    juce::Image toggleOnImage;

    juce::Label titleLabel;
    juce::Label valueLabel;  // Show current parameter value

    // Animation system
    float currentDialRotation = 0.5f;
    float targetDialRotation = 0.5f;
    float animationSpeed = 0.6f;

    // Dial interaction
    juce::Rectangle<float> dialBounds;
    float lastDragY = 0.0f;
    bool isDragging = false;

    // Toggle switch interaction
    juce::Rectangle<float> toggleBounds;

    void loadAssets();
    void updateDialRotation();
    void timerCallback() override;
    float getValueFromMouse(const juce::MouseEvent& event);
    void setParameterValue(float newValue);
    void updateValueDisplay(float value);

    // Parameter listener callbacks
    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int, bool) override {}
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HHOOMPEditor)
};
