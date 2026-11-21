# MK NOISE RhYTM Machine

JUCE-based MIDI noise instrument with three independently selectable noise layers, labeled Level / Pan / Pitch Shifter sliders, independent one-knob effects, and a Gate12-based curve gate.

Place it on a MIDI instrument track and draw notes in the piano roll. Noise sounds only while at least one MIDI note is held, with a 5 ms attack/release to avoid clicks. Overlapping notes keep the noise playing until the last note ends. Notes trigger the noise without changing its tuning. The note envelope also silences effect tails after release; the trance gate remains the last effect in the chain.

## Interface

The editor is a green analog hardware panel drawn from the SVG artwork in `analog_plugin_svgs`. All twelve assets are embedded into the binary with `juce_add_binary_data`, so the plugin needs no external files at runtime: `background.svg` draws the chassis and the three screwed panel plates, `knob_bg.svg` and `knob_cap.svg` draw every rotary dial (the cap rotates with the value), `hslider_bg.svg`/`hslider_thumb.svg` and `vslider_bg.svg`/`vslider_thumb.svg` draw the faders, `toggle_on.svg`/`toggle_off.svg` draw the switches, `dropdown.svg` draws the menus, `readout.svg` draws every value display, and `waveform_display.svg` is the CRT screen behind the editable gate curve.

`Source/AnalogLookAndFeel.*` implements the skin while keeping the stock JUCE slider layout, so parameter behaviour, automation and accessibility are unchanged. Controls are laid out in the 960 × 1076 design space of `background.svg` and scaled to the current window, so the panel, fonts and artwork stay aligned at any size. The window is resizable with the design aspect ratio locked: drag the lower-right corner to scale the whole interface from 70% to 170%.

## Effects

Each layer's Pitch Shifter processes its generated audio over a -24 to +24 semitone range, before pan and mixing. It uses two crossfaded, variable-speed delay read heads with preallocated 60 ms grain windows; it is no longer a noise-generation frequency/color control. Pitch changes are smoothed. At zero semitones the shifter bypasses exactly; shifted attacks can be delayed/smeared by the grain window. White noise has no fundamental pitch, so pitch shifting is easiest to hear on mechanical/electrical noise.

Each effect has its own on/off switch and knob. The global Pitch Shifter adds -12 to +12 semitones to the three layer shifters; other knobs adjust their own effect amounts. Zero amount preserves dry audio. The window is freely resizable: the editor keeps its design aspect ratio and scales every control, font and asset proportionally between 70% and 170%. A host that forces a shorter window, including iPad, gets vertical scrolling instead; dragging sliders or curve points edits them rather than scrolling.

The previous shared FX Amount, LFO Rate, Delay Time and Stereo Width controls are no longer shown. Their parameter IDs remain for compatibility with existing projects; modulation rate and delay time retain their saved settings (defaults 0.3 Hz and 250 ms).

## Gate12 integration

The full upstream [Gate12 source](https://github.com/tiagolr/gate12) is copied under `ThirdParty/Gate12`, pinned to commit `df652453236fa03859dbd5fb8eb3dde1fb574ab5` (version 1.3.3). The plugin compiles and directly calls its original `Pattern` curve functions, rather than approximating them.

- Drag the 8–32 curve points to edit level and time. New presets use exact, freely positioned points; endpoints stay at the loop boundaries and interior points cannot cross their neighbors. Changing the point count adds points within existing segments or removes the least significant points. Reducing a complex curve can simplify it. Old saved projects retain their original grid-cell positions.
- Choose Hold, Curve, S-curve, Pulse, Wave, Triangle, Stairs, Smooth Stairs or Half Sine. Curve at zero tension is linear; Tension shapes the chosen segment type.
- Cycle controls the full envelope length. A `1/4` cycle is one quarter-note beat; `1/8` is half a beat. Triplet and dotted cycles are available.
- MIDI Retrigger restarts the gate on each note-on. With retrigger off, the gate follows host PPQ position, including song seeks. Without host timing it runs at 120 BPM.
- Swing is a tall vertical slider in the lower-left of the gate panel. It delays offbeat sixteenth notes: 0% preserves straight timing, and 100% gives a 2:1 long/short pair. Eighth-note boundaries remain on time. Swing changes are smoothed over 20 ms and apply to the whole gate, including stereo/filter gating and MIDI retrigger. The playback cursor follows the swung timing. Swing is saved with the project and is not reset by selecting a preset; older projects default to 0%.
- Attack and Release smooth the gate transitions. Selecting a preset in the plugin menu sets its curve, point count, cycle, smoothing, filter and stereo mode; drawing edits the current curve without replacing it when the editor reopens.
- Preset-menu refresh follows changes to the saved selection without interrupting pending mouse/touch selections. The menu ignores viewport drag-scrolling so selecting a preset on iPad does not drag the surrounding panel.
- Filter Gate uses an actual two-pole (12 dB/oct) lowpass swept by the gate envelope, together with amplitude gating. Linked / Alternating / Ping Pong settings control stereo gating. Ordinary presets reset filter/stereo options; all settings and edits are saved.

### Preset interpretations

All supplied names retain their original order. The curves are original musical interpretations of their names, not copies of another plugin's factory presets. A short description appears beside the gate controls.

- `Bt` means a fraction of a beat: `1/4 Bt Gate` repeats four times per beat. Musical note names mean note lengths: `1/4 Classic Gate` repeats once per beat. Triplets multiply the named note duration by 2/3; dotted notes by 1.5.
- Classic gates are 50% open, trance gates 25%, pulses 12.5%, and micro gates 4%. Offbeat gates open in the second half of their cycle.
- Sidechain pumps duck on the beat and recover. Soft pumps retain a 45% floor; deep and hard pumps recover increasingly late. Reverse pumps fade down; ghost sidechain retains a 70% floor.
- Swing uses a two-note pair with 2:1 onset spacing. Alternating gates alternate strong/weak hits. Choppers subdivide a cycle into four equal chops; machine gun into eight narrow shots; stutters use three decaying repeats and a rest.
- First Step and Two Step open the first one/two sixteenth-note steps of a bar; End Gate opens only the final step. End Fade fades in the last beat. Other fades span a bar; Fade Out In reaches silence halfway through.
- Speed Up / Slow Down change successive pulse intervals monotonically. Slow and fast tremolo repeat every two beats / every sixteenth note. AM is unipolar 32 Hz modulation at 120 BPM; Copter is an accented double throb at 16 hits/second.
- The trance-style, drum and glitch names have individually authored accent/rhythm patterns. Random/glitch patterns are deterministic on recall; Randomize Gate generates a fresh editable pattern.
- Stereo Alternating switches left/right every quarter note. Ping Pong uses shorter alternating hits with rests. Mono output sums the gated stereo channels.

The complete upstream desktop interface, audio/sidechain triggering, external MIDI/CV output, filesystem preset tools and 12-pattern sequencer are not embedded. The noise-machine interface wraps the shared Gate12 curve engine for desktop/iPad use.

### Licensing and upstream modifications

Gate12 is copyright tiagolr/tilr and licensed under GPLv3; see `ThirdParty/Gate12/LICENSE`. Its GPLv3 requirements apply to this Gate12-derived integration. Preserve the upstream notices and provide the corresponding source and license when distributing binaries. The VST3 build includes the Gate12 license in its Resources folder. Distribution also requires an appropriate JUCE license or compliance with JUCE's open-source terms.

Local modifications to Gate12: `Pattern.cpp` includes `Globals.h` instead of the unrelated desktop plugin processor; `Pattern.h` explicitly includes `<cstdint>`; point/version counters use atomics to avoid concurrent-instance races. The curve algorithms are unchanged. The integration uses fixed parameter snapshots and calls segment functions directly, avoiding the upstream `get_y_at` mutex on the audio thread.

## Build

Windows (VST3 and Standalone):

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --target MKNoiseRhythmMachine_VST3 MKNoiseRhythmMachine_Standalone
```

On macOS, CMake also configures VST3, AUv3, and Standalone targets. For iOS/iPadOS it configures the AUv3 plus a Standalone host app that embeds the app extension, so one build produces an installable iPad app. AUv3 requires Apple's Xcode toolchain and cannot be built on Windows.

Build the iPad AUv3 and its host app from macOS with Xcode:

```sh
cmake -S . -B build-ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphoneos -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build-ios --config Release --target MKNoiseRhythmMachine_Standalone
```

`MKNoiseRhythmMachine_AUv3` builds the app extension on its own; the Standalone target depends on it and embeds it under `PlugIns/`, so the resulting app is a ready AUv3 host.

### GitHub Actions (iOS AUv3)

`.github/workflows/ios-auv3.yml` builds the AUv3 and its host app on a `macos-15` runner and publishes:

- `MK-NOISE-RhYTM-Machine-<version>-ios.ipa` (or `-ios-unsigned.ipa` when signing secrets are absent)
- `MK-NOISE-RhYTM-Machine-<version>-AUv3.appex.zip` for embedding in another host
- `Gate12-LICENSE.txt` (required by the GPLv3 integration)

Run it from **Actions → iOS AUv3 → Run workflow** (deployment target defaults to `17.0`, plus an optional signed build), push a `v*` tag to attach the artefacts to a GitHub release, or let it run on pull requests that touch the plugin sources.

An unsigned IPA must be signed before installation (Xcode, Apple Configurator, or a sideloading tool with a development certificate). For signed builds, add these repository secrets; tag builds then sign automatically and manual runs can request it with the "signed" input:

| Secret | Purpose |
| --- | --- |
| `APPLE_TEAM_ID` | Apple Developer team identifier |
| `APPLE_CERT_P12_BASE64` | base64-encoded signing certificate `.p12` |
| `APPLE_CERT_PASSWORD` | password of that `.p12` |
| `APPLE_API_KEY_P8_BASE64` | base64-encoded App Store Connect API key `.p8` |
| `APPLE_API_KEY_ID` | key identifier of that API key |
| `APPLE_API_ISSUER_ID` | issuer identifier of that API key |

The project uses the JUCE checkout in the parent `JUCE` folder when available; otherwise CMake fetches JUCE 9.0.5. Set `MK_JUCE_SOURCE_DIR` to use a different local JUCE checkout.

Build `MKNoiseRhythmMachineTests`, then run `ctest --test-dir build -C Release --output-on-failure`. Tests cover MIDI-triggered silence, overlapping notes, measured pitch-shift frequency ratios, all named preset definitions, gate contour/timing semantics, real stereo/filter gating, saved-state compatibility, point-count changes, resizable aspect-locked scaling, enlarged drawable effect dials, short-window scrolling, plus the existing Gate12/tempo/effect regressions.
