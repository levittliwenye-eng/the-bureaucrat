# Contributing

Thank you for improving THE BUREAUCRAT.

## Before A Pull Request

1. Open or reference an issue for behavior changes with a broad user impact.
2. Keep changes focused and preserve the eight stable parameter identifiers.
3. Build the plug-in and run all tests described in `README.md`.
4. Do not add audio, fonts, screenshots, generated artwork, or third-party code
   unless its origin and redistribution licence are documented.
5. Do not add network access, telemetry, activation, or tracking without an
   explicit project decision and corresponding privacy review.

Contributions are submitted under AGPLv3, the same licence as the project code.
By submitting a contribution, you confirm that you have the right to provide it
under those terms. No separate right to relicense a contribution is implied.

Artwork contributions intended for `UI/v3/runtime/` must be clearly identified
and available under CC BY-SA 4.0 or a compatible licence.

## Coding Style

- Use C++17 and the existing JUCE conventions in the project.
- Keep real-time audio code allocation-free and lock-free.
- Add focused tests for DSP, state, parameter, or UI behavior changes.
- Use concise comments only where the implementation is not self-explanatory.

Brand names and logos are governed by `TRADEMARKS.md`; forks must not imply that
modified builds are official releases.
