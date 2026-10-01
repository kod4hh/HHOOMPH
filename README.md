# HHOOMPH

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

HHOOMPH adds perceived loudness and presence to any signal without increasing peak levels. Harmonic saturation with 2x oversampling and plateau-style gain compensation lets tracks cut through a mix while keeping headroom intact.

Free and open source, by [Headroom Heros](https://www.headroomheros.com).

---

## Features

- Inflator-style harmonic saturation with variable curve (warm/dense at low settings, aggressive at high)
- 2x oversampling with polyphase IIR anti-aliasing filter
- Plateau-style loudness compensation — gets louder without clipping
- DC blocker per channel
- Parameter smoothing — no zipper noise
- Soft-clip output stage
- DAW-compensated latency
- Universal Binary — Intel and Apple Silicon
- Signed and notarized by Apple

## Controls

| Control | Range | Description |
|---|---|---|
| Dial | 0–10 | Saturation amount. Below 1: bypass. 2–4: glue. 5–7: presence. 8–10: character. |
| Toggle | On/Off | Bypass the effect |

Tip: double-click the value display to type an exact number.

## Formats

- VST3 — macOS (Windows coming soon)
- Audio Unit (AU) — macOS
- Standalone

## System Requirements

- macOS 10.15 Catalina or later
- Intel or Apple Silicon Mac
- Compatible DAW with VST3 or AU support

## Install

Download the latest release from the [releases page](https://github.com/kod4hh/HHOOMPH/releases) and run the `.pkg` installer. It installs VST3 and AU automatically. No restart required for VST3; restart your DAW once for AU.

**Manual install (optional):**
- VST3: `/Library/Audio/Plug-Ins/VST3/`
- AU: `/Library/Audio/Plug-Ins/Components/`

## Build from Source

Requires CMake 3.15+ and a C++17 compiler.

```bash
git clone https://github.com/kod4hh/HHOOMPH.git
cd HHOOMPH
cmake -Bbuild -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## License

HHOOMPH is free and open source under the [GNU GPLv3](https://www.gnu.org/licenses/gpl-3.0).

Built with [JUCE](https://juce.com) and [chowdsp_utils](https://github.com/Chowdhury-DSP/chowdsp_utils) (BSD-3-Clause).

Copyright © 2025 Headroom Heros
