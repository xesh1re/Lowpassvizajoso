#include "VisajosoProcessor.h"
#include "VisajosoEditor.h"
#include <cmath>

namespace
{
    constexpr double kShelfHz = 140.0;
    constexpr float kPlainQ = 0.7071f;
}

VisajosoProcessor::VisajosoProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout VisajosoProcessor::createLayout()
{
    using P    = juce::AudioParameterFloat;
    using NR   = juce::NormalisableRange<float>;
    using Attr = juce::AudioParameterFloatAttributes;

    auto percent = [] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; };

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<P> (juce::ParameterID { "cutoff", 1 }, "Cutoff",
                                           NR (60.0f, 20000.0f, 1.0f, 0.3f), 900.0f,
                                           Attr().withStringFromValueFunction ([] (float v, int)
                                           {
                                               return juce::String (juce::roundToInt (v)) + " Hz";
                                           })));

    params.push_back (std::make_unique<P> (juce::ParameterID { "reso", 1 }, "Resonance",
                                           NR (0.0f, 1.0f, 0.01f), 0.35f,
                                           Attr().withStringFromValueFunction (percent)));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "slope", 1 }, "Slope",
                                                                    juce::StringArray { "12 dB", "24 dB", "48 dB" }, 1));

    params.push_back (std::make_unique<P> (juce::ParameterID { "wobble", 1 }, "Wobble",
                                           NR (0.0f, 1.0f, 0.01f), 0.35f,
                                           Attr().withStringFromValueFunction (percent)));

    params.push_back (std::make_unique<P> (juce::ParameterID { "wspeed", 1 }, "Speed",
                                           NR (0.05f, 8.0f, 0.01f, 0.4f), 0.7f,
                                           Attr().withStringFromValueFunction ([] (float v, int)
                                           {
                                               return juce::String (v, 2) + " Hz";
                                           })));

    params.push_back (std::make_unique<P> (juce::ParameterID { "warble", 1 }, "Warble",
                                           NR (0.0f, 1.0f, 0.01f), 0.5f,
                                           Attr().withStringFromValueFunction (percent)));

    params.push_back (std::make_unique<P> (juce::ParameterID { "bass", 1 }, "Bass",
                                           NR (0.0f, 12.0f, 0.1f), 3.0f,
                                           Attr().withLabel ("dB")));

    params.push_back (std::make_unique<P> (juce::ParameterID { "mix", 1 }, "Mix",
                                           NR (0.0f, 1.0f, 0.01f), 1.0f,
                                           Attr().withStringFromValueFunction (percent)));

    params.push_back (std::make_unique<P> (juce::ParameterID { "output", 1 }, "Output",
                                           NR (-24.0f, 12.0f, 0.1f), 0.0f,
                                           Attr().withLabel ("dB")));

    return { params.begin(), params.end() };
}

void VisajosoProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;

    auto raw = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    smoothCutoff.reset (sampleRate, 0.05);
    smoothReso.reset (sampleRate, 0.03);
    smoothWobble.reset (sampleRate, 0.03);
    smoothSpeed.reset (sampleRate, 0.05);
    smoothWarble.reset (sampleRate, 0.03);
    smoothBass.reset (sampleRate, 0.03);
    smoothMix.reset (sampleRate, 0.02);
    smoothOutput.reset (sampleRate, 0.02);

    smoothCutoff.setCurrentAndTargetValue (raw ("cutoff"));
    smoothReso.setCurrentAndTargetValue (raw ("reso"));
    smoothWobble.setCurrentAndTargetValue (raw ("wobble"));
    smoothSpeed.setCurrentAndTargetValue (raw ("wspeed"));
    smoothWarble.setCurrentAndTargetValue (raw ("warble"));
    smoothBass.setCurrentAndTargetValue (raw ("bass"));
    smoothMix.setCurrentAndTargetValue (raw ("mix"));
    smoothOutput.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (raw ("output")));

    for (size_t ch = 0; ch < 2; ++ch)
    {
        for (auto& stage : lowpass[ch])
            stage.reset();

        shelf[ch].reset();
        delayBuffer[ch].assign ((size_t) delaySize, 0.0f);
    }

    delayPos = 0;
    phaseA = 0.0;
    phaseB = 0.0;
    controlCounter = 0;
}

bool VisajosoProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return out == layouts.getMainInputChannelSet();
}

void VisajosoProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples  = buffer.getNumSamples();

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    auto raw = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    smoothCutoff.setTargetValue (raw ("cutoff"));
    smoothReso.setTargetValue (raw ("reso"));
    smoothWobble.setTargetValue (raw ("wobble"));
    smoothSpeed.setTargetValue (raw ("wspeed"));
    smoothWarble.setTargetValue (raw ("warble"));
    smoothBass.setTargetValue (raw ("bass"));
    smoothMix.setTargetValue (raw ("mix"));
    smoothOutput.setTargetValue (juce::Decibels::decibelsToGain (raw ("output")));

    const int slope = juce::jlimit (0, 2, (int) (raw ("slope") + 0.5f));
    const int numStages = slope == 0 ? 1 : (slope == 1 ? 2 : 4);

    const double sr = currentSampleRate;
    const float srf = (float) sr;
    constexpr float twoPi = juce::MathConstants<float>::twoPi;

    float* data[2] = { nullptr, nullptr };
    for (int ch = 0; ch < numChannels; ++ch)
        data[ch] = buffer.getWritePointer (ch);

    for (int i = 0; i < numSamples; ++i)
    {
        const float cutoff = smoothCutoff.getNextValue();
        const float reso   = smoothReso.getNextValue();
        const float wobble = smoothWobble.getNextValue();
        const float speed  = smoothSpeed.getNextValue();
        const float warble = smoothWarble.getNextValue();
        const float bass   = smoothBass.getNextValue();
        const float mix    = smoothMix.getNextValue();
        const float outGain = smoothOutput.getNextValue();

        // LFOs: dos velocidades que no se alinean, para que el "agua" no se repita igual
        phaseA += (double) speed / sr;
        if (phaseA >= 1.0) phaseA -= 1.0;
        phaseB += (double) speed * 1.618 / sr;
        if (phaseB >= 1.0) phaseB -= 1.0;

        // Los coeficientes se recalculan cada 16 muestras (suficiente para un barrido suave)
        if (controlCounter <= 0)
        {
            controlCounter = 16;

            const float lfo = 0.6f * std::sin (twoPi * (float) phaseA)
                            + 0.4f * std::sin (twoPi * (float) phaseB);

            // El wobble mueve el corte hasta +-1.5 octavas
            const float fc = juce::jlimit (20.0f, 0.45f * srf, cutoff * std::exp2 (wobble * 1.5f * lfo));

            coefPlain = FilterMath::makeLowpass (fc, sr, kPlainQ);
            coefReso  = FilterMath::makeLowpass (fc, sr, FilterMath::resoToQ (reso));
            coefShelf = FilterMath::makeLowShelf (kShelfHz, sr, bass);

            for (size_t ch = 0; ch < 2; ++ch)
            {
                for (int s = 0; s < 4; ++s)
                    lowpass[ch][(size_t) s].c = (s == numStages - 1) ? coefReso : coefPlain;

                shelf[ch].c = coefShelf;
            }
        }
        --controlCounter;

        // Temblor de tono: retardo de 0 a 6 ms que oscila (el retardo es 0 cuando Warble = 0)
        const float depthMs = warble * 3.0f;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const size_t c = (size_t) ch;
            const float dry = data[ch][i];

            auto& buf = delayBuffer[c];
            buf[(size_t) delayPos] = dry;

            // Izquierda y derecha con fases distintas = efecto mas ancho y "liquido"
            const float lfoD = 0.6f * std::sin (twoPi * ((float) phaseA + 0.25f * (float) ch))
                             + 0.4f * std::sin (twoPi * ((float) phaseB + 0.10f * (float) ch));

            const float delaySamples = depthMs * (1.0f + lfoD) * 0.001f * srf;
            const float readPos = (float) delayPos - delaySamples;
            const int i0 = (int) std::floor (readPos);
            const float frac = readPos - (float) i0;

            const float s0 = buf[(size_t) (i0 & delayMask)];
            const float s1 = buf[(size_t) ((i0 + 1) & delayMask)];

            float wet = s0 + (s1 - s0) * frac;

            for (int s = 0; s < numStages; ++s)
                wet = lowpass[c][(size_t) s].process (wet);

            wet = shelf[c].process (wet);

            data[ch][i] = (dry * (1.0f - mix) + wet * mix) * outGain;
        }

        delayPos = (delayPos + 1) & delayMask;
    }
}

juce::AudioProcessorEditor* VisajosoProcessor::createEditor()
{
    return new VisajosoEditor (*this);
}

void VisajosoProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void VisajosoProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VisajosoProcessor();
}
