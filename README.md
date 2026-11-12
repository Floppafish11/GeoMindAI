# GeoMind AI

A Geode mod project for Geometry Dash 2.2081, classic mode only.

## Current status

This is the initial project scaffold. It adds a GeoMind AI entry point to the main menu. Automated gameplay, route discovery, and XDBot macro export are not implemented yet.

## Planned system

- Classic game modes: cube, ship, ball, UFO, wave, robot, spider and swing.
- Classify level objects, portals, orbs and pads.
- Run repeatable route trials and score candidate paths by completion and consistency.
- Store route data separately from playback.
- Validate exported macros against the target macro format before enabling export.

## Compatibility

- Geometry Dash: 2.2081
- Geode SDK: 5.x

## Build

A GitHub Actions workflow will build the mod for Windows and upload the compiled Geode package as an artifact.

## XDBot compatibility

XDBot's public README lists macro import support and a Path Finder feature, but a verified specification for its native macro serialization is not available here. Export is not advertised as working until that format can be tested against a real XDBot macro.
