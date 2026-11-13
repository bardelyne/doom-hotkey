# Third-party sources and licenses

When enabled, the mod downloads the unmodified archive from the [Crispy Doom 7.2.0 Windows release](https://github.com/fabiangreffrath/crispy-doom/releases/tag/crispy-doom-7.2). See the [exact engine source and build scripts](https://github.com/fabiangreffrath/crispy-doom/tree/fdeabe4c23a833534b70aa84a22f1685aae7125d) and the [Windows build log](https://github.com/fabiangreffrath/crispy-doom/actions/runs/38230603065/job/114748843276). Crispy Doom is distributed under the [GNU GPL](LICENSE.md).

The two WAD files are from [Freedoom 0.13.0](https://github.com/freedoom/freedoom/tree/v0.13.0). See [Freedoom’s license and copyright notice](assets/Freedoom-COPYING.txt).

## Windows libraries

These are the matching MSYS2 UCRT64 package sources. The archives include the upstream code and MSYS2’s packaging files and patches. The [runtime license notices](assets/runtime-licenses) come from the matching packages or their upstream sources.

| Downloaded libraries | Source |
| --- | --- |
| `SDL2.dll` | [SDL2 2.32.10-1](https://repo.msys2.org/mingw/sources/mingw-w64-SDL2-2.32.10-1.src.tar.zst) |
| `SDL2_mixer.dll` | [SDL2_mixer 2.8.2-1](https://repo.msys2.org/mingw/sources/mingw-w64-SDL2_mixer-2.8.2-1.src.tar.zst) |
| `SDL2_net.dll` | [SDL2_net 2.4.0-1](https://repo.msys2.org/mingw/sources/mingw-w64-SDL2_net-2.4.0-1.src.tar.zst) |
| `SDL3.dll` | [SDL3 3.4.18-1](https://repo.msys2.org/mingw/sources/mingw-w64-sdl3-3.4.18-1.src.tar.zst) |
| `libFLAC.dll` | [FLAC 1.5.0-2](https://repo.msys2.org/mingw/sources/mingw-w64-flac-1.5.0-2.src.tar.zst) |
| `libfluidsynth-3.dll` | [FluidSynth 2.6.1-1](https://repo.msys2.org/mingw/sources/mingw-w64-fluidsynth-2.6.1-1.src.tar.zst) |
| `libgcc_s_seh-1.dll`, `libgomp-1.dll`, `libstdc++-6.dll` | [GCC 16.2.0-5](https://repo.msys2.org/mingw/sources/mingw-w64-gcc-16.2.0-5.src.tar.zst) |
| `libiconv-2.dll` | [libiconv 1.19-1](https://repo.msys2.org/mingw/sources/mingw-w64-libiconv-1.19-1.src.tar.zst) |
| `libmp3lame-0.dll` | [LAME 3.100-3](https://repo.msys2.org/mingw/sources/mingw-w64-lame-3.100-3.src.tar.zst) |
| `libmpg123-0.dll` | [mpg123 1.33.7-1](https://repo.msys2.org/mingw/sources/mingw-w64-mpg123-1.33.7-1.src.tar.zst) |
| `libogg-0.dll` | [libogg 1.3.6-1](https://repo.msys2.org/mingw/sources/mingw-w64-libogg-1.3.6-1.src.tar.zst) |
| `libopus-0.dll` | [Opus 1.6.1-1](https://repo.msys2.org/mingw/sources/mingw-w64-opus-1.6.1-1.src.tar.zst) |
| `libopusfile-0.dll` | [opusfile 0.12-4](https://repo.msys2.org/mingw/sources/mingw-w64-opusfile-0.12-4.src.tar.zst) |
| `libpng16-16.dll` | [libpng 1.6.59-1](https://repo.msys2.org/mingw/sources/mingw-w64-libpng-1.6.59-1.src.tar.zst) |
| `libportaudio.dll` | [PortAudio 19.7.0-7](https://repo.msys2.org/mingw/sources/mingw-w64-portaudio-1~19.7.0-7.src.tar.zst) |
| `libreadline8.dll` | [Readline 8.3.003-1](https://repo.msys2.org/mingw/sources/mingw-w64-readline-8.3.003-1.src.tar.zst) |
| `libsamplerate-0.dll` | [libsamplerate 0.2.2-1](https://repo.msys2.org/mingw/sources/mingw-w64-libsamplerate-0.2.2-1.src.tar.zst) |
| `libsndfile-1.dll` | [libsndfile 1.2.2-1](https://repo.msys2.org/mingw/sources/mingw-w64-libsndfile-1.2.2-1.src.tar.zst) |
| `libtermcap-0.dll` | [termcap 1.3.1-8](https://repo.msys2.org/mingw/sources/mingw-w64-termcap-1.3.1-8.src.tar.zst) |
| `libvorbis-0.dll`, `libvorbisenc-2.dll`, `libvorbisfile-3.dll` | [libvorbis 1.3.7-3](https://repo.msys2.org/mingw/sources/mingw-w64-libvorbis-1.3.7-3.src.tar.zst) |
| `libwavpack-1.dll` | [WavPack 5.9.0-1](https://repo.msys2.org/mingw/sources/mingw-w64-wavpack-5.9.0-1.src.tar.zst) |
| `libwinpthread-1.dll` | [MinGW-w64 revision 4564ee4b5](https://github.com/mingw-w64/mingw-w64/tree/4564ee4b5063097bf747af3a3f8270a28adff820/mingw-w64-libraries/winpthreads) and [MSYS2 recipe and patch](https://github.com/msys2/MINGW-packages/tree/6c5d16ee3b3e0dd9a5387878709ad55ea0887d6d/mingw-w64-winpthreads) |
| `libxmp.dll` | [libxmp 4.7.4-1](https://repo.msys2.org/mingw/sources/mingw-w64-libxmp-4.7.4-1.src.tar.zst) |
| `zlib1.dll` | [zlib 1.3.2-2](https://repo.msys2.org/mingw/sources/mingw-w64-zlib-1.3.2-2.src.tar.zst) |
