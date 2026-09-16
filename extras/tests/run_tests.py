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
    failures = []
    for index, test in enumerate(tests, 1):
        name = str(test.relative_to(tests_dir))
        print(f"\n[{index}/{len(tests)}] {name}", flush=True)
        binary = build / str(index)
        if test.suffix == ".ino":
            # Enable marine extensions so RMB and conditional marine checks run.
            sources = [*sorted((root / "src").glob("*.cpp")),
                       support / "Arduino.cpp", support / "sketch_main.cpp"]
            extra_flags = ["-I" + str(support), "-DNMEA_EXTRAS=1"]
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

    print(f"\n{len(tests) - len(failures)}/{len(tests)} regressions passed.", flush=True)
    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
