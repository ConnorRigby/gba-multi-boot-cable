# GBA MULTIBOOT LOADER

Template repository to get started executing multiboot payloads on a Nintendo
GameBoy Advance. Most of this code was taken from https://github.com/cartr/MSMCcable.
I just cleaned it up, updated the gba-multi-cable to compile with more recent 
version of Arduino. I also updated/created a set of Makefiles to make everything
compile correctly. 

See below for description of each directory and the projects within it.

## Folder descriptions

### `2ndloader`

Small GBA program to load a larger GBA program faster using the GBA's UART
mode. This loader is loaded initially via GBA's multiboot protocol using the `gbl`
tool.

### `gba-multi-cable`

Small Arduino program that interacts with `gbl` to first load `2ndloader`, then
loads the "real" payload over the UART interface.

### `template_mb`

Example GBA program that can be bulit and uploaded using the tools.

### `main.c`, `multi.c`, `serxfer.c`

Simple linux program to interact with `gba-multi-cable` and `2ndloader`

## Building everything

To build everything, you will need some dependencies:

* `platformio` for the `gba-multi-cable` project
  * `pip install platformio`
* `devkitpro` for the `2ndloader` and `template_mb` projects
  * [installation instructions](https://devkitpro.org/wiki/Getting_Started)
* standard linux utilities
  * `make`
  * `gcc`
  * `perl`

Once everything is installed, simply call `make` to build everything. If it was 
successful, call `make burn` to burn the `gba-multi-cable` project to the arduino
Nano. (the arduino must be plugged into the PC). PlatformIO should auto detect
the serial port. After that wire up your multiboot cable, then you can use the
tool:

```bash
./gbl -p/dev/ttyUSB0 template_mb/template_mb.gba
```

## Wiring

Below is a table for wiring up a multi-boot cable. Note the colors may differ
for your particular cable.

| GBA function | GBA Color (?) | Arduino pin |
|--------------|---------------|-------------|
| SI           | red           | 8           |
| SO           | yellow        | 9           |
| SD           | brown         | 10          |
| SC           | white         | 11          |
| VCC          | blue          | NC          |
| GND          | green         | GND         |
| LED          | NC            | 13          |

## License

See each folder for it's individual license. The original repository also contains
license information [here](https://github.com/cartr/MSMCcable)