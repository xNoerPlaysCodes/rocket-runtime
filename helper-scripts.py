#!/usr/bin/env python3
import argparse
import subprocess
import sys
import shutil
import os

# -d -r -p -t -i -b 

parser: argparse.ArgumentParser = argparse.ArgumentParser()
parser.add_argument("-d", "--get-deps", action="store_true", help="Shows the \
        dependencies needed to build")
parser.add_argument("-r", "--build-rnative", action="store_true", help="Builds the \
        dependencies for RNative")
parser.add_argument("-p", "--print-loc", action="store_true", help="Counts lines of \
        code")
parser.add_argument("-t", "--run-tests", action="store_true", help="Runs all tests")
parser.add_argument("-i", "--init-cmake", action="store_true", help="Initializes CMake \
        with necessary flags")
parser.add_argument("-b", "--build", action="store_true", help="Build, all at once, \
        all in order")
parser.add_argument("-g", "--glfnldr-backend", action="store", help="Which glfnldr-backend to \
        use when building", default="GLEW", choices=["GLEW", "LIBEPOXY"])


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
    passed = 0.0
    for name in sorted(os.listdir(TEST_DIR)):
        if name.endswith(".exe"):
            name = name[:-4]
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
                ["tests/" + name, "--", "--unit-test"],
                cwd="bin",
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
            )
        else:
            result = subprocess.run(
                [path, "--", "--unit-test"],
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
            )

        if result.returncode != 0:
            print(" FAIL: " + name)
            failed = True
        else:
            passed += 1
        done += 1

    print("\r", end="", flush=True)
    print("[" + "100" + "%]", flush=True)
    print(str(int(passed)) + "/" + str(int(done)) + " tests passed")
    sys.exit(1 if failed else 0)


def init_cmake(args):
    commands = ["cmake"]
    for command in commands:
        if not shutil.which(command):
            print(f"{command} was not found in PATH.")
            return 1

    os.makedirs("build", exist_ok=True)

    backend = args.glfnldr_backend
    args = ["-DCMAKE_EXPORT_COMPILE_COMMANDS=ON", "-DGLFNLDR_BACKEND=" + backend, "-B", "build"]

    print("command: " + "cmake" + " " + " ".join(args))
    subprocess.run(["cmake"] + args)
    return 0


def build(args):
    if not os.path.isdir("build"):
        init_cmake(args)
        build_rnative()

    cpus = os.cpu_count()

    if os.path.exists("build/build.ninja") and (cpus + 2 != cpus * 2):
        cpus += 2 # Ninja scales better with 2 extra jobs

    command = "cmake"
    args = ["--build", "build", "--", "-j" + str(cpus)]

    print("> command: " + "cmake" + " " + " ".join(args))
    prc = subprocess.run(["cmake"] + args)

    cmake_code = prc.returncode

    return cmake_code

def main() -> int:
    args = parser.parse_args()

    if args.get_deps:
        get_deps()
    elif args.print_loc:
        return print_loc()
    elif args.build_rnative:
        return build_rnative()
    elif args.init_cmake:
        return init_cmake(args)
    elif args.build:
        return build(args)
    elif args.run_tests:
        return run_tests()
    else:
        parser.print_help()
        return 1

    return 0


if __name__ == "__main__":
    exit(main())
