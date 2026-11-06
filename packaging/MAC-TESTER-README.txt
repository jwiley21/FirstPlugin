OrbitalPan v@PROJECT_VERSION@ - Mac tester build @ORBITALPAN_EFFECTIVE_BUILD_ID@
============================

This download contains a compiled VST3 plugin for Intel and Apple silicon
Macs running macOS 11 or later. No Xcode, Homebrew, or compilation is needed.
The DAW must also support your macOS version. This is a test build.

IDENTIFY YOUR VERSION

The ZIP/folder name includes the version and a unique build ID. BUILD-INFO.txt
lists the full details. After installation, the plugin's top row must match
that version and build. Send that information with every bug report.

The actual plugin bundle also includes the version/build in its filename:

@ORBITALPAN_PACKAGE_PREFIX@.vst3

All versions still identify as OrbitalPan inside Ableton so existing projects
can use the update. Move earlier OrbitalPan bundles OUT of the scanned VST3
folders before installing this one. Different filenames do not make them
independent plugins. Keep previous ZIPs/backups somewhere else.

INSTALL IN ABLETON LIVE

1. Quit Ableton Live.
2. Unzip the download. Keep the versioned .vst3 as a complete bundle;
   do not copy files out of its Contents folder.
3. In Finder, press Command+Shift+G, paste this path, and press Return:

   ~/Library/Audio/Plug-Ins/

4. Open the VST3 folder, or create a folder named VST3 if it is missing.
5. Move ALL older OrbitalPan*.vst3 bundles outside the plugin folders as backups.
   Check both locations (the second is system-wide):

   ~/Library/Audio/Plug-Ins/VST3/
   /Library/Audio/Plug-Ins/VST3/

   Copy this new bundle into the first location:

   @ORBITALPAN_PACKAGE_PREFIX@.vst3
6. Open Live > Settings > Plug-Ins. Enable Use VST3 Plug-In System Folders.
   Rescan and search for OrbitalPan in Live's browser.
7. Open the plugin and compare its top-row version/build with BUILD-INFO.txt.
   If they differ, check for an older duplicate in this system-wide folder:

   /Library/Audio/Plug-Ins/VST3/

   Keep only the new copy in the scanned folders. Quit/reopen Live and hold
   Option while rescanning. A plugin without a version header is an older build.

If your Library does not contain Audio/Plug-Ins yet, open Terminal and run:

mkdir -p "$HOME/Library/Audio/Plug-Ins/VST3"
open "$HOME/Library/Audio/Plug-Ins/VST3"

MACOS MAY BLOCK THIS TEST BUILD

This build is ad-hoc signed, but it does not have an Apple Developer ID
signature or Apple notarization. macOS may block it after downloading.
If it is blocked, only for the copy you received directly from the developer:

1. Quit Live.
2. Open Terminal and run this command for this plugin only:

xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/VST3/@ORBITALPAN_PACKAGE_PREFIX@.vst3"

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
buffer size, what you were doing, and BUILD-INFO.txt from this download.

The automated suite runs on the build Mac's native CPU. Both CPU architectures
are included, but this does not replace testing in Live on your own machine.
