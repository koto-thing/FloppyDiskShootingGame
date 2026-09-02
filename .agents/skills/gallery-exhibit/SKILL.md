---
name: gallery-exhibit
description: Add or update a 3D model, animation preset, description, and unlock condition in this game's GalleryScene. Use when the user asks to exhibit another gameplay model or change an existing gallery exhibit.
---

# Gallery Exhibit

Add the requested exhibit without creating a second model implementation

## Source of truth

- Register the stable save identifier in `Domain/ValueObjects/GalleryEntry.h`; append before `Count` and never reorder existing entries because their ordinal is persisted as a bit
- Add the presentation metadata, animation names, framing scale, and renderer dispatch to `Presentation/Scenes/GalleryScene.cpp`
- Reuse the gameplay model view from `Presentation/Gameplay/Models` or `Presentation/Gameplay/Stages`; if the model is still embedded in gameplay rendering, extract the smallest shared header-only view and call it from both places
- Add the unlock at the real gameplay event: normal models unlock when spawned, bosses when their defeat state begins, and attached units with their owning boss unless the user specifies another rule
- Persist through `SettingsRepository::UnlockGalleryEntry`; do not overwrite volume, retro effect, or previously unlocked bits

## Animation presets

Expose only states or motions that exist in gameplay. Pass gallery time into the same model state or transform parameters used by the game. Static models keep a single `IDLE` preset. Pause must freeze the preset, switching a model resets its animation and camera, and locked entries remain a dark silhouette with hidden metadata

Keep keyboard and mouse controls consistent with the existing scene: arrow keys select model and animation, Space toggles playback, A/D or left drag orbits, W/S or the wheel zooms, R resets, and the bottom-right button returns to the title

## Verification

- Extend the settings round-trip test when persistence behavior changes
- Build Debug x64 for `FloppyDiskShootingGame.vcxproj`
- Build and run `FloppyEngine.Tests.vcxproj`
- Build Release x64 and confirm the executable plus required resources still fit within 1.44MB
- Run `git diff --check` and inspect every new exhibit in locked and unlocked states when visual execution is available
