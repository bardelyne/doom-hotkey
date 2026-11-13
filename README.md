# Can It Run Doom?

A Windhawk mod that starts Doom fullscreen with **Ctrl+Alt+Shift+D**. Press the shortcut again to return to the running game.

![Freedoom Phase 1 title screen and main menu](assets/doom-hotkey-freedoom-menu.png)

## Play Freedoom

Import [`doom-hotkey.wh.cpp`](doom-hotkey.wh.cpp) as a local mod in Windhawk. Enable **Download official game files**, choose a Freedoom campaign, and press the shortcut. The first launch downloads about 34 MB and checks SHA-256 hashes for both archives and the extracted files. Later launches reuse the verified files and work offline.

The downloaded Crispy Doom engine needs x64 Windows or x64 emulation on Windows 11 on Arm. Setup uses the Windows `tar.exe` utility. The mod downloads the files only when you press the shortcut with downloads enabled and a verified copy is missing.

## Use an existing Doom installation

Set **Doom executable** to the full local path to an engine that supports `-fullscreen`. Set **Game WAD** to `DOOM.WAD`, `DOOM2.WAD`, or a compatible IWAD. Leave **Game WAD** blank if your engine finds its game data automatically. You can also set just **Game WAD** to play your own game with downloaded Crispy Doom.

The shortcut accepts Ctrl, Alt, Shift and Win modifiers followed by a letter, number or F1–F24. Windows reserves F12. If Windows cannot register a new combination, the previous one stays active. Windhawk's debug log records invalid paths, shortcut conflicts, and setup errors.

## Storage and uninstall

Windhawk leaves `%LOCALAPPDATA%\Windhawk Doom` on disk when you uninstall the mod. The `Runtime` subfolder holds the game cache. Close the game before deleting it to reclaim space. The `Profile` subfolder holds settings and saves for the downloaded engine. Each custom WAD keeps the same save folder when you move or rename the file. Earlier test builds may also have files directly under `Runtime` or in the old versioned folder. Keep any saves you want before deleting those folders.

If you used a custom WAD with the v0.3.2 test build, you can copy its existing slots from the old `custom-<filename>-…` folder into the new `custom-<checksum>` folder after launching that WAD once and closing the game.

## Official downloads

- [Crispy Doom 7.2.0 for 64-bit Windows](https://github.com/fabiangreffrath/crispy-doom/releases/download/crispy-doom-7.2/crispy-doom-7.2.0-win64.zip)
- [Freedoom 0.13.0](https://github.com/freedoom/freedoom/releases/download/v0.13.0/freedoom-0.13.0.zip)

To try the game directly, extract Crispy Doom into `playable/engine` and the Freedoom archive into `playable/game`. Run `Play Freedoom.cmd` to open Phase 1 fullscreen.

## Credits

- **id Software** created DOOM and released its engine source. [DOOM source release](https://github.com/id-Software/DOOM)
- **Simon Howard and the Chocolate Doom contributors** developed the source port that Crispy Doom builds on. [Chocolate Doom](https://github.com/chocolate-doom/chocolate-doom)
- **Fabian Greffrath and the Crispy Doom contributors** maintain the engine used by this mod. [Crispy Doom](https://github.com/fabiangreffrath/crispy-doom)
- **The Freedoom contributors** created the campaigns, graphics, sounds, and music. [Freedoom](https://github.com/freedoom/freedoom)

The mod source uses [GPL-2.0-or-later](LICENSE.md). See [third-party sources and licenses](THIRD-PARTY.md) for the downloaded engine, Windows libraries, and Freedoom.
