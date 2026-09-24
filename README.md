# Acoustic Guitar by JGK V3.3 — Full Build

A Windows x64 VST3 + Standalone acoustic-guitar instrument for FL Studio and compatible hosts.

## Main features
- Real CC0 Martin HD28 steel-string samples
- MIDI chord detection
- Major, Minor, 7, Maj7, Min7, Sus2, Sus4, Add9
- Guitar voicing and inversion controls
- Capo 0–12
- Down / up / palm-muted strumming
- Strum speed separate from rhythmic pattern speed
- Humanize, dynamics, tone, room, swing, tight/loose and output controls
- STRUM / FINGERPICK / PERCUSSION performance modes
- Scratch and guitar-body percussion
- Choke events
- 8-step editable rhythm pattern
- 8 rhythm presets
- Chord pads
- Latch / Hold / MIDI behaviour
- Strum variation
- Live chord display and fingering diagram
- Procedural dark-walnut / sunset JGK interface
- Visual capo rests on the paper when OFF and moves to the neck when enabled
- VST3 and Standalone builds

## Stability changes vs the failed V3/V3.1 build
- Built on the same minimal JUCE structure proven to load in FL Studio in the V3.2 diagnostic
- New VST3 identity: `com.jgk.acousticguitar.v33` / `Ag33`
- Static MSVC runtime
- No PNG/UI image decoding at plugin startup
- Fixed-size editor
- Strict V3.3-only state loading; missing/old version state is rejected
- Safer sample validation and sample-rate guards
- Standalone target included for troubleshooting

## Samples
The sample set is the CC0 2017 Martin HD28 Vintage Series by Jeff Learman,
from `sfzinstruments/Discord-SFZ-GM-Bank`, pinned to commit:

`05d5ed8befa042fd9d99a6d159dfc3673d3f8edc`
