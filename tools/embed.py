"""Generate the self-contained Windhawk source from verified release archives."""

import argparse
import base64
import ctypes as ct
from ctypes import wintypes as win
import hashlib
from pathlib import Path, PurePosixPath
import re
import textwrap
from zipfile import ZipFile

ROOT = Path(__file__).resolve().parent.parent
SCREENSHOT_URL = (
    "https://raw.githubusercontent.com/bardelyne/doom-hotkey/"
    "fcb4b5729067da4cd6110db02d3d3294b8234789/assets/doom-hotkey-freedoom-menu.png"
)
CRISPY_ARCHIVE = "crispy-doom-7.2.0-win64.zip"
FREEDOOM_ARCHIVE = "freedoom-0.13.0.zip"
CRISPY_HASH = "c04feca63c00fd4c381c53cce8444a2bb520020be38368ce80da60484c94f2ab"
FREEDOOM_HASH = "3f9b264f3e3ce503b4fb7f6bdcb1f419d93c7b546f4df3e874dd878db9688f59"
# Windhawk limits source lines to 4096 characters, including indentation and quotes.
ASSET_CHUNK_SIZE = 4088


def checked_archive(name, expected):
    archive = ROOT / "playable" / name
    with archive.open("rb") as stream:
        if hashlib.file_digest(stream, "sha256").hexdigest() != expected:
            raise ValueError(f"Release checksum mismatch: {name}")
    return ZipFile(archive)


def read_bundle():
    with checked_archive(CRISPY_ARCHIVE, CRISPY_HASH) as archive:
        files = {}
        for member in archive.infolist():
            name = PurePosixPath(member.filename).name
            if member.is_dir() or not (name == "crispy-doom.exe" or name.endswith(".dll")):
                continue
            if name in files or not all(c.isascii() and (c.isalnum() or c in ".-_+") for c in name):
                raise ValueError(f"Unexpected engine file: {name}")
            files[name] = archive.read(member), 0
        if "crispy-doom.exe" not in files:
            raise ValueError("Engine executable missing")
    with checked_archive(FREEDOOM_ARCHIVE, FREEDOOM_HASH) as archive:
        for campaign in (1, 2):
            name = f"freedoom{campaign}.wad"
            data = archive.read(f"freedoom-0.13.0/{name}")
            if not data.startswith(b"IWAD"):
                raise ValueError(f"Invalid game archive: {name}")
            files[name] = data, campaign
        notice = archive.read("freedoom-0.13.0/COPYING.txt").decode("utf-8")
        files["Freedoom-COPYING.txt"] = notice.encode("utf-8"), 0
    return files, notice.removeprefix("\ufeff")


def compressor():
    cabinet = ct.WinDLL("cabinet", use_last_error=True)
    cabinet.CreateCompressor.argtypes = [win.DWORD, ct.c_void_p, ct.POINTER(win.HANDLE)]
    cabinet.Compress.argtypes = [win.HANDLE, ct.c_void_p, ct.c_size_t, ct.c_void_p,
                                 ct.c_size_t, ct.POINTER(ct.c_size_t)]
    cabinet.CloseCompressor.argtypes = [win.HANDLE]
    return cabinet


def compress(cabinet, data):
    handle = win.HANDLE()
    if not cabinet.CreateCompressor(5, None, ct.byref(handle)):  # COMPRESS_ALGORITHM_LZMS
        raise ct.WinError(ct.get_last_error())
    try:
        size = ct.c_size_t()
        if cabinet.Compress(handle, data, len(data), None, 0, ct.byref(size)):
            raise RuntimeError("Expected a compressed buffer size")
        if ct.get_last_error() != 122 or not size.value:
            raise ct.WinError(ct.get_last_error())
        packed = ct.create_string_buffer(size.value)
        if not cabinet.Compress(handle, data, len(data), packed, len(packed), ct.byref(size)):
            raise ct.WinError(ct.get_last_error())
        return packed.raw[:size.value]
    finally:
        cabinet.CloseCompressor(handle)


def write_assets(output, files):
    cabinet = compressor()
    entries = []
    identity = hashlib.sha256()
    for index, (name, (data, campaign)) in enumerate(sorted(files.items())):
        digest = hashlib.sha256(data).digest()
        identity.update(name.encode("ascii") + b"\0" + digest)
        hex_bytes = ", ".join(f"0x{b:02x}" for b in digest)
        compressed = compress(cabinet, data)
        packed = base64.b64encode(compressed).decode("ascii")
        identifier = f"kAsset{index}"
        output.write(f"constexpr const char* {identifier}[] = {{\n")
        for start in range(0, len(packed), ASSET_CHUNK_SIZE):
            output.write(f'    "{packed[start:start + ASSET_CHUNK_SIZE]}",\n')
        output.write("};\n")
        entries.append(
            f'    {{L"{name}", {campaign}, {len(data)}, {{{hex_bytes}}}, '
            f'{identifier}, ARRAYSIZE({identifier}), {len(compressed)}}},\n'
        )
        print(f"Bundled {name}", flush=True)
    output.write(f'constexpr wchar_t kBundleDirectory[] = L"crispy-7.2-freedoom-0.13-{identity.hexdigest()[:12]}";\n')
    output.write("constexpr File kFiles[] = {\n" + "".join(entries) + "};\n")


def verify_assets(source, files):
    text = source.read_text(encoding="utf-8")
    arrays = re.findall(r'^constexpr const char\* (kAsset\d+)\[\] = \{\n(.*?)\n\};', text, re.M | re.S)
    table = re.search(r'^constexpr File kFiles\[\] = \{\n(.*?)\n\};', text, re.M | re.S)
    if len(arrays) != len(files) or len(dict(arrays)) != len(files) or table is None:
        raise ValueError("Unexpected embedded asset arrays or manifest")
    chunks = dict(arrays)
    entries = table[1].splitlines()
    if len(entries) != len(files):
        raise ValueError("Unexpected embedded manifest size")

    cabinet = ct.WinDLL("cabinet", use_last_error=True)
    cabinet.CreateDecompressor.argtypes = [win.DWORD, ct.c_void_p, ct.POINTER(win.HANDLE)]
    cabinet.Decompress.argtypes = [win.HANDLE, ct.c_void_p, ct.c_size_t, ct.c_void_p,
                                   ct.c_size_t, ct.POINTER(ct.c_size_t)]
    cabinet.CloseDecompressor.argtypes = [win.HANDLE]
    handle = win.HANDLE()
    if not cabinet.CreateDecompressor(5, None, ct.byref(handle)):
        raise ct.WinError(ct.get_last_error())
    try:
        for entry, (name, (data, campaign)) in zip(entries, sorted(files.items())):
            pattern = r'    \{L"([^"\n]+)", ([012]), (\d+), \{([^}]+)\}, (kAsset\d+), ARRAYSIZE\(\5\), (\d+)\},'
            match = re.fullmatch(pattern, entry)
            if match is None:
                raise ValueError(f"Unexpected manifest entry for {name}")
            filename, index, size, digest, array, packed_size = match.groups()
            checksum = ", ".join(f"0x{byte:02x}" for byte in hashlib.sha256(data).digest())
            if (filename, int(index), int(size), digest) != (name, campaign, len(data), checksum):
                raise ValueError(f"Manifest mismatch: {name}")
            lines = chunks.pop(array).splitlines()
            if not all(re.fullmatch(r'    "[A-Za-z0-9+/=]+",', line) for line in lines):
                raise ValueError(f"Invalid embedded chunks: {name}")
            packed = base64.b64decode("".join(line[5:-2] for line in lines), validate=True)
            output = ct.create_string_buffer(len(data))
            written = ct.c_size_t()
            if (len(packed) != int(packed_size) or
                    not cabinet.Decompress(handle, packed, len(packed), output, len(output), ct.byref(written)) or
                    written.value != len(data) or output.raw != data):
                raise ValueError(f"Embedded contents differ from the upstream archive: {name}")
    finally:
        cabinet.CloseDecompressor(handle)
    print(f"Verified all {len(files)} embedded files against the pinned upstream archives.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verify", type=Path, help="Verify the payloads in an existing generated source file.")
    args = parser.parse_args()
    files, notice = read_bundle()
    if args.verify is not None:
        verify_assets(args.verify, files)
        return
    launcher = (ROOT / "doom-hotkey.wh.cpp").read_text(encoding="utf-8")
    runtime = (ROOT / "tools/embedded-runtime.cpp").read_text(encoding="utf-8")
    launcher = launcher.replace(
        "Launch Doom from anywhere with a customizable keyboard shortcut.",
        "Play the bundled Doom engine and Freedoom campaigns with a customizable hotkey."
    )
    launcher = launcher.replace(
        "// @compilerOptions -luser32 -lshell32",
        "// @license         GPL-2.0-or-later\n"
        "// @compilerOptions -luser32 -lshell32 -lole32 -luuid -lbcrypt -lcabinet -lcrypt32"
    )
    launcher = launcher.replace("assets/doom-hotkey-freedoom-menu.png", SCREENSHOT_URL)
    start = launcher.index("\n1. Extract")
    end = launcher.index("\nChanges apply", start)
    launcher = launcher[:start] + textwrap.dedent("""
        The mod includes **Crispy Doom 7.2** and the complete **Freedoom Phase 1 and Phase 2** campaigns.
        Choose the campaign in Settings. The first launch unpacks the engine and selected campaign
        into `%LOCALAPPDATA%\\Windhawk Doom\\Runtime`. The game starts fullscreen every time.
        The bundled x64 engine runs on x64 Windows and uses x64 emulation on Windows 11 on Arm.

        Set **Game WAD** to your own DOOM.WAD or DOOM2.WAD to play the original game with the bundled engine.
        **Doom executable** is optional and lets you choose another source port that accepts -fullscreen.

        The bundled engine keeps settings and per-game saves in `%LOCALAPPDATA%\\Windhawk Doom\\Profile`.
        You can remove the Runtime folder after closing the game to free space. The mod recreates it on demand.
        The first launch after upgrading copies old settings and backs up old shared save slots in `Profile\\Legacy Saves`.
    """) + launcher[end:]
    launcher = launcher.replace(
        "- enginePath: \"\"",
        "- campaign: phase1\n"
        "  $name: Bundled campaign\n"
        "  $description: Used when Doom executable and Game WAD are both blank.\n"
        "  $options:\n"
        "    - phase1: Freedoom Phase 1\n"
        "    - phase2: Freedoom Phase 2\n"
        "- enginePath: \"\""
    ).replace(
        "Full local drive path to the engine executable.",
        "Optional full local drive path. Leave blank to use bundled Crispy Doom."
    ).replace(
        "Leave blank if the engine finds the game data itself.",
        "Leave blank to play the selected bundled campaign with the bundled engine."
    )
    credits_end = launcher.index("\n*/")
    launcher = launcher[:credits_end] + textwrap.dedent("""

        ## Source and licenses

        See the [project repository](https://github.com/bardelyne/doom-hotkey) for the generator and
        [third-party sources and licenses](https://github.com/bardelyne/doom-hotkey/blob/main/THIRD-PARTY.md).
    """) + launcher[credits_end:]
    bundled_selection = textwrap.indent(textwrap.dedent("""\
        std::wstring profileArguments;
        if (Trim(engineSetting).empty()) {
            const int campaign = GetSetting(L"campaign") == L"phase2" ? 2 : 1;
            if (!DoomBundle::Prepare(engine, wad, campaign, profileArguments)) {
                Wh_Log(L"The bundled Doom engine or game data could not be prepared.");
                return;
            }
        }
    """), "    ")
    launcher = launcher.replace("    // BUNDLED DOOM PREPARATION\n", bundled_selection)
    launcher = launcher.replace(
        "    // BUNDLED DOOM PROFILE ARGUMENTS", "    command += profileArguments;"
    )
    before, after = launcher.split("// EMBEDDED DOOM IMPLEMENTATION")
    runtime_before, runtime_after = runtime.split("// EMBEDDED DOOM ASSETS")
    build = ROOT / "build"
    build.mkdir(exist_ok=True)
    result = build / "doom-hotkey.wh.cpp"
    temporary = build / "doom-hotkey.wh.cpp.partial"
    with temporary.open("w", encoding="utf-8", newline="\n") as output:
        output.write(before)
        output.write("// Generated by tools/embed.py from pinned upstream releases.\n")
        output.write("// Crispy Doom source: https://github.com/fabiangreffrath/crispy-doom/tree/crispy-doom-7.2\n")
        output.write("/* Freedoom license\n" + notice + "*/\n")
        output.write(runtime_before)
        write_assets(output, files)
        output.write(runtime_after)
        output.write(after)
    temporary.replace(result)
    print(f"{result} ({result.stat().st_size:,} bytes)")


if __name__ == "__main__":
    main()
