#include "VisajosoEditor.h"

using namespace VisajosoColours;

//==============================================================================
VisajosoLookAndFeel::VisajosoLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, textMain());
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, textDim());

    setColour (juce::ComboBox::backgroundColourId, navy());
    setColour (juce::ComboBox::outlineColourId, royal());
    setColour (juce::ComboBox::textColourId, textMain());
    setColour (juce::ComboBox::arrowColourId, vivid());

    setColour (juce::PopupMenu::backgroundColourId, navy());
    setColour (juce::PopupMenu::textColourId, textMain());
    setColour (juce::PopupMenu::highlightedBackgroundColourId, royal());

    setColour (juce::TextButton::buttonColourId, deepRoyal());
    setColour (juce::TextButton::buttonOnColourId, royal());
    setColour (juce::TextButton::textColourOffId, textMain());
    setColour (juce::TextButton::textColourOnId, textMain());
}

void VisajosoLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                            float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                            juce::Slider&)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (8.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    const float arcRadius = radius - 4.0f;
    const float bodyRadius = arcRadius - 8.0f;

    // Cuerpo con degradado azul rey -> negro
    g.setGradientFill (juce::ColourGradient (deepRoyal(), centre.x, centre.y - bodyRadius,
                                             black(), centre.x, centre.y + bodyRadius, false));
    g.fillEllipse (centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f);
    g.setColour (royal());
    g.drawEllipse (centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f, 1.5f);

    // Riel
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour (0xff141b45));
    g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    // Valor, con brillo
    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                         rotaryStartAngle, angle, true);
    g.setColour (vivid().withAlpha (0.28f));
    g.strokePath (value, juce::PathStrokeType (10.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    g.setColour (vivid());
    g.strokePath (value, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    // Aguja
    juce::Path pointer;
    const float len = bodyRadius * 0.55f;
    pointer.addRoundedRectangle (-1.5f, -bodyRadius + 4.0f, 3.0f, len, 1.5f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
    g.setColour (juce::Colour (0xffeaf3ff));
    g.fillPath (pointer);
}

//==============================================================================
FilterDisplay::FilterDisplay (juce::AudioProcessorValueTreeState& s)
    : state (s)
{
    startTimerHz (30);
}

void FilterDisplay::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (1.0f);

    g.setGradientFill (juce::ColourGradient (navy(), b.getX(), b.getY(),
                                             black(), b.getX(), b.getBottom(), false));
    g.fillRoundedRectangle (b, 10.0f);

    auto inner = b.reduced (14.0f, 10.0f);

    auto caption = inner.removeFromTop (14.0f);
    g.setColour (textDim());
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.drawText ("Curva del filtro: donde la linea baja, esas frecuencias se escuchan mas bajito",
                caption, juce::Justification::centredLeft);

    auto plot = inner.reduced (0.0f, 2.0f);
    auto axis = plot.removeFromBottom (14.0f);

    constexpr double sr = 48000.0;
    constexpr double minF = 20.0, maxF = 20000.0;
    constexpr double minDb = -60.0, maxDb = 18.0;

    auto xForFreq = [&] (double f)
    {
        return plot.getX() + (float) (std::log (f / minF) / std::log (maxF / minF)) * plot.getWidth();
    };

    auto yForDb = [&] (double db)
    {
        db = juce::jlimit (minDb, maxDb, db);
        return plot.getBottom() - (float) ((db - minDb) / (maxDb - minDb)) * plot.getHeight();
    };

    // Agua animada de fondo (decorativa, se mueve con la velocidad del wobble)
    {
        const double t = juce::Time::getMillisecondCounterHiRes() * 0.001;
        const float wspeed = state.getRawParameterValue ("wspeed")->load();
        const float waveSpeed = 0.8f + wspeed * 1.5f;

        for (int k = 0; k < 3; ++k)
        {
            juce::Path wave;
            const float baseY = plot.getY() + plot.getHeight() * (0.35f + 0.22f * (float) k);
            const float amp = 5.0f + 3.0f * (float) k;
            const float freq = 0.012f + 0.004f * (float) k;

            for (float xPos = plot.getX(); xPos <= plot.getRight(); xPos += 4.0f)
            {
                const float yPos = baseY + amp * std::sin (xPos * freq * 6.0f
                                                           + (float) t * waveSpeed * (1.0f + 0.4f * (float) k));
                if (xPos == plot.getX())
                    wave.startNewSubPath (xPos, yPos);
                else
                    wave.lineTo (xPos, yPos);
            }

            g.setColour (royal().withAlpha (0.20f - 0.04f * (float) k));
            g.strokePath (wave, juce::PathStrokeType (2.0f));
        }
    }

    // Cuadricula
    g.setFont (juce::Font (juce::FontOptions (10.0f)));
    const double gridFreqs[3] = { 100.0, 1000.0, 10000.0 };
    const char* gridNames[3] = { "100 Hz", "1 kHz", "10 kHz" };

    for (int k = 0; k < 3; ++k)
    {
        const float xPos = xForFreq (gridFreqs[k]);
        g.setColour (royal().withAlpha (0.25f));
        g.drawVerticalLine ((int) xPos, plot.getY(), plot.getBottom());
        g.setColour (textDim());
        g.drawText (gridNames[k], juce::Rectangle<float> (xPos - 30.0f, axis.getY(), 60.0f, axis.getHeight()),
                    juce::Justification::centred);
    }

    const double gridDbs[3] = { -48.0, -24.0, 0.0 };
    for (double db : gridDbs)
    {
        g.setColour (royal().withAlpha (db == 0.0 ? 0.55f : 0.22f));
        g.drawHorizontalLine ((int) yForDb (db), plot.getX(), plot.getRight());
    }

    // Parametros actuales
    const float cutoff = state.getRawParameterValue ("cutoff")->load();
    const float reso   = state.getRawParameterValue ("reso")->load();
    const int slope    = juce::jlimit (0, 2, (int) (state.getRawParameterValue ("slope")->load() + 0.5f));
    const float wobble = state.getRawParameterValue ("wobble")->load();
    const float bass   = state.getRawParameterValue ("bass")->load();

    const int numStages = slope == 0 ? 1 : (slope == 1 ? 2 : 4);
    const auto shelfCoeffs = FilterMath::makeLowShelf (140.0, sr, bass);

    auto buildCurve = [&] (double fc)
    {
        fc = juce::jlimit (20.0, 0.45 * sr, fc);
        const auto plain = FilterMath::makeLowpass (fc, sr, 0.7071);
        const auto resonant = FilterMath::makeLowpass (fc, sr, (double) FilterMath::resoToQ (reso));

        juce::Path path;
        constexpr int numPoints = 240;

        for (int k = 0; k < numPoints; ++k)
        {
            const double f = minF * std::pow (maxF / minF, (double) k / (double) (numPoints - 1));

            double mag = 1.0;
            for (int s = 0; s < numStages; ++s)
                mag *= FilterMath::magnitude (s == numStages - 1 ? resonant : plain, f, sr);
            mag *= FilterMath::magnitude (shelfCoeffs, f, sr);

            const double db = 20.0 * std::log10 (std::max (mag, 1.0e-6));
            const float xPos = xForFreq (f);
            const float yPos = yForDb (db);

            if (k == 0)
                path.startNewSubPath (xPos, yPos);
            else
                path.lineTo (xPos, yPos);
        }

        return path;
    };

    // Rango por donde "viaja" el corte por culpa del wobble
    if (wobble > 0.01f)
    {
        const double octaves = 1.5 * (double) wobble;
        g.setColour (vivid().withAlpha (0.28f));
        g.strokePath (buildCurve ((double) cutoff * std::pow (2.0, -octaves)), juce::PathStrokeType (1.5f));
        g.strokePath (buildCurve ((double) cutoff * std::pow (2.0, octaves)), juce::PathStrokeType (1.5f));
    }

    // Curva principal
    const auto curve = buildCurve ((double) cutoff);

    juce::Path filled (curve);
    filled.lineTo (plot.getRight(), plot.getBottom());
    filled.lineTo (plot.getX(), plot.getBottom());
    filled.closeSubPath();

    g.setGradientFill (juce::ColourGradient (vivid().withAlpha (0.55f), 0.0f, plot.getY(),
                                             royal().withAlpha (0.05f), 0.0f, plot.getBottom(), false));
    g.fillPath (filled);

    g.setColour (vivid().withAlpha (0.30f));
    g.strokePath (curve, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    g.setColour (vivid());
    g.strokePath (curve, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    // Marca del punto de corte
    g.setColour (juce::Colour (0xffeaf3ff).withAlpha (0.5f));
    g.drawVerticalLine ((int) xForFreq ((double) cutoff), plot.getY(), plot.getBottom());

    g.setColour (royal());
    g.drawRoundedRectangle (b, 10.0f, 1.5f);
}

//==============================================================================
VisajosoEditor::VisajosoEditor (VisajosoProcessor& p)
    : juce::AudioProcessorEditor (&p), processor (p), display (p.apvts)
{
    setLookAndFeel (&lookAndFeel);

    const char* ids[8]   = { "cutoff", "reso", "wobble", "wspeed", "warble", "bass", "mix", "output" };
    const char* names[8] = { "CUTOFF", "RESONANCE", "WOBBLE", "SPEED", "WARBLE", "BASS", "MIX", "OUTPUT" };

    const char* tips[8] =
    {
        "Desde que frecuencia se empiezan a cortar los agudos. Mas a la izquierda = suena mas tapado, como detras de una pared.",
        "Resalta justo la frecuencia de corte. Poco = natural. Mucho = sonido mas liquido y con silbido.",
        "Hace que el corte suba y baje solo, como el movimiento del agua. En la pantalla se ve el rango como lineas claras.",
        "Que tan rapido se mueve el Wobble. Lento = olas grandes. Rapido = burbujeo.",
        "Hace temblar un poquito el tono, como cuando hablas bajo el agua.",
        "Refuerza los graves para el golpe de bombo que se oye desde afuera de la disco.",
        "Mezcla entre el sonido original (izquierda) y el filtrado (derecha).",
        "Volumen final del plugin. Subelo si el efecto queda muy bajito."
    };

    addAndMakeVisible (display);

    presetsLabel.setText ("PRESETS RAPIDOS:", juce::dontSendNotification);
    presetsLabel.setJustificationType (juce::Justification::centredLeft);
    presetsLabel.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    presetsLabel.setColour (juce::Label::textColourId, textMain());
    addAndMakeVisible (presetsLabel);

    waterButton.setTooltip ("Suena como estar bajo el agua: tapado, con olas y tono temblando.");
    clubButton.setTooltip ("Suena como estar afuera de la disco: solo se oyen los graves y el bombo.");
    normalButton.setTooltip ("Un lowpass normal, sin efectos raros.");
    addAndMakeVisible (waterButton);
    addAndMakeVisible (clubButton);
    addAndMakeVisible (normalButton);

    waterButton.onClick  = [this] { applyPreset (900.0f, 0.35f, 1, 0.35f, 0.7f, 0.5f, 3.0f, 1.0f, 0.0f); };
    clubButton.onClick   = [this] { applyPreset (220.0f, 0.45f, 2, 0.0f, 0.6f, 0.0f, 6.0f, 1.0f, 3.0f); };
    normalButton.onClick = [this] { applyPreset (2000.0f, 0.20f, 0, 0.0f, 0.6f, 0.0f, 0.0f, 1.0f, 0.0f); };

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& k = knobs[i];

        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 18);
        k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                      juce::MathConstants<float>::pi * 2.8f, true);
        k.slider.setTooltip (tips[i]);
        addAndMakeVisible (k.slider);

        k.label.setText (names[i], juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        k.label.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
        k.label.setColour (juce::Label::textColourId, textMain());
        addAndMakeVisible (k.label);

        k.desc.setText (tips[i], juce::dontSendNotification);
        k.desc.setJustificationType (juce::Justification::centredTop);
        k.desc.setFont (juce::Font (juce::FontOptions (11.0f)));
        addAndMakeVisible (k.desc);

        k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            processor.apvts, ids[i], k.slider);
    }

    slopeLabel.setText ("SLOPE", juce::dontSendNotification);
    slopeLabel.setJustificationType (juce::Justification::centred);
    slopeLabel.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    slopeLabel.setColour (juce::Label::textColourId, textMain());
    addAndMakeVisible (slopeLabel);

    slopeDesc.setText ("Que tan fuerte se cortan los agudos. 12 dB = suave. 24 dB = tapado. 48 dB = como detras de una pared gruesa.",
                       juce::dontSendNotification);
    slopeDesc.setJustificationType (juce::Justification::centredTop);
    slopeDesc.setFont (juce::Font (juce::FontOptions (11.0f)));
    addAndMakeVisible (slopeDesc);

    slopeBox.addItemList (processor.apvts.getParameter ("slope")->getAllValueStrings(), 1);
    slopeBox.setTooltip ("Pendiente del filtro: 12, 24 o 48 dB por octava.");
    addAndMakeVisible (slopeBox);
    slopeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor.apvts, "slope", slopeBox);

    setSize (880, 720);
}

VisajosoEditor::~VisajosoEditor()
{
    setLookAndFeel (nullptr);
}

void VisajosoEditor::setParam (const juce::String& id, float plainValue)
{
    if (auto* param = processor.apvts.getParameter (id))
        param->setValueNotifyingHost (processor.apvts.getParameterRange (id).convertTo0to1 (plainValue));
}

void VisajosoEditor::applyPreset (float cutoff, float reso, int slope, float wobble, float wspeed,
                                  float warble, float bass, float mix, float output)
{
    setParam ("cutoff", cutoff);
    setParam ("reso", reso);
    setParam ("slope", (float) slope);
    setParam ("wobble", wobble);
    setParam ("wspeed", wspeed);
    setParam ("warble", warble);
    setParam ("bass", bass);
    setParam ("mix", mix);
    setParam ("output", output);
}

void VisajosoEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0b1238), 0.0f, 0.0f,
                                             black(), 0.0f, (float) getHeight(), false));
    g.fillAll();

    auto header = getLocalBounds().reduced (24, 0).removeFromTop (62);

    g.setColour (vivid());
    g.setFont (juce::Font (juce::FontOptions (32.0f, juce::Font::bold)));
    g.drawText ("LOWPASS VISAJOSO", header.removeFromLeft (400), juce::Justification::centredLeft);

    g.setColour (textDim());
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawFittedText ("Un lowpass corta los agudos y deja pasar los graves. Aqui ademas se mueve como agua y puede sonar como afuera de la disco.",
                      header, juce::Justification::centredRight, 2);

    // Franja azul rey -> azul vivo bajo el titulo
    g.setGradientFill (juce::ColourGradient (royal(), 24.0f, 0.0f,
                                             vivid(), (float) getWidth() - 24.0f, 0.0f, false));
    g.fillRect (24.0f, 64.0f, (float) getWidth() - 48.0f, 3.0f);
}

void VisajosoEditor::resized()
{
    auto area = getLocalBounds().reduced (20, 0);
    area.removeFromTop (70);

    display.setBounds (area.removeFromTop (160).reduced (0, 4));
    area.removeFromTop (6);

    auto presetRow = area.removeFromTop (34).reduced (0, 2);
    presetsLabel.setBounds (presetRow.removeFromLeft (140));
    waterButton.setBounds (presetRow.removeFromLeft (170).reduced (4, 0));
    clubButton.setBounds (presetRow.removeFromLeft (210).reduced (4, 0));
    normalButton.setBounds (presetRow.removeFromLeft (120).reduced (4, 0));

    area.removeFromTop (4);
    area.removeFromBottom (10);

    auto row1 = area.removeFromTop (area.getHeight() / 2);
    auto row2 = area;

    const int colW = row1.getWidth() / 5;

    auto placeKnob = [] (Knob& k, juce::Rectangle<int> cell)
    {
        k.label.setBounds (cell.removeFromTop (18));
        k.desc.setBounds (cell.removeFromBottom (78).reduced (4, 0));
        k.slider.setBounds (cell.reduced (6, 0));
    };

    for (size_t i = 0; i < 5; ++i)
        placeKnob (knobs[i], row1.removeFromLeft (colW));

    for (size_t i = 5; i < 8; ++i)
        placeKnob (knobs[i], row2.removeFromLeft (colW));

    auto slopeCell = row2.removeFromLeft (colW);
    slopeLabel.setBounds (slopeCell.removeFromTop (18));
    slopeDesc.setBounds (slopeCell.removeFromBottom (78).reduced (4, 0));
    slopeBox.setBounds (slopeCell.withSizeKeepingCentre (colW - 24, 30));
}
