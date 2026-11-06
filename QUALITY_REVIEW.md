# OrbitalPan 0.2 quality review

## Implemented

| Finding in 0.1 | Change and practical effect |
| --- | --- |
| No stereo width control | Added an automatable, saved Width dial in the bottom row. Range is 0–200% added width, default zero; original stereo is preserved at zero. A phase-decorrelated mid signal creates width from dual-mono audio, and the existing high-passed side signal is expanded. |
| Mono compatibility needed for widening | Width adds equal-and-opposite side information after the spatial processing. The mono sum therefore matches the same chain with Width zero, within floating-point precision. The fixed 120 Hz high-pass attenuates added low-frequency width. This is an original implementation, not Polyverse's algorithm. |
| Every scope point was blue | Full-rate, fourth-order Linkwitz–Riley analysis splits both channels at 250 Hz and 4 kHz before reducing display traffic. Three simultaneous coloured traces show the stereo distribution of the bands. Crossovers overlap rather than assigning each sample a hard frequency label. |
| Scope coordinates were raw L/R | Use conventional rotated stereo coordinates, with mono vertical and side information horizontal. The XY control still positions the effect independently of the scope. |
| Scope retained samples indefinitely, and inactive source queues filled | Fixed 350 ms trail lifetime; clear trails when switching sources. Drain the other source, discard queued history on opening/toggling, and disable analysis when the editor is closed or Scope is off. |
| Scope storage resized/reset while the UI could read it | Fixed-capacity single-producer/single-consumer storage. Preparing audio only resets producer-owned filter state, leaving consumer indices/storage intact. A full queue drops new display frames without blocking audio. |
| Reverb never received host sample rate | Call `setSampleRate` during preparation, fixing reverb timing above/below 44.1 kHz. Report a four-second tail rather than one second. |
| Neutral settings unintentionally boosted signal by about 3 dB | JUCE Reverb multiplies its dry setting by two; compensate and normalise centre balance to unity. Neutral position, Width zero now passes audio at unity, and Mix zero is exactly dry. |
| Audio callback resized/copied buffers and allocated shelf coefficients | Fixed 32-sample stack scratch space and fixed-storage biquads. No dynamic buffer resizing or reference-counted coefficient creation inside processing. Handles blocks larger than the host's advertised maximum. |
| Tone/reverb updates depended on host block length | Update from smoothed controls every 32 samples; retain the interval across host blocks. Width ramps over 50 ms, like the existing parameters. |
| Older sessions have no Width parameter | Append Width to the parameter list and migrate absent Width to zero, including when loading an old preset after a newer preset. Existing IDs and order are preserved. |
| Local and downloaded framework versions differed | Pin the fallback to JUCE 8.0.12, matching the documented local build. Add optional regression tests and an option to build without automatically installing. |

## Verification

Windows x64 Release VST3 built successfully with MSVC 19.44 and JUCE 8.0.12. CTest passed (1 suite, 0 failures), and the software-rendered editor preview was inspected for the three frequency colours and bottom controls. The build was staged separately and did not overwrite the installed plug-in.

The automated suite exercises:

- Zero-width transparency, mono-sum preservation on random stereo input, stereo generation from mono, and attenuation of added 40 Hz width.
- Neutral unity gain and exact dry output, including empty, single-sample, irregular and oversized buffers.
- The complete processing chain with pan, depth, reverb, mix and Width automation, comparing its mono sum against Width zero.
- New-state round-trip and old-state migration after a nonzero Width setting.
- Blue/green/red separation using 80 Hz, 1 kHz and 10 kHz tones; preservation of simultaneous left/right sample pairs; queue wrap, overflow and discard.
- Sample rates of 44.1, 48, 96 and 192 kHz.

Build and test commands are in the README. A rendered editor preview uses synthetic three-band audio rather than a DAW capture.

## Compatibility and remaining product work

The gain and reverb corrections intentionally change the sound of existing sessions: neutral output is about 3 dB lower than the old steady-state output, and reverb timing now follows the host sample rate. Width itself defaults to zero. Compare existing mixes before replacing a release in a production session.

The current XY horizontal control remains a stereo balance control: moving to an extreme attenuates the opposite input channel rather than folding both inputs into the selected channel. A true stereo positioning mode would require a separate design decision. Widening after balance can also spread a hard-panned signal back across the stereo field.

Before release, audition width on vocals, transient percussion, sustained bass and complete mixes; check headroom, off-centre placement and mono playback in Ableton. Stereo peaks can rise with widening or front-position gain; there is no output trim, peak meter or limiter. Output metering/trim would be useful next additions. Automatic limiting is intentionally absent because it would change dynamics and the mono-sum guarantee.

Still needed for distribution: DAW automation/editor lifecycle soak testing, pluginval or Steinberg host validation, macOS builds/testing, accessible keyboard control of the XY pad, scalable UI, and signed installers. Automated DSP tests and an editor snapshot do not replace listening tests or host validation.

Reference: [Polyverse Wider product page](https://polyversemusic.com/products/wider/) and [Wider manual](https://polyversemusic.com/docs/wider-manual/) describe mono-compatible widening and low-end exclusion. OrbitalPan implements its own algorithm and a fixed bass-protection cutoff, not all Wider features.
