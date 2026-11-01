# Local validation, November 1, 2026

Source publication and distribution preparation on Windows x64.

- Release CMake build using pinned w64devkit, SDL3 3.4.18 and DXC.
- Camera/projection, physics, AI racing, scenery, course geometry, platform
  paths, animation/music and Metal fingerprint checks.
- All course scenery was checked by `course_scene`. After the steering pose
  adjustments, the other suites were rerun; scenery geometry was unchanged.
- Native Direct3D 12 check: both music tracks loaded; title screen and Damien
  Miri credit inspected. Stereo looping, crossfade, pause and mute are covered
  by the presentation test.
- Publication audit: 82 runtime artwork files, PNG/RGBA dimensions, PCM music,
  file sizes, local paths, common secret signatures and third-party notices.

GitHub workflows are prepared but have not run on GitHub yet. Native Mac
compilation, graphics, sound and controller validation remain to be done on
Apple Silicon. Shader audits do not replace those checks.

Local archives, tools, builds, videos and distribution files are ignored by
Git. References and previous versions are archived locally, not deleted.
Detailed local logs are in `build/qa/` and `build/windows/Testing/Temporary/`.
They are excluded from the public repository.
