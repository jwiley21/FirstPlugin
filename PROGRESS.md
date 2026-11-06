# OrbitalPan progress and AI handoff

Last updated: 2026-11-06. Read this before working; history is in CHANGELOG.md.

## Current state

- Source version: **0.2.2**, Mac compatibility/package verification update.
- Repository: https://github.com/jwiley21/FirstPlugin, branch `main`.
- Confirmed prior Mac artifact: **0.2.1-b4.1-881a368**, successful run:
  https://github.com/jwiley21/FirstPlugin/actions/runs/42189669734.
  The previous handoff's claim that 0.2.1 was unpushed was stale.
- Product: JUCE 8.0.12 / C++17 stereo VST3. Windows developer, Mac/Ableton tester.

## Current task: Mac installation and a distinct tester update

Tester saw a grey file and an approximate file format/type error. Exact error,
step, chip, macOS and Live version are unknown. No root cause or successful
Ableton install/listening test is confirmed.

Implemented for 0.2.2:

- Intel deployment target lowered from 11.0 to 10.13; arm64 stays at 11.0.
- Host smoke test scans compiled VST3, verifies name/version, instantiates,
  processes stereo audio, restores state, and creates its editor.
- Mac CI runs audio regressions natively and under x86_64 Rosetta.
- Extracts final ZIP, verifies identity/version, executable bit, signature,
  architectures, per-slice minimum OS and system-only dependencies; loads the
  actual versioned extracted bundle in both architecture modes.
- Expanded installation and grey-file/wrong-download troubleshooting.
- Version/build stays on ZIP/folder/.vst3/header; IDs/parameters/DSP unchanged.
  ZIP remains the delivery format as the user requested.

Checks so far:

- Local YAML parsed, all five Bash steps passed bash -n, Python syntax passed.
- git diff --check passed.
- Windows build attempt blocked: VS 2026 is present but development environment
  discovery fails and compiler headers are missing. No new Windows test success.
- Mac checks await GitHub. Do not infer success from configured tests.

## Remaining work and constraints

- Build/download 0.2.2 in GitHub; record actual result and build ID here.
- Confirm installation/playback on the client's actual macOS/Live version.
- Targeting 10.13 does not prove every old Intel Mac works. Rosetta tests do not
  replace native Intel hardware or older-macOS testing.
- Ad-hoc signed, not Developer ID signed/notarized. Download quarantine may
  still require the narrow plugin-specific workaround in READ-ME-FIRST.txt.
- VST3 needs Live 10.1+ (native Apple silicon Live 11.1+).
- Keep one OrbitalPan*.vst3 across both user/system scanned folders. Move older
  versions elsewhere because all share one internal plugin identity.
- Prior 0.2.0 Windows DSP results are historical; DJ listening, automation,
  CPU, mono playback and project recall are still unconfirmed.
- Old sessions may change level/timing versus 0.1.0 due to 0.2.0 bug fixes.

## Feedback to collect

- Version/build from header or BUILD-INFO.txt and exact downloaded filename.
- Mac chip, macOS and Live versions; exact error and step where it appeared.
- Sample rate/buffer, audio/settings, expected/actual result, reproduction.
- Outcome/evidence: unconfirmed / reproduced / fixed / tester-confirmed.
