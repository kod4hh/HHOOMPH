#include "HHOOMPEditor.h"
#include "HHOOMP.h"
#include "BinaryData.h"

HHOOMPEditor::HHOOMPEditor(HHOOMP& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Load designer assets
    loadAssets();

    DBG("Background valid: " << backgroundImage.isValid() << " Size: " << backgroundImage.getWidth() << "x" << backgroundImage.getHeight());
    DBG("Dial knob valid: " << dialKnobImage.isValid() << " Size: " << dialKnobImage.getWidth() << "x" << dialKnobImage.getHeight());

    // Title label removed - background image has branding

    // Set up value display label (retro green display in black box)
    valueLabel.setText("5.0", juce::dontSendNotification);
    valueLabel.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    valueLabel.setJustificationType(juce::Justification::centred);
    // Commodore 64 / Matrix phosphor green: RGB(136, 255, 0)
    valueLabel.setColour(juce::Label::textColourId, juce::Colour(136, 255, 0));

    // Make label editable on double-click for precise value entry
    valueLabel.setEditable(false, true); // Double-click editable
    valueLabel.setColour(juce::Label::backgroundWhenEditingColourId, juce::Colours::black);
    valueLabel.setColour(juce::TextEditor::textColourId, juce::Colour(136, 255, 0));
    valueLabel.setColour(juce::TextEditor::backgroundColourId, juce::Colours::black);
    valueLabel.setColour(juce::TextEditor::highlightColourId, juce::Colour(136, 255, 0).withAlpha(0.3f));
    valueLabel.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    valueLabel.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(136, 255, 0).withAlpha(0.5f));

    valueLabel.onEditorShow = [this]() {
        if (auto* editor = valueLabel.getCurrentTextEditor())
        {
            editor->setInputRestrictions(4, "0123456789."); // Only numbers and decimal
            editor->grabKeyboardFocus(); // Force keyboard focus
            editor->selectAll(); // Select all text for easy replacement
        }
    };

    valueLabel.onTextChange = [this]() {
        // Parse typed value and update parameter
        float newValue = valueLabel.getText().getFloatValue();
        newValue = juce::jlimit(0.0f, 10.0f, newValue); // Clamp to 0-10
        setParameterValue(newValue);
        updateValueDisplay(newValue); // Update display formatting
    };

    addAndMakeVisible(valueLabel);

    // Initialize dial position from parameter
    auto currentValue = audioProcessor.getState().params.gain->get();
    DBG("CONSTRUCTOR - Initial parameter value: " << currentValue);
    targetDialRotation = currentValue / 10.0f;
    currentDialRotation = targetDialRotation; // Start at correct position
    updateValueDisplay(currentValue);

    // Listen to parameter changes from host/automation
    auto* gainParam = audioProcessor.getParameters()[0];
    gainParam->addListener(this);

    auto* bypassParam = audioProcessor.getParameters()[1];
    bypassParam->addListener(this);

    // Set fixed window size - matched to background aspect ratio (1131x1620)
    // 340 width, height = 340 / 0.698 = 487 to prevent cropping
    setSize(340, 487);

    // Disable resizing for v1 (free promo version)
    setResizable(false, false);

    // Start animation timer
    startTimerHz(60);
}

HHOOMPEditor::~HHOOMPEditor()
{
    stopTimer();
    auto* gainParam = audioProcessor.getParameters()[0];
    gainParam->removeListener(this);

    auto* bypassParam = audioProcessor.getParameters()[1];
    bypassParam->removeListener(this);
}

void HHOOMPEditor::paint(juce::Graphics& g)
{
    // Draw designer's background image (fills the whole window)
    if (backgroundImage.isValid()) {
        g.drawImage(
            backgroundImage,
            getLocalBounds().toFloat(),
            juce::RectanglePlacement::fillDestination
        );
    } else {
        // Fallback to black background
        g.fillAll(juce::Colours::black);
    }

    // Draw rotating dial knob
    if (dialKnobImage.isValid() && backgroundImage.isValid()) {
        auto bounds = getLocalBounds().toFloat();

        // Calculate scale factor applied to background image by fillDestination
        // fillDestination uses max(scaleX, scaleY) to fill the destination
        float bgImageWidth = static_cast<float>(backgroundImage.getWidth());
        float bgImageHeight = static_cast<float>(backgroundImage.getHeight());
        float scaleX = bounds.getWidth() / bgImageWidth;
        float scaleY = bounds.getHeight() / bgImageHeight;
        float scale = juce::jmax(scaleX, scaleY); // fillDestination uses max scale

        // Dial knob center position in v1.0 design (1131x1620 background)
        // Scaled from v0.9: (723, 903.5) * (1131/1449, 1620/1914)
        constexpr float knobCenterX = 564.0f;  // 723 * 0.780 = 564
        constexpr float knobCenterY = 764.0f;  // 903.5 * 0.846 = 764

        // Original bounding box size for reference: 240x545
        // Make it bigger - use a size multiplier
        constexpr float sizeMultiplier = 2.5f; // Natural looking size
        constexpr float knobWidth = 240.0f * sizeMultiplier;
        constexpr float knobHeight = 545.0f * sizeMultiplier;

        // Calculate scaled position and size
        // When fillDestination scales, it centers the image, so we need to account for that
        float scaledBgWidth = bgImageWidth * scale;
        float scaledBgHeight = bgImageHeight * scale;
        float offsetX = (bounds.getWidth() - scaledBgWidth) * 0.5f;
        float offsetY = (bounds.getHeight() - scaledBgHeight) * 0.5f;

        // Dial knob center in window coordinates
        float knobCenterWindowX = offsetX + knobCenterX * scale;
        float knobCenterWindowY = offsetY + knobCenterY * scale;

        // Dial knob bounds in window coordinates, centered on the knob center point
        dialBounds = juce::Rectangle<float>(
            knobCenterWindowX - (knobWidth * scale * 0.5f),
            knobCenterWindowY - (knobHeight * scale * 0.5f),
            knobWidth * scale,
            knobHeight * scale
        );

        // Draw dial knob with rotation
        g.saveState();
        auto centre = dialBounds.getCentre();

        // Rotation: 0.0 = 7 o'clock (225°), 1.0 = 5 o'clock (495° = 135°)
        constexpr float startAngle = 5.0f * juce::MathConstants<float>::pi / 4.0f;   // 225° (7 o'clock)
        constexpr float sweepAngle = 3.0f * juce::MathConstants<float>::pi / 2.0f;   // 270° sweep
        float angle = startAngle + currentDialRotation * sweepAngle;

        DBG("Paint - currentDialRotation: " << currentDialRotation << ", angle (degrees): " << (angle * 180.0f / juce::MathConstants<float>::pi));

        g.addTransform(juce::AffineTransform::rotation(angle, centre.x, centre.y));
        g.drawImage(dialKnobImage, dialBounds);
        g.restoreState();
    }

    // Draw bypass toggle switch at bottom
    if (toggleBounds.getWidth() > 0) {
        // Get current bypass state from parameter
        bool isBypassed = audioProcessor.getState().params.bypass->get();

        // Draw appropriate toggle image
        const juce::Image& toggleImage = isBypassed ? toggleOffImage : toggleOnImage;
        if (toggleImage.isValid()) {
            g.drawImage(toggleImage, toggleBounds, juce::RectanglePlacement::fillDestination);
        }
    }
}

void HHOOMPEditor::resized()
{
    auto bounds = getLocalBounds().toFloat();

    // Calculate scale factor for background image (same as in paint())
    // New background dimensions: 1131 x 1620
    float bgImageWidth = 1131.0f;
    float bgImageHeight = 1620.0f;
    float scaleX = bounds.getWidth() / bgImageWidth;
    float scaleY = bounds.getHeight() / bgImageHeight;
    float scale = juce::jmax(scaleX, scaleY); // fillDestination uses max scale

    // Position of black display box in original image coordinates (v1.0 design)
    // Centered horizontally in the dial, vertically centered in the black oval
    constexpr float displayBoxCenterX = 565.5f;  // Centered (1131/2)
    constexpr float displayBoxCenterY = 1023.0f; // Split the difference - perfect center
    constexpr float displayBoxWidth = 90.0f;
    constexpr float displayBoxHeight = 35.0f;

    // Calculate scaled position accounting for centering offset
    float scaledBgWidth = bgImageWidth * scale;
    float scaledBgHeight = bgImageHeight * scale;
    float offsetX = (bounds.getWidth() - scaledBgWidth) * 0.5f;
    float offsetY = (bounds.getHeight() - scaledBgHeight) * 0.5f;

    // Position value label in the black display box
    float labelX = offsetX + (displayBoxCenterX - displayBoxWidth * 0.5f) * scale;
    float labelY = offsetY + (displayBoxCenterY - displayBoxHeight * 0.5f) * scale;
    float labelW = displayBoxWidth * scale;
    float labelH = displayBoxHeight * scale;

    valueLabel.setBounds(juce::Rectangle<int>(
        static_cast<int>(labelX),
        static_cast<int>(labelY),
        static_cast<int>(labelW),
        static_cast<int>(labelH)
    ));

    // Position toggle switch at bottom center (200x373 toggle images)
    constexpr float toggleCenterX = 565.5f;  // Centered
    constexpr float toggleCenterY = 1364.0f; // Near bottom (moved up ~36px)
    constexpr float toggleWidth = 200.0f * 0.80f;   // 0.80x scale as mentioned
    constexpr float toggleHeight = 373.0f * 0.80f;

    float toggleX = offsetX + (toggleCenterX - toggleWidth * 0.5f) * scale;
    float toggleY = offsetY + (toggleCenterY - toggleHeight * 0.5f) * scale;
    float toggleW = toggleWidth * scale;
    float toggleH = toggleHeight * scale;

    toggleBounds = juce::Rectangle<float>(toggleX, toggleY, toggleW, toggleH);

    // Dial bounds are calculated in paint()
}

void HHOOMPEditor::loadAssets()
{
    // Load background image
    backgroundImage = juce::ImageCache::getFromMemory(
        BinaryData::interface_dsn_VST_whtbckgnd0202_png,
        BinaryData::interface_dsn_VST_whtbckgnd0202_pngSize
    );

    // Load dial knob image (the rotating part)
    dialKnobImage = juce::ImageCache::getFromMemory(
        BinaryData::dial_knob_png,
        BinaryData::dial_knob_pngSize
    );

    // Load toggle switch images
    toggleOffImage = juce::ImageCache::getFromMemory(
        BinaryData::switchtoggleoff_png,
        BinaryData::switchtoggleoff_pngSize
    );

    toggleOnImage = juce::ImageCache::getFromMemory(
        BinaryData::switchtoggleon_png,
        BinaryData::switchtoggleon_pngSize
    );

    DBG("Assets loaded - Background: " << (backgroundImage.isValid() ? "YES" : "NO")
        << ", Dial Knob: " << (dialKnobImage.isValid() ? "YES" : "NO")
        << ", Toggle Off: " << (toggleOffImage.isValid() ? "YES" : "NO")
        << ", Toggle On: " << (toggleOnImage.isValid() ? "YES" : "NO"));
}

void HHOOMPEditor::updateDialRotation()
{
    // Don't update from parameter while user is dragging!
    if (!isDragging)
    {
        // Sync target rotation from parameter value (0-10 range)
        auto paramValue = audioProcessor.getState().params.gain->get();
        targetDialRotation = paramValue / 10.0f; // Normalize to 0-1 for rotation

        // Update value label with smart formatting
        updateValueDisplay(paramValue);
    }

    // Smooth animation
    if (std::abs(currentDialRotation - targetDialRotation) > 0.001f) {
        currentDialRotation += (targetDialRotation - currentDialRotation) * animationSpeed;
        repaint();
    }
}

void HHOOMPEditor::timerCallback()
{
    updateDialRotation();
}

void HHOOMPEditor::mouseDown(const juce::MouseEvent& event)
{
    // Check if click is on toggle switch
    if (toggleBounds.contains(event.position))
    {
        // Toggle the bypass parameter
        bool currentBypass = audioProcessor.getState().params.bypass->get();
        audioProcessor.getState().params.bypass->setValueNotifyingHost(!currentBypass);
        repaint(); // Redraw to show new toggle state
        DBG("Toggle clicked - Bypass: " << (!currentBypass ? "ON" : "OFF"));
        return;
    }

    // Check if click is within dial bounds
    DBG("Mouse click at: " << event.position.x << ", " << event.position.y);
    DBG("Dial bounds: " << dialBounds.toString());
    DBG("Contains click: " << (dialBounds.contains(event.position) ? "YES" : "NO"));

    if (dialBounds.contains(event.position))
    {
        DBG("MOUSE DOWN - Dial interaction started");
        isDragging = true;
        lastDragY = event.position.y;
        audioProcessor.getParameters()[0]->beginChangeGesture();
    }
}

void HHOOMPEditor::mouseDrag(const juce::MouseEvent& event)
{
    if (!isDragging) return;

    // Calculate value change based on vertical drag distance
    float dragDistance = lastDragY - event.position.y; // Up = positive, down = negative
    float sensitivity = 200.0f; // Higher = less sensitive

    // Use targetDialRotation instead of reading parameter during drag
    float currentValue = targetDialRotation * 10.0f;
    float newValue = currentValue + (dragDistance / sensitivity) * 10.0f; // Map to 0-10 range

    // Clamp to 0-10
    newValue = juce::jlimit(0.0f, 10.0f, newValue);

    DBG("DRAG - Current: " << currentValue << " Distance: " << dragDistance << " New: " << newValue);

    // Update dial rotation directly during drag
    targetDialRotation = newValue / 10.0f;

    // Update value label immediately with smart formatting
    updateValueDisplay(newValue);

    // Update parameter using normalized 0-1 range (parameter range is 0-10)
    auto* param = audioProcessor.getParameters()[0];
    param->setValueNotifyingHost(newValue / 10.0f);

    lastDragY = event.position.y;
}

void HHOOMPEditor::mouseUp(const juce::MouseEvent& event)
{
    if (isDragging)
    {
        // Commit final value using normalized 0-1 range (parameter range is 0-10)
        float finalValue = targetDialRotation * 10.0f;

        DBG("Mouse up - Final value: " << finalValue << ", target rotation: " << targetDialRotation);

        auto* param = audioProcessor.getParameters()[0];
        param->setValueNotifyingHost(finalValue / 10.0f);

        // End gesture
        param->endChangeGesture();

        // Now exit drag mode
        isDragging = false;
    }
}

void HHOOMPEditor::setParameterValue(float newValue)
{
    // Update the parameter using normalized 0-1 range (parameter range is 0-10)
    auto* param = audioProcessor.getParameters()[0];
    param->setValueNotifyingHost(newValue / 10.0f);

    // Update dial rotation
    targetDialRotation = newValue / 10.0f;

    // Update value display with smart formatting
    updateValueDisplay(newValue);
}

void HHOOMPEditor::parameterValueChanged(int parameterIndex, float newValue)
{
    if (parameterIndex == 0 && !isDragging) { // Gain parameter - only update if not dragging
        // Get the actual parameter value (0-10 range) from chowdsp state
        auto actualValue = audioProcessor.getState().params.gain->get();
        targetDialRotation = actualValue / 10.0f; // Normalize to 0-1 for rotation
        updateValueDisplay(actualValue);
        DBG("Parameter changed externally to: " << actualValue);
    }
    else if (parameterIndex == 1) { // Bypass parameter
        // Repaint to update toggle switch visual state
        repaint();
        DBG("Bypass parameter changed to: " << (audioProcessor.getState().params.bypass->get() ? "ON" : "OFF"));
    }
}

void HHOOMPEditor::updateValueDisplay(float value)
{
    // Special formatting for value 10: remove decimal to save space
    if (value >= 9.95f) // Close enough to 10 (accounting for float precision)
    {
        valueLabel.setText("10", juce::dontSendNotification);
    }
    else
    {
        // Show with one decimal place for all other values
        valueLabel.setText(juce::String(value, 1), juce::dontSendNotification);
    }
}
