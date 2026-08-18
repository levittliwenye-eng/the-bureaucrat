# Third-Party Components

This repository uses the AGPLv3 open-source licensing path for JUCE. Third-party
components remain under their own terms; the project licence does not replace
those terms.

## Runtime And Build Dependencies

| Component | Version/source | Purpose | Licence |
| --- | --- | --- | --- |
| JUCE Framework | 8.0.11, fetched from `juce-framework/JUCE` | Plug-in formats, parameters, UI, audio and DSP support | AGPLv3 or commercial JUCE licence; this project uses AGPLv3 |
| Steinberg VST3 SDK | JUCE-bundled | VST3 interfaces and wrapper | MIT |
| Apple AudioUnitSDK | JUCE-bundled, macOS only | AU wrapper support | Apache-2.0 |
| HarfBuzz | JUCE-bundled | Text shaping | Old MIT |
| SheenBidi | JUCE-bundled | Bidirectional text | Apache-2.0 |
| libpng | JUCE-bundled | PNG decoding | PNG Reference Library licence |
| zlib | JUCE-bundled | Compression | zlib licence |
| Independent JPEG Group code | JUCE-bundled | Image decoding | IJG licence |
| FLAC | JUCE-bundled Standalone dependency | Audio file support | BSD-style Xiph.Org licence |
| Ogg Vorbis | JUCE-bundled Standalone dependency | Audio file support | BSD-style Xiph.Org licence |

The complete licence texts and notices copied from the verified JUCE 8.0.11
checkout are in `LICENSES/`. JUCE source is not vendored in this repository; the
build either fetches the pinned release or uses a caller-supplied checkout.

## Project Content

- Source code under `Source/` and project tests are project-authored and AGPLv3.
- Runtime artwork under `UI/v3/runtime/` is covered by `ASSETS_LICENSE.md`.
- No audio samples, impulse responses, MIDI files, model weights, activation
  libraries, or analytics SDKs are included.
- Marketing audio and videos are deliberately excluded from the public source
  repository.
