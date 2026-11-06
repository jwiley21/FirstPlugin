# OrbitalPan: instructions for AI contributors

## Start here

Read `PROGRESS.md` for the current handoff, `CHANGELOG.md` for version history,
and `VERSION` for the next source build's semantic version. Inspect git status
before editing. Preserve unrelated user changes.

## Maintain the handoff

For each substantive task, update `PROGRESS.md` with what changed, checks actually
run and their results, remaining work, and any tester feedback. Mark work as
local, pushed, CI-built, or tester-confirmed only when evidence supports it.
Never invent a successful Mac run, listening test, or publication. Keep the
current handoff concise; put historical release changes in `CHANGELOG.md`.

## Version and download rules

- `VERSION` is the only source of the semantic version (`MAJOR.MINOR.PATCH`).
  CMake reads it for JUCE metadata, the editor header, and package metadata.
- For a new user-facing update after a version has been pushed, increment PATCH
  for fixes or MINOR for features and record the changes in `CHANGELOG.md`.
  Continue using the same version while finishing an unpushed task. Documentation
  or CI-only fixes can keep the semantic version, but must be logged.
- Every Mac workflow run automatically gets a unique identity:
  `b<RUN_NUMBER>.<RUN_ATTEMPT>-<7-character commit>`.
  Do not edit VERSION in CI, commit generated build metadata, or create push loops.
- Use the CMake-generated package prefix and `BUILD-INFO.txt` when packaging.
  ZIP, enclosing folder, .vst3 filename, plugin header, and feedback must agree on version/build.
- The user explicitly requires the actual downloaded .vst3 bundle to include its
  version/build: `OrbitalPan-v<version>-<build ID>.vst3`. Rename the packaged bundle
  using the generated prefix; JUCE's intermediate build bundle can keep its name.
  Preserve PRODUCT_NAME, BUNDLE_ID, manufacturer/plugin codes, and parameter IDs.
  Filename changes must not create a new internal plugin identity.
- Tester installation removes older OrbitalPan bundles from both user/system VST3
  folders before copying the new version. Distinct filenames still share one
  internal plugin identity; do not recommend keeping them installed together.

## Build and verification

Use `-DORBITALPAN_COPY_PLUGIN_AFTER_BUILD=OFF` for development verification.
The Windows build is under `build/quality`; local JUCE is usually `C:/JUCE/JUCE`.
Run the existing CTest suite for DSP/build changes, and inspect a rendered editor
preview for visible layout changes. Validate workflow YAML and shell syntax when
editing CI. Version-only documentation changes do not need new audio tests.

CMake must enable both C and CXX in the top-level project (JUCE includes C files).
On macOS use `lipo "$binary" -verify_arch arm64 x86_64`: the input must precede
`-verify_arch`. The Mac workflow is ad-hoc signed for testers, not notarized.

The Mac tester package targets Intel macOS 10.13+ and Apple silicon macOS 11+.
Verify the actual Mach-O minimum for each architecture; do not infer support
from the runner's OS or a successful compile. Run the existing audio suite
and load the renamed bundle after extracting the final ZIP in both native
arm64 and x86_64 (Rosetta) modes before upload. Verify its signature, executable
permissions, stable bundle identity, version, and system-only dependencies.
Rosetta checks do not establish native Intel hardware or older-macOS coverage.
Ableton must support VST3 (Live 10.1+; native Apple silicon Live 11.1+).
Do not call a greyed file a confirmed binary defect without the exact error.
Keep the Mac download as a ZIP unless the user requests a distribution change:
a macOS .vst3 is a directory bundle, not a single raw downloadable file.

Keep audio processing allocation-free. Preserve zero-width transparency, width's
mono-sum behavior, and legacy state migration. Do not claim that preserving the
width stage's mono sum makes all spatial processing transparent.

Do not commit build outputs or automatically push/publish unless the user's task
authorizes it. Provide Windows PowerShell push commands when requested.
