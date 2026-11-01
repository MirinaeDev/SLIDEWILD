# Distributing a downloadable build

Source files belong in Git. Executables, screenshots, trailers and ZIP files
belong in GitHub Release attachments, outside the source history.

## Windows x64

1. From a clean checkout, run `./setup.ps1` then `./build.ps1` in PowerShell.
2. Use `Play.cmd` to check all eight courses, audio, controllers and fullscreen.
3. With Python 3.10+, run `python tools/check_repository.py` and `python tools/package_windows.py`.
4. Extract the ZIP into another folder and verify its SHA256 manifest and launch.
5. Attach `dist/Slidewild-0.35-Windows-x64.zip` and its checksum to a Release.

Packaging refuses to overwrite an existing distribution. Move a previous build
outside `dist/` before rebuilding the same version; do not automatically delete
versions already shared with players.

## Mac Apple Silicon

Follow [MACOS-BUILD.txt](MACOS-BUILD.txt), then run `bash test-macos.sh` on a
real Mac. The Metal port is prepared; native GPU, audio and controller validation
is still required. CPU CI tests and shader fingerprints are not sufficient.

After validation, archive the application:

```sh
ditto -c -k --sequesterRsrc --keepParent build-macos/Slidewild.app Slidewild-Mac-AppleSilicon.zip
```

The build script applies an ad hoc local signature. Smooth public distribution
requires Developer ID signing and notarization with the owner's credentials,
which must remain outside the repository.

## Before publishing

- Verify artwork rights and decide whether to grant a reuse license.
- Run the GitHub workflows and inspect their results; adding workflows does not mean they have run.
- Test archives on other machines. Do not announce a validated Mac binary before native testing.
- Keep `.local-archive/`, `.tools/`, `build*/`, `dist/` and `media/` out of Git.
