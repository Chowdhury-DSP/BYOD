# BYOD (Build-Your-Own Distortion)

![CI](https://github.com/Chowdhury-DSP/BYOD/workflows/CI/badge.svg)
[![License](https://img.shields.io/badge/License-BSD-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

BYOD is a guitar distortion plugin with a customisable
signal chain that allows users to create their own guitar
distortion effects. The plugin contains a wide variety
of distortion effects from analog modelled circuits
to purely digital creations, along with some musical
tone-shaping filters, and a handful of other useful
processing blocks.

## TODO
- New processors:
  - klon drive stage
  - hysteresis
  - wavefolder
  - tremolo
  - envelope filter
- expose processor parameters to DAW?

## Building

To build from scratch, you must have CMake installed.

```bash
# Clone the repository
$ git clone https://github.com/Chowdhury-DSP/BYOD.git
$ cd BYOD

# initialize and set up submodules
$ git submodule update --init --recursive

# build with CMake
$ cmake -Bbuild
$ cmake --build build --config Release
```

## Credits:

- GUI Framework - [Plugin GUI Magic](https://github.com/ffAudio/PluginGUIMagic)
- Extra Icons - [FontAwesome](https://fontawesome.com/)

Credits for the individual processing blocks are shown on
the information page for each block. Big thanks to all who
have contributed!

## License

BYOD is open source, and is licensed under the BSD 3-clause license.
Enjoy!
