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

        print("> command: " + " ".join(gen_header))
        subprocess.run(gen_header)

        print("> command: " + " ".join(gen_source))
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
    TEST_ENV = "./bin"
    print("RocketGE: Test Runner")
    print("Tests from: " + TEST_DIR)
    print("Test Env: " + TEST_ENV)
    if not os.path.isdir(TEST_DIR):
        print(f"Test dir '{TEST_DIR}' not found")
        sys.exit(1)

    failed = False

    count = 0.0
    for name in sorted(os.listdir(TEST_DIR)):
        count += 1.0

    done = 0.0
    passed = 0.0
    failed_tests = []
    for name in sorted(os.listdir(TEST_DIR)):
        if (sys.platform == "win32" or os.path.exists("toolchain.cmake")) and not name.endswith(".exe"):
            continue
        path = os.path.join(TEST_DIR, name)

        if not is_executable(path):
            done += 1
            continue

        # print("[" + str(int((done / count) * 100)) + "%]", end="", flush=True)
        
        name_strip = name
        if name.endswith(".exe"):
            name_strip = name[:-4]
        print("\r\033[K", end="")
        print("\r" + " " * 24, end="")
        print("\r[", end="")
        progress = int((done / count) * 20)
        remaining = 20 - progress
        print("#" * progress, end="")
        print(" " * remaining, end="")
        print("]", end="")
        print(" " + "Running: " + name_strip, end="")

        print(end="", flush=True)
        result = subprocess.run(
            ["../" + TEST_DIR + "/" + name, "--", "--unit-test"],
            cwd=TEST_ENV,
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
        )

        if result.returncode != 0:
            failed = True
            failed_tests.append(name)
        else:
            passed += 1
        done += 1

    print("\r\033[K", end="")
    print("\r" + " " * 24, end="")
    print("\r[", end="")
    progress = int((done / count) * 20)
    remaining = 20 - progress
    print("#" * progress, end="")
    print(" " * remaining, end="")
    print("]", end="")
    print(" " + str(int(passed)) + "/" + str(int(done)) + " tests")
    if failed:
        print("FAIL: ", end="")
        for test in failed_tests:
            print(test, end=", ")
        print()
    # print(str(int(passed)) + "/" + str(int(done)) + " tests passed")
    sys.exit(1 if failed else 0)


def init_cmake(args):
    commands = ["cmake", "git"]
    for command in commands:
        if not shutil.which(command):
            print(f"{command} was not found in PATH.")
            return 1

    os.makedirs("build", exist_ok=True)

    if sys.platform == "win32":
        print("run build.sh")
        return 1
    #     if not os.path.isdir("windeps"):
    #         print("installation of dependencies required")
    #         if not os.path.isdir(".gitwindeps"):
    #             os.mkdir(".gitwindeps")
    #         cmd = ["git"]
    #         args = ["clone", "https://github.com/NOERLOL-Mirrors/rocket-runtime-windeps", "--depth", "1", ".gitwindeps"]
    #         print("command: " + "git" + " " + " ".join(args))
    #         # subprocess.run(cmd + args, check=True)
    #         src = ".gitwindeps/windeps"
    #         dst = "."
    #         if os.path.isdir(src):
    #             for item in os.listdir(src):
    #                 s = os.path.join(src, item)
    #                 d = os.path.join(dst, item)
    #                 # Move each file/folder
    #                 if os.path.exists(d):
    #                     print(f"overwrite {d}")
    #                     if os.path.isdir(d):
    #                         shutil.rmtree(d)
    #                     else:
    #                         os.remove(d)
    #                 shutil.move(s, d)

    backend = args.glfnldr_backend
    args = ["-DCMAKE_EXPORT_COMPILE_COMMANDS=ON", "-DGLFNLDR_BACKEND=" + backend, "-B", "build"]
 
    if sys.platform == "win32":
        args.append("-DBUILD_SHARED_LIBS=OFF")
        args.append("-D__rge_WINDOWS__=ON")

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
    return_codes = []

    if args.get_deps:
        get_deps()
    if args.print_loc:
        return_codes.append(print_loc())
    if args.build_rnative:
        return_codes.append(build_rnative())
    if args.init_cmake:
        return_codes.append(init_cmake(args))
    if args.build:
        return_codes.append(build(args))
    if args.run_tests:
        return_codes.append(run_tests())

    return max(return_codes, default=1)

if __name__ == "__main__":
    exit(main())
