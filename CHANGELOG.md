# Changelog

All notable changes to this project will be documented in this file.

## [UNRELEASED]
- Added "Smoothing" parameter for "Muff Drive" module.
- Added new factory presets.
- Fixed parameter name changes not showing up in some CLAP hosts.

## [1.1.0] 2022-11-21
- Added support for the CLAP plugin format (with parameter modulation).
- Added modulation ports so modules with internal modulation can share modulation signals.
- Added "Rotary" module.
- Added "Panner" module.
- Added "Scanner Vibrato" module.
- Added "Flanger" module.
- Added "Phaser4" and "Phaser8" modules.
- Added "Junior B" module.
- Added "Bass Face" module.
- Added "Blonde" modules.
- Added "Smooth Reverb" and "Shimmer Reverb" modules.
- Added "Octaver" module.
- Added "Neural" mode to Centaur module.
- Added "Multi-Mode" option to SVF module.
- Added "High-Quality" options for Range Booster and Muff Drive modules.
- Added "Extend" and "Invert" options for Clean Gain module.
- Added support for Proteus models with a conditioning input for GuitarML module.
- Added new factory presets.
- Added UI control to replace a wire with a 1-input/1-output module.
- Added UI control to replace a 1-input/1-output module with a wire.
- Added UI size to the saved plugin state.
- Added parameter context menus for AAX and VST3 plugins formats.
- Improved neural network-based modules performance and sound quality at different sample rates.
- Improved UI rendering performance.
- Fixed cutoff frequency behaviour in High Cut module.
- Fixed gain normalization across sample-rates for IR modules.
- Fixed undo/redo not restoring preset name.

## [1.0.1] 2022-03-15
- Changed UI rendering to use OpenGL by default on Windows/Linux, unless OpenGL 2.0+ is not available on the host system.
- Fixed YenDrive giving NaN output when Gain parameter set to zero.
- Fixed Spring Reverb "pop" when switching from mono to stereo input.
- Fixed Metal Face "stuttering" behaviour at higher sample rates.
- Fixed plugin state not saving/loading correctly in VST2.
- Fixed noise spikes in King Of Tone, RONN, and Muff Drive processors.

## [1.0.0] 2022-03-11
- Initial release.
