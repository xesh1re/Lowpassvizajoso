#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "FilterMath.h"
#include <array>
#include <vector>

class VisajosoProcessor : public juce::AudioProcessor
{
public:
    VisajosoProcessor();
    ~VisajosoProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Lowpass Visajoso"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    using MultSmoothed = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative>;

    MultSmoothed smoothCutoff { 900.0f };
    juce::LinearSmoothedValue<float> smoothReso   { 0.35f };
    juce::LinearSmoothedValue<float> smoothWobble { 0.35f };
    juce::LinearSmoothedValue<float> smoothSpeed  { 0.7f };
    juce::LinearSmoothedValue<float> smoothWarble { 0.5f };
    juce::LinearSmoothedValue<float> smoothBass   { 3.0f };
    juce::LinearSmoothedValue<float> smoothMix    { 1.0f };
    juce::LinearSmoothedValue<float> smoothOutput { 1.0f };

    // Hasta 4 biquads en serie por canal (12, 24 o 48 dB por octava) + shelf de graves
    std::array<std::array<FilterMath::Biquad, 4>, 2> lowpass;
    std::array<FilterMath::Biquad, 2> shelf;
    FilterMath::Coeffs coefPlain, coefReso, coefShelf;
    int controlCounter = 0;

    // LFOs del movimiento de agua (dos senoidales con velocidades inconmensurables = movimiento organico)
    double phaseA = 0.0, phaseB = 0.0;

    // Linea de retardo modulada para el temblor de tono
    static constexpr int delaySize = 4096;
    static constexpr int delayMask = delaySize - 1;
    std::array<std::vector<float>, 2> delayBuffer;
    int delayPos = 0;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VisajosoProcessor)
};
