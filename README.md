# sys-botbase-C++
A Nintendo Switch (CFW) sys-module that allows users to remote control their Switch via local WiFi or USB, as well as read and write to a game's memory. This can be used to create bots for games and other fun automation projects.

This is a C++ rewrite of the original sys-botbase and usb-botbase with support for ACNH removed, tick-precise [Pokémon Automation's controller commands added](https://github.com/PokemonAutomation/ComputerControl), and reduced heap use.

It is designed to be backwards compatible with existing SysBot.NET implementations, though backwards compatibility can be disabled by sending `configure enableBackwardsCompat 0` or by setting `g_enableBackwardsCompat` in source and building the project.

With backwards compatibility disabled, WiFi and USB connections client-side can be simplified: read and send operations can be looped while there's data available, both connections send raw data (no decoding or endian conversion needed), both expect `\r\n` as a line terminator.

New structure should make the sys-module easier to maintain and extend, as well as make it easier to add new features.

## Features:
### Tick-precise Controller Commands:
- Send fast, asynchronous, tick-precise controller commands

Commands:
- `cqControllerState {hex-encoded controller command struct}`: Enqueues the specified controller state into the schedule queue. The command struct is a hex-encoded `ControllerCommand` struct. See [`include/controllerCommands.h`](include/controllerCommands.h#L59) for details.
- `cqCancel`: Cancel all pending controller commands and set the controller state back to neutral.
- `cqReplaceOnNext`: Declare that the next command should atomically replace the entire command schedule.\
This differs from `cqCancel + cqControllerState` in that the transition from the current schedule to the new command happens without returning the controller to the neutral state. Meaning if button `A` is being held down by the existing command schedule and is replaced with a new command that also holds `A`, the button `A` will be held throughout and never released.\
\
Example usecase: SV sandwich making. If the current schedule is holding `A` to hold an ingredient and it needs to change directions, a `cqReplaceOnNext + new command` can be used to replace the path with the new path without ever releasing `A` as doing so will drop the ingredient.

### Remote Control:
- Set controller state
- Simulate button press, hold, and release
- Simulate touch screen drawing
- Simulate keyboard input

### Memory Reading and Writing:
- Read/write x amount bytes of consecutive memory from RAM based on:
    1. Absolute memory address
    2. Address relative to main nso base
    3. Address relative to heap base

### Screen Capture:
- Capture current screen and return as JPG

### Logging:
- Added text file logging to `atmosphere/contents/43000000000B/log.txt` for debugging purposes.
- It will always log on error, exception, or during/after generally important operations. More verbose logging can be enabled by sending `configure enableLogs 1`.

## Disclaimer:
This project was created for the purpose of development for bot automation. The creators and maintainers of this project are not liable for any damages caused or bans received. Use at your own risk.

## Installation
1. Download [latest release](https://github.com/PokemonAutomation/sys-botbase-cpp/releases/latest) and extract into the root of your Nintendo Switch SD card.
2. Open the `config.cfg` located in `atmosphere/contents/43000000000B` using your favorite text editor.
3. Change text to `usb` if you want to connect using USB, or `wifi` if you want to connect using a local TCP connection. Defaults to `wifi`.
4. Restart your Switch. If the right Joy-Con glows blue like shown, sys-botbase is installed correctly.
   ![](joycon-glow.gif)
5. Follow [SysBot's usb-botbase setup guide](https://github.com/kwsch/SysBot.NET/wiki/Configuring-a-new-USB-Connection) if you want to use USB.

## Building
1. Clone this repository.
2. Get [devkitPro](https://devkitpro.org/wiki/Getting_Started).
3. Run `MSys2`, use `pacman -S switch-dev libnx switch-libjpeg-turbo devkitARM`. To easily update installed packages in the future, use `pacman -Syu`.
4. Open the `.sln` with Visual Studio 2022 and build the solution. Alternatively, can run `MSys2`, `cd` to the cloned repository, and `make` (can optionally append ` -j$(nproc)`) to build the project.

## Credits
- Big thank you to [jakibaki](https://github.com/jakibaki/sys-netcheat) for a great sys-module base to learn and work with, as well as being helpful on the ReSwitched Discord!
- Thanks to RTNX on Discord for bringing to my attention a nasty little bug that would very randomly cause RAM poking to go bad and the switch (sometimes) crashing as a result.
- Thanks to Anubis for stress testing!
- Thanks to FishGuy for the initial USB-Botbase implementation.
- Thanks to Mysticial for the tick-precise controller input template, testing, and debugging.
