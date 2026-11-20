#pragma once

#include "VisajosoProcessor.h"
#include <array>
#include <memory>

// Paleta: negro, azul rey y azul vivo
namespace VisajosoColours
{
    inline juce::Colour vivid()    { return juce::Colour (0xff2d8cff); }  // azul vivo
    inline juce::Colour royal()    { return juce::Colour (0xff2b44e8); }  // azul rey
    inline juce::Colour deepRoyal(){ return juce::Colour (0xff14206e); }
    inline juce::Colour black()    { return juce::Colour (0xff03040a); }
    inline juce::Colour navy()     { return juce::Colour (0xff0a1030); }
    inline juce::Colour textMain() { return juce::Colour (0xffe3edff); }
    inline juce::Colour textDim()  { return juce::Colour (0xff8fa3d9); }
}

class VisajosoLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VisajosoLookAndFeel();

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override;
};

// Pantalla: curva del filtro en vivo, con "agua" animada de fondo
class FilterDisplay : public juce::Component, private juce::Timer
{
public:
    explicit FilterDisplay (juce::AudioProcessorValueTreeState& s);
    ~FilterDisplay() override { stopTimer(); }

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override { repaint(); }

    juce::AudioProcessorValueTreeState& state;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterDisplay)
};

class VisajosoEditor : public juce::AudioProcessorEditor
{
public:
    explicit VisajosoEditor (VisajosoProcessor&);
    ~VisajosoEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        juce::Label desc;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void setParam (const juce::String& id, float plainValue);
    void applyPreset (float cutoff, float reso, int slope, float wobble, float wspeed,
                      float warble, float bass, float mix, float output);

    VisajosoProcessor& processor;
    VisajosoLookAndFeel lookAndFeel;
    juce::TooltipWindow tooltipWindow { this, 400 };

    FilterDisplay display;

    juce::Label presetsLabel;
    juce::TextButton waterButton { "BAJO EL AGUA" };
    juce::TextButton clubButton { "AFUERA DE LA DISCO" };
    juce::TextButton normalButton { "NORMAL" };

    std::array<Knob, 8> knobs;

    juce::Label slopeLabel, slopeDesc;
    juce::ComboBox slopeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> slopeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VisajosoEditor)
};
