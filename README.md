# PSP Mod Base

This is a pair of plugins and a collection of basic libraries for making game modding and code injection easier.

The goal is simple - make game modding with custom code easier on PSP!

You may freely use this as a basis for a plugin to build mods for PSP games and apps.

## Usage

Intended usage is either as a Visual Studio solution or as a regular makefile project.

In case you're using `make`, please make sure to set up the [PSPDEV Toolchain!](https://pspdev.github.io/)

### PSPDEV side-note

(TODO: write a shell script to automate this!)

(You may skip this step it you're using Visual Studio and/or the pspsdk submodule)

Be sure to get the CFW API libraries and headers. You'll need this regardless of if you use CFW or not. PSPSDK does not come with this pre-included.

The easiest place to acquire them currently is from the uofw/uofwinst repository:

https://github.com/uofw/uofwinst/tree/master/include

https://github.com/uofw/uofwinst/tree/master/libs

From that repository, place include headers (all the .h files) into `$PSPDEV/psp/sdk/include` and libraries (all the .a files) into `$PSPDEV/psp/sdk/lib`

### Windows

- Clone the repository with `--recurse-submodules`

- Open PSPModBase.sln in Visual Studio

- Open source/PSPModBase/main.c of the PSPModBase project

- Build as per usual

### Make

- Clone the repository without submodules

- Install and set up [PSPDEV Toolchain](https://pspdev.github.io/) if you hadn't already!

- Try to do: `make -C source/PSPModBase`

If all is well, you should end up with an elf and a prx in the `build` folder!

Now you can modify the code to your needs and liking.

## PSP CFW usage

In case you wish to run the userspace module (PSPModBase.prx), you will need to use the bootstrap.

Included is code for a kernel plugin called `PSPModBaseBoot`.

You have to modify its code to match your needs.

### Modifications for bootstrapper

The minimal modifications that are required are:

1. Make sure `MODULE_NAME_INTERNAL` definition matches the one in PSPModBase!

2. Make sure to name `MODULE_BOOT_TARGET` the same as your PRX filename!

By default this is already set up for `PSPModBase` but as you rename your project, you have to address these 2 things to get it working.

### Runtime

In order to actually run your plugin, copy both your main plugin (PSPModBase.prx) and the bootstrapper (PSPModBaseBoot.prx) to the PSP. 

**NOTE** - these files must be placed next to each other!

Then, in your CFW use PSPModBaseBoot.prx as the plugin for GAME (or your specific GameID if you're using a CFW such as ARK-4).

### Recommended plugins for PSP debugging

- [PSPLINK](http://pspdev.github.io/psplinkusb/) - required for runtime debugging. Enable it by adding "psplink.prx" to the plugins list. Also make sure that USB Charging is NOT enabled.

- [libkprintf](https://github.com/Linblow/libkprintf) - an addon which hooks kernel printf functions to restore its functionality via sio and psplink. Use this in combination with psplink to get print output via pspsh. Keep in mind that it may take it a bit to hook in so you might not catch early prints.

## PPSSPP usage

Simply copy the files to: `memstick/PSP/PLUGINS/(plugin_name)`

You also need to create a plugin.ini (example included in the data folder).



Optionally, if you want the build script to automatically copy files for each new build, define the `PPSSPPMemstick` environment variable in your system and the build script should automatically copy new builds directly into your emulator memstick folder.

## Additional Info

This code is highly based on the [Widescreen Fixes Pack](https://github.com/ThirteenAG/WidescreenFixesPack) PSP plugins. 

For more examples on what you can do with this base, you may explore the code for PPSSPP plugins such as [GTA Vice City Stories](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTAVCS.PPSSPP.WidescreenFix/main.c).

### PSPSDK cygwin toolchain

Included as a submodule (`external/pspsdk`) is a PSPDEV toolchain built under cygwin. This is mainly intended for Windows users. 

It is a minimal configuration that can run only the toolchain, but you may also launch bash by running `launch-bash.ps1` from Powershell. Certain things are currently broken (such as `psp-pacman`) due to cygwin's inherent incompatibilites.

If you want to invoke `make` standalone in a Windows environment, you may do so by running `external/pspsdk/vsmake.ps1` from Powershell.

And this should go without saying - if you're not building directly under Windows, you do not need to clone this submodule.

## Credits

- Portions of code based on [pspdev/psplinkusb](https://github.com/pspdev/psplinkusb) and [pspdev/pspsdk](https://github.com/pspdev/pspsdk) samples.

- ThirteenAG for [Widescreen Fixes Pack](https://github.com/ThirteenAG/WidescreenFixesPack)
