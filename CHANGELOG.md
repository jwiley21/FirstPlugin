# OrbitalPan version history

Semantic versions describe product changes. Automatic build IDs distinguish
individual pushes and reruns, even when they have the same semantic version.
`VERSION` is authoritative for the current source tree. A version entry records
changes; it does not by itself mean a binary has been published or tested in a DAW.

## 0.2.2 — Mac compatibility and package verification

- Broadened the Intel deployment target from macOS 11 to macOS 10.13;
  Apple silicon still requires macOS 11+. The host's OS requirements also apply.
- Added a VST3 host smoke test for discovery, reported version, instantiation,
  stereo audio processing, state restore, and editor creation.
- Mac CI now runs audio regressions natively and as x86_64 under Rosetta,
  then loads both slices of the versioned bundle extracted from the final ZIP.
- Packaging checks the extracted signature, executable permissions, stable
  bundle ID, version, both Mach-O minimum OS versions, and system dependencies.
- Clarified greyed-out files, wrong downloads, Live's VST3 requirement (10.1+),
  installation, and the existing plugin-specific quarantine workaround.
- Kept the version/build in the ZIP, folder, actual .vst3, and plugin header.
  Internal identity, parameters and DSP are unchanged. Still an ad-hoc signed,
  non-notarized tester build; older macOS and Ableton need tester confirmation.

## 0.2.1 — version identification and tester handoff

- Added a visible version and build ID above the plugin's controls.
- Added one VERSION file shared by CMake, plugin metadata, and download metadata.
- Mac ZIPs and their enclosing folders now include the version, workflow build
  number, attempt number, and source commit abbreviation.
- Downloads include BUILD-INFO.txt and this change history for tester feedback.
- Added AGENTS.md and PROGRESS.md to preserve context between AI sessions.
- The actual downloaded `.vst3` bundle is versioned too, as explicitly requested.
  Internal plugin identity remains stable for saved host references. Remove old
  OrbitalPan bundles from scanned plugin folders before installing an update.
- No audio processing or parameter changes relative to 0.2.0.

## 0.2.0 — stereo width and frequency scope

- Added the 0–200% Width dial with smoothed automation, mono-sum preservation,
  stereo generation from dual-mono material, and attenuation of added bass width.
- Added blue low-frequency, green mid-frequency, and red high-frequency scope
  traces with 250 Hz / 4 kHz crossovers and a frequency legend.
- Corrected vectorscope coordinates, trail expiry, stale queues, and capture use.
- Fixed neutral gain, reverb sample-rate setup, and audio-thread allocations.
- Added regression tests and migration of older presets to Width zero.
- Added a universal Mac tester build workflow.
- Fixed top-level C language configuration (`f4266af`) and lipo argument ordering
  (`d3c40da`). The user subsequently reported a successful Mac workflow run.
- Compatibility: neutral output is approximately 3 dB lower than 0.1.0 because
  of the gain correction; reverb timing now follows the host sample rate.

## 0.1.0 — original plugin

- Stereo XY positioning with Depth, Mix, front/back tone and reverb.
- Single-colour input/output stereo scope.
