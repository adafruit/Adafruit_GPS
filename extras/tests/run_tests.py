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
    # Host builds otherwise enable extensions automatically. Cover the basic
    # GPS API and marine extensions, each with consistent float/double types
    # across the sketch and every library translation unit.
    cases = []
    for test in tests:
        if test.suffix == ".ino":
            for extras in (0, 1):
                for float_type in ("float", "double"):
                    cases.append((test, extras, float_type))
        else:
            cases.append((test, None, None))
    failures = []
    for index, (test, extras, float_type) in enumerate(cases, 1):
        name = str(test.relative_to(tests_dir))
        if extras is not None:
            name += f" (NMEA_EXTRAS={extras}, NMEA_FLOAT_T={float_type})"
        print(f"\n[{index}/{len(cases)}] {name}", flush=True)
        binary = build / str(index)
        if test.suffix == ".ino":
            sources = [*sorted((root / "src").glob("*.cpp")),
                       support / "Arduino.cpp", support / "sketch_main.cpp"]
            extra_flags = ["-I" + str(support), f"-DNMEA_EXTRAS={extras}",
                           f"-DNMEA_FLOAT_T={float_type}"]
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
