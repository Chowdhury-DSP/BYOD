# BYOD User Manual

Build-Your-Own-Distortion (BYOD) is an audio plugin that allows
the user to create custom guitar effects, with a focus on guitar
distortion. The plugin contains emulations of guitar distortion
and tone-shaping circuits from various, along with a handful of
other useful effects. BYOD is currently available for Windows,
Linux, Mac, and iOS in the following formats: VST/VST3, AU, LV2,
AAX, AUv3, and Standalone.

## Installation

To install BYOD for desktop, download the plugin installer
from the ChowDSP website. If you would like to try the latest
changes (potentially unstable), you can download the latest
Nightly build. It is also possible to compile the plugin from
the source code. BYOD for iOS can be downloaded from the App
Store.

## Getting Started

BYOD is primarly comprised of a "processing chain", made up
of multiple "processors", and "cables" which route signal between
the processors.

### Creating A Processor
To create a new processor, either right-click on the plugin
background, or click on the "plus" icon in the upper right corner.
A menu will appear, offering a selection of processor to be created.

@TODO: GIF

### Removing A Processor


## Global Controls

The bottom bar of the plugin (or top bar on iOS) contains several
"global" controls. Note that these controls are not saved when
saving a preset.

### Undo/Redo
The undo/redo controls will undoor redo the following actions:
- Adding/removing/replacing a processor
- Creating/destroying a cables
- Changing a parameter

### Input Mode
Input mode: selects which channel(s) will be used as the input
to the processor chain. In order to save computing resources (CPU),
it is recommended to avoid using "Stereo" mode except when the
input is a stereo signal.

## Input/Output Gain
These controls can be used to affect the gain staging before or
after the processor chain.

### Oversampling
The oversampling menu can be used to control the amount of oversampling
used by the processor chain. There are options for minumum phase
or linear phase oversampling, up to a factor of 16x. There are also
options to use a different oversampling configuration for offline renderring.

### Settings
The "cog" icon on the far right of the global controls opens a "Settings"
menu. Note that the settings provided in this menu are shared across all
instances of the plugin. There are additional options for viewing the plugin
source code, and copying the plugin's diagnostic info.

## Presets
