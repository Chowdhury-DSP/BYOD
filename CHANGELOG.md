# Changelog

All notable changes to this project will be documented in this file.

## [UNRELEASED]
- Changed UI rendering to use OpenGL by default on Windows/Linux, unless OpenGL 2.0+ is not available on the host system.
- Fixed YenDrive giving NaN output when Gain parameter set to zero.
- Fixed Spring Reverb "pop" when switching from mono to stereo input.
- Fixed Metal Face "stuttering" behaviour at higher sample rates.
- Fixed plugin state not saving/loading correctly in VST2.
- Fixed noise spikes in King Of Tone, RONN, and Muff Drive processors.

## [1.0.0] 2022-03-11
- Initial release.
