cmake_minimum_required(VERSION 3.22)

project(LowpassVisajoso VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ON = copia el .vst3 a la carpeta VST3 del sistema despues de compilar
option(COPY_AFTER_BUILD "Copiar el plugin a la carpeta VST3 al terminar" ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(LowpassVisajoso
    COMPANY_NAME "Brou"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE Brou
    PLUGIN_CODE Lvis
    FORMATS VST3 Standalone
    PRODUCT_NAME "Lowpass Visajoso"
    COPY_PLUGIN_AFTER_BUILD ${COPY_AFTER_BUILD})

target_sources(LowpassVisajoso PRIVATE
    FilterMath.h
    VisajosoProcessor.cpp
    VisajosoProcessor.h
    VisajosoEditor.cpp
    VisajosoEditor.h)

target_compile_definitions(LowpassVisajoso PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(LowpassVisajoso
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
