# Lowpass Visajoso (VST3)

Filtro pasa-bajos con caracter: suena bajo el agua o afuera de una discoteca.
Interfaz en negro, azul rey y azul vivo. Hecho con JUCE.

## Presets rapidos (botones arriba)

- **Bajo el agua**: tapado, con olas y el tono temblando.
- **Afuera de la disco**: corte fuerte y graves reforzados, solo se oye el bombo.
- **Normal**: un lowpass limpio.

## Perillas

- **Cutoff**: desde que frecuencia se cortan los agudos.
- **Resonance**: resalta la frecuencia de corte (mas liquido y con silbido).
- **Wobble**: el corte sube y baja solo, como el agua (hasta 1.5 octavas).
- **Speed**: velocidad del Wobble y del Warble.
- **Warble**: temblor de tono, como hablar bajo el agua. Izquierda y derecha se mueven distinto.
- **Bass**: refuerzo de graves (shelf a 140 Hz).
- **Mix** y **Output**.
- **Slope**: 12, 24 o 48 dB por octava.

## Compilar en Windows

Visual Studio 2022 (C++), CMake (con "Add to PATH") y Git. Luego doble clic a `build.bat`.
O sube la carpeta a un repo de GitHub y descarga el resultado desde Actions > Artifacts.
