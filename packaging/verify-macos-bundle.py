"""Fail packaging if the extracted Mac bundle cannot meet its advertised contract."""
import os
from pathlib import Path
import plistlib
import re
import subprocess
import sys


def run(*args):
    return subprocess.check_output(args, text=True)


def verify(bundle, version):
    with (bundle / "Contents/Info.plist").open("rb") as stream:
        info = plistlib.load(stream)
    assert info["CFBundleIdentifier"] == "com.orbitalaudio.orbitalpan", info
    assert info["CFBundleShortVersionString"] == version, info
    assert info["CFBundleExecutable"] == "OrbitalPan", info
    binary = bundle / "Contents/MacOS/OrbitalPan"
    assert os.access(binary, os.X_OK), "Bundle executable lost execute permissions"
    run("lipo", str(binary), "-verify_arch", "arm64", "x86_64")
    run("codesign", "--verify", "--strict", "--verbose=2", str(bundle))
    for arch, expected in (("x86_64", (10, 13)), ("arm64", (11, 0))):
        commands = run("otool", "-arch", arch, "-l", str(binary))
        minima = re.findall(
            r"cmd LC_(?:BUILD_VERSION|VERSION_MIN_MACOSX)\b(?:(?!\nLoad command).)*?"
            r"\n\s+(?:minos|version) (\d+)\.(\d+)", commands, re.S
        )
        assert len(minima) == 1, f"Cannot determine {arch} deployment target"
        minimum = tuple(map(int, minima[0]))
        assert minimum == expected, f"Unexpected {arch} minimum: {minimum}, expected {expected}"
        dependencies = run("otool", "-arch", arch, "-L", str(binary))
        for line in dependencies.splitlines()[1:]:
            dependency = line.strip().split(" (", 1)[0]
            assert dependency.startswith(("/System/Library/", "/usr/lib/")), (
                f"Non-system dependency in {arch}: {dependency}"
            )
        print(f"PASS {arch}: deployment target {minimum}, system dependencies only")
    print(f"PASS signed universal macOS bundle, identity, version and permissions: {bundle.name}")


if __name__ == "__main__":
    verify(Path(sys.argv[1]), sys.argv[2])
