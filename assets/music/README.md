# Music

Tracks supplied by Damien Miri: `Menu.mp3` for menus and `Course.m4a` for racing.
Original files are retained in the ignored local archive.

The game uses `menu.wav` and `course.wav` (48 kHz, stereo, 16-bit PCM), decoded
by SDL without additional codecs. `tools/prepare_music.py` removes digital
silence at the ends, blends the loop seam over 120 ms, and normalizes peaks
to 0.90. Independent tracks crossfade between menus and racing; race music
is attenuated while paused.

Optional conversion requires PyAV and NumPy. Playing and compiling the game
do not require those dependencies. Supplying the tracks for this game does
not grant permission to reuse them in other projects.
