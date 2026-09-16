#!/usr/bin/env python3
"""Build and run every regression under extras/tests with Linux g++."""

import os
from pathlib import Path
import subprocess
import tempfile


def main():
    tests_dir = Path(__file__).resolve().parent
    root = tests_dir.parent.parent
    support = root / "extras/test_support"
    tests = sorted([*tests_dir.glob("*/*.cpp"), *tests_dir.glob("*/*.ino")])
    if not tests:
        raise RuntimeError("No regression tests found")

    # Keep binaries outside the checkout, including when run locally.
    build = Path(tempfile.mkdtemp(prefix="gps-tests-"))
    compiler = os.environ.get("CXX", "g++")
    flags = [
        "-std=c++11", "-Wall", "-Wextra", "-Werror", "-g", "-pthread",
        "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
        "-fno-omit-frame-pointer", "-I" + str(root / "src"),
    ]
    # Apply the same type to the sketch and every library translation unit.
    # The default float API and optional double API must both remain usable.
    cases = []
    for test in tests:
        modes = ("float", "double") if test.suffix == ".ino" else (None,)
        cases.extend((test, mode) for mode in modes)
    failures = []
    for index, (test, mode) in enumerate(cases, 1):
        name = str(test.relative_to(tests_dir))
        if mode is not None:
            name += f" (NMEA_FLOAT_T={mode})"
        print(f"\n[{index}/{len(cases)}] {name}", flush=True)
        binary = build / str(index)
        if test.suffix == ".ino":
            # Enable marine extensions so RMB and conditional marine checks run.
            sources = [*sorted((root / "src").glob("*.cpp")),
                       support / "Arduino.cpp", support / "sketch_main.cpp"]
            extra_flags = ["-I" + str(support), "-DNMEA_EXTRAS=1",
                           f"-DNMEA_FLOAT_T={mode}"]
        else:
            sources = [root / "src/Adafruit_NMEA.cpp",
                       root / "src/Adafruit_GNSS.cpp"]
            extra_flags = []
        try:
            subprocess.run([compiler, *flags, *extra_flags,
                            *map(str, sources), "-x", "c++", str(test),
                            "-o", str(binary)], check=True, timeout=120)
            subprocess.run([str(binary)], check=True, timeout=30)
        except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as error:
            print(f"FAIL: {name}: {error}", flush=True)
            failures.append(name)

    print(f"\n{len(cases) - len(failures)}/{len(cases)} test configurations "
          f"passed ({len(tests)} test sources).", flush=True)
    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
