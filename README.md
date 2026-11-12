# GeoMind AI

Geode mod for Geometry Dash **2.2081**, **classic mode only**.

## Current build: route capture and replay foundation

This version adds:
- A GeoMind AI button on the main menu.
- Pause-menu controls to record a route, save it, replay it, and stop replay.
- Input capture through the game's PlayerObject button interface.
- Level-ID validation so a saved route cannot be replayed on another level.
- Route data stored as JSON in Geode's mod save directory (`last-route.gma`).

A replay sends the captured press/release events back through the game's PlayerObject interface. This is a playback foundation, not yet an autonomous solver.

## Not implemented yet

- Autonomous route discovery from a blank level.
- High-speed multi-trial search or a genetic/heuristic optimizer.
- Full semantic classification of every portal, orb, pad, trigger, and mode transition.
- Verified XDBot macro export. XDBot's public repository does not include its serialization source, so no unsupported file format is being guessed.

## Build target

- Geometry Dash 2.2081
- Geode SDK 5.11.2
- Windows x64

The GitHub Actions workflow builds and uploads a `.geode` artifact on pushes to `main`.

## Use

1. Install the compiled `.geode` into your Geode mods folder.
2. Open a level and pause.
3. Select **Record Route**, resume and play.
4. Pause and select **Stop + Save**.
5. Replay the saved route from the same level using **Replay Route**.

The route file is stored in the mod's Geode save directory. This first implementation is intended for testing the capture/replay pipeline; it does not claim to find a level solution by itself.
