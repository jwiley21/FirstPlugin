# OrbitalPan progress and AI handoff

Last updated: 2026-11-06. Read this file at the start of a task and update it
before handing work back. Release history lives in `CHANGELOG.md`.

## Current state

- Source version: **0.2.1** (see `VERSION`). Version identification work is local
  and has not yet been pushed or built by GitHub.
- Last known successful Mac tester build: **0.2.0**, after commit `d3c40da`.
  Evidence: the user reported that the workflow worked. Do not infer that the
  DJ has completed audio/automation tests from that report.
- Product: JUCE 8.0.12 / C++17 stereo VST3, named OrbitalPan.
- Repository: https://github.com/jwiley21/FirstPlugin, branch `main`.
- The user develops on Windows; their DJ friend tests in Ableton on a Mac.

## Current task: make tester versions identifiable

Implemented locally:

- `VERSION` is the semantic-version source of truth for CMake/JUCE and the UI.
- Header shows `OrbitalPan v0.2.1` and the exact build ID.
- Every push to main triggers a Mac build with a new workflow number. Reruns also
  get a distinct attempt number: `b12.1-abc1234` is an illustrative example.
- Download and folder names follow:
  `OrbitalPan-v0.2.1-b12.1-abc1234-macOS-Universal.zip`.
- ZIP includes BUILD-INFO.txt, CHANGELOG.md, source commit, and install guidance.
- The user explicitly clarified that the actual .vst3 filename must be versioned,
  e.g. `OrbitalPan-v0.2.1-b12.1-abc1234.vst3`, not only the ZIP filename.
- Internal plugin IDs remain stable; testers must remove prior bundles from both
  scanned VST3 folders because differently named versions still share that ID.
- AI maintenance instructions added to `AGENTS.md`.

Validation completed locally:

- Windows x64 Release VST3 and test executable built successfully with MSVC/JUCE.
- CTest: 1 suite passed, 0 failed. The existing audio tests all passed.
- Inspected `build/quality/OrbitalPan-v0.2.1-preview.png`: version/build header is
  visible and the controls/scope remain correctly laid out.
- VST3 module metadata reports 0.2.1.
- Generated metadata/header/install instructions agree with the .vst3 filename
  for three sample build IDs (new pushes and reruns yield different names).
- Malformed semantic versions and unsafe build IDs are rejected by CMake.
- Workflow YAML and all shell steps pass local syntax checks; git diff check passes.
- Renamed Mac bundle signing/loading and the updated upload still need the next
  GitHub run and a tester check in Ableton. Do not claim these passed locally.

## Completed foundations

- 0.2.0 added Width and frequency-based scope colours; see CHANGELOG.md.
- Windows Release build and audio tests previously passed at 44.1/48/96/192 kHz.
- Fixed two Mac workflow errors: CMAKE_C_COMPILE_OBJECT (enable C at project root)
  and lipo parsing (put binary path before -verify_arch).
- Mac workflow compiles a universal arm64/x86_64 bundle and runs tests natively on
  its Apple silicon runner. Ad-hoc signing is used; Apple notarization is absent.

## Remaining work and known constraints

- Push 0.2.1 and verify the versioned Mac artifact and header match in Ableton.
- Await DJ feedback on width sound, mono playback, gain, CPU, automation, and
  project recall. No completed listening-test report has been provided yet.
- Keep only one OrbitalPan*.vst3 installed across the user's and system's VST3
  folders. Old copies/cached instances were a source of tester confusion.
- Old sessions may change level/timing because 0.2.0 fixed gain and reverb bugs.
- Public distribution still needs signing/notarization and broader host testing.
- Potential next improvements: output trim/meters, keyboard XY control, UI scaling.
  These are backlog ideas, not completed or authorized active tasks.

## Feedback record (fill in when received)

- Version and build ID from the plugin header or BUILD-INFO.txt:
- Mac chip / macOS / Ableton versions:
- Sample rate / buffer size:
- Source audio and parameter settings:
- Expected result / actual result / reproduction steps:
- Outcome: unconfirmed / reproduced / fixed / tester verified:
- Fix version and evidence:
