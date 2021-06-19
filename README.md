# BYOD (Build-Your-Own Distortion)

![CI](https://github.com/Chowdhury-DSP/BYOD/workflows/CI/badge.svg)
[![License](https://img.shields.io/badge/License-BSD-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

This repository contains template code for building a Chowdhury DSP
audio plugin.

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

## License

BYOD is open source, and is licensed under the BSD 3-clause license.
Enjoy!
