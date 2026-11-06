OrbitalPan - Mac tester build
============================

This download contains a compiled VST3 plugin for Intel and Apple silicon
Macs running macOS 11 or later. No Xcode, Homebrew, or compilation is needed.
The DAW must also support your macOS version. This is a test build.

INSTALL IN ABLETON LIVE

1. Quit Ableton Live.
2. Unzip the download. Keep OrbitalPan.vst3 as a complete bundle;
   do not copy files out of its Contents folder.
3. In Finder, press Command+Shift+G, paste this path, and press Return:

   ~/Library/Audio/Plug-Ins/

4. Open the VST3 folder, or create a folder named VST3 if it is missing.
5. If an older OrbitalPan.vst3 is installed, move it somewhere outside the
   plugin folders as a backup. Copy the new OrbitalPan.vst3 into VST3.
6. Open Live > Settings > Plug-Ins. Enable Use VST3 Plug-In System Folders.
   Rescan and search for OrbitalPan in Live's browser.

If your Library does not contain Audio/Plug-Ins yet, open Terminal and run:

mkdir -p "$HOME/Library/Audio/Plug-Ins/VST3"
open "$HOME/Library/Audio/Plug-Ins/VST3"

MACOS MAY BLOCK THIS TEST BUILD

This build is ad-hoc signed, but it does not have an Apple Developer ID
signature or Apple notarization. macOS may block it after downloading.
If it is blocked, only for the copy you received directly from the developer:

1. Quit Live.
2. Open Terminal and run this command for this plugin only:

xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/VST3/OrbitalPan.vst3"

3. Reopen Live and hold Option while clicking Rescan.

This removes the download quarantine attribute from this one trusted test
plugin. It does not disable Gatekeeper for your Mac. If it still fails,
send the exact error to the developer; do not change system security settings.

WHAT TO TEST

- Width: 0% should add no widening; turn it up to spread the sound.
- Check mono playback while changing Width: the added width should cancel.
- Scope: blue = below 250 Hz; green = 250 Hz to 4 kHz; red = above 4 kHz.
- Compare IN and OUT, turn Scope off/on, and stop/start playback.
- Move the orange point and automate Width, Depth, Mix, and position.
- Save, close and reopen your project. Check that settings are restored.
- Report crashes, clicks, unexpected volume changes, or excessive CPU use.

Neutral output is about 3 dB quieter than version 0.1 because a gain bug was
fixed. Reverb timing was also corrected. Leave headroom when widening.
Keep your original project and old plugin available while comparing versions.

Send feedback with your Mac chip, macOS version, Live version, sample rate,
buffer size, what you were doing, and SOURCE-COMMIT.txt from this download.

The automated suite runs on the build Mac's native CPU. Both CPU architectures
are included, but this does not replace testing in Live on your own machine.
