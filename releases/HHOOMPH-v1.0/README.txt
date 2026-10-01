HHOOMPH v1.0 — Harmonic Saturation Plugin
==========================================
by Headroom Heros | headroomheros.com
Free and Open Source (GPL v3)

UNIVERSAL BINARY — Intel and Apple Silicon


WHAT IT DOES
------------
HHOOMPH adds perceived loudness and presence without raising peak levels.
Turn the dial up and tracks cut through a mix. Keep the peaks where they are.


INSTALLATION
------------
Run the included .pkg installer. It will install VST3 and AU automatically
and guide you through Apple's standard license acceptance.

After install:
  VST3: restart your DAW and rescan plugins. Look for HHOOMPH under Headroom Heros.
  AU:   restart your DAW once. Look for HHOOMPH under Headroom Heros.

Manual install locations (if needed):
  VST3:  /Library/Audio/Plug-Ins/VST3/
  AU:    /Library/Audio/Plug-Ins/Components/
  App:   Copy HHOOMPH.app to your Applications folder


CONTROLS
--------
Dial (0–10)
  Below 1    Bypass — audio passes through unchanged
  2–4        Glue and thickening (subtle)
  5–7        Presence and loudness (obvious)
  8–10       Character and aggression

  Tip: double-click the value display to type an exact number.

Toggle
  Bypass the effect on/off


TIPS
----
- Start at 2–3 for subtle glue on drums or a mix buss
- Use 5–7 for clear loudness enhancement on vocals or leads
- Try 8–10 on loops or individual samples for aggressive character
- Compare bypass toggle at a sweet spot to hear the difference


TECHNICAL
---------
- 2x oversampling with polyphase IIR anti-aliasing filter
- Plateau-style gain compensation (louder without clipping)
- DC blocker per channel
- Parameter smoothing (no zipper noise)
- Soft-clip output stage
- DAW-compensated latency
- Formats: VST3, Audio Unit, Standalone
- macOS 10.15 or later
- Universal Binary (Intel + Apple Silicon)
- Signed and notarized by Apple


SYSTEM REQUIREMENTS
-------------------
- macOS 10.15 Catalina or later
- Intel or Apple Silicon Mac
- DAW with VST3 or AU support (for plugin formats)


OPEN SOURCE
-----------
HHOOMPH is free and open source under the GNU GPLv3.
Source code: https://github.com/kod4hh/HHOOMPH

Built with:
  JUCE Framework            — juce.com
  chowdsp_utils             — github.com/Chowdhury-DSP/chowdsp_utils (BSD-3-Clause)

See the Licenses folder for full legal text.

Copyright © 2025 Headroom Heros
