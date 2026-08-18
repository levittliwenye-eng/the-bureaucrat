# Installation

THE BUREAUCRAT 1.0.0 supports macOS 11 or newer on Apple Silicon and 64-bit
Intel Macs.

## Build From Source

Follow the commands in `README.md`. The resulting plug-ins are located under:

- `build/TheBureaucrat_artefacts/Debug/AU/THE BUREAUCRAT.component`
- `build/TheBureaucrat_artefacts/Debug/VST3/THE BUREAUCRAT.vst3`

Copy them to the user-local plug-in folders:

- AU: `~/Library/Audio/Plug-Ins/Components/`
- VST3: `~/Library/Audio/Plug-Ins/VST3/`

Quit audio applications before replacing an existing build, then reopen the DAW
and request a plug-in rescan. The manufacturer is shown as Disconnec audio.

## Uninstall

Quit all audio applications and remove the two plug-in bundles from the folders
above. The plug-in creates no account, background service, activation file, or
telemetry database.

Developer-built unsigned or ad-hoc-signed binaries may trigger macOS security
warnings. Public binary releases should be signed and notarized by their
distributor; source builds do not require an installer.
