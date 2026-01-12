#!/usr/bin/env python3
import argparse
import subprocess
import sys
import shutil
import os

parser: argparse.ArgumentParser = argparse.ArgumentParser()
parser.add_argument("--get-deps", action="store_true", help="Shows the \
        dependencies needed to build")
parser.add_argument("--build-rnative", action="store_true", help="Builds the \
        dependencies for RNative")
parser.add_argument("--print-loc", action="store_true", help="Counts lines of \
        code")
parser.add_argument("--run-tests", action="store_true", help="Runs all tests")


def get_deps() -> None:
    deps: dict = {
        "GLFW": "  >= 3.4",
        "SDL": "   == 2.0",
        "OpenGL": ">= 2.0",
        "GLEW": "  >= 1.5",
        "OpenAL": "(OpenAL-soft) any",
        "miniz": " >= 3.0",
    }

    for key, value in deps.items():
        print(key + ": " + value)
    print()
    print("These are ONLY the dependencies which are needed to be installed separately")


def build_rnative() -> int:
    print("checking operating system... ", end="")
    if sys.platform.startswith("linux"):
        print("linux")
        print("generate source and header for: xdg-toplevel-icon-v1")
        gen_header = ["wayland-scanner", "client-header",
                      "/usr/share/wayland-protocols/staging/xdg-toplevel-icon/xdg-toplevel-icon-v1.xml",
                      "src/include/rnative/xdg-toplevel-icon-v1-client-protocol.h"]  # How am i supposed to split a path???
        gen_source = ["wayland-scanner", "private-code", "/usr/share/wayland-protocols/staging/xdg-toplevel-icon/xdg-toplevel-icon-v1.xml", "src/rnative/xdg-toplevel-icon-v1-client-protocol.c"]

        print("command: " + " ".join(gen_header))
        subprocess.run(gen_header)

        print("command: " + " ".join(gen_source))
        subprocess.run(gen_source)

        print("complete.")
    elif sys.platform == "darwin":
        print("macOS")
        print("no work to do.")
    elif sys.platform == "win32":
        print("windows")
        print("no work to do.")
    else:
        print("unknown")
        print("unknown operating system.")
        return 1

    return 0


def print_loc():
    command = "cloc"
    if not shutil.which(command):
        print(f"{command} was not found in PATH.")
        return 1
    args = ["--include-ext=cpp,hpp", "--exclude-dir=rnative", "src/", "include/"]

    print("command: " + command + " " + " ".join(args))
    subprocess.run([command] + args)
    return 0


def is_executable(path):
    return os.path.isfile(path) and os.access(path, os.X_OK)


def run_tests():
    TEST_DIR = "./bin/tests"
    if not os.path.isdir(TEST_DIR):
        print(f"Test dir '{TEST_DIR}' not found")
        sys.exit(1)

    failed = False

    count = 0.0
    for name in sorted(os.listdir(TEST_DIR)):
        count += 1.0

    done = 0.0
    for name in sorted(os.listdir(TEST_DIR)):
        path = os.path.join(TEST_DIR, name)

        if not is_executable(path):
            done += 1
            continue

        print("\r", end="", flush=True)
        print("[" + str(int((done / count) * 100)) + "%]", end="", flush=True)
        if name == "sound_engine_test" \
                or name == "icon_test" \
                or name == "plugin_test" \
                or name == "multithreaded_test":
            # cd bin && tests/sound_engine_test -- --unit-test
            result = subprocess.run(
                ["tests/sound_engine_test", "--", "--unit-test"],
                cwd="bin"
            )
        else:
            result = subprocess.run(
                [path, "--", "--unit-test"],
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
            )

        if result.returncode != 0:
            print()
            print("FAIL: " + name)
            failed = True
        done += 1

    print("\r", end="", flush=True)
    print("[" + "100" + "%]", flush=True)
    sys.exit(1 if failed else 0)

def main() -> int:
    args = parser.parse_args()

    if args.get_deps:
        get_deps()
    elif args.build_rnative:
        return build_rnative()
    elif args.print_loc:
        return print_loc()
    elif args.run_tests:
        return run_tests()
    else:
        parser.print_help()
        return 1

    return 0


if __name__ == "__main__":
    exit(main())
