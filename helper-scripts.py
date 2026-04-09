#!/usr/bin/env python3
import argparse
import subprocess
import sys
import shutil
import os
from pathlib import Path

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
        use when building", default="GLAD", choices=["GLAD", "GLEW", "LIBEPOXY"])
parser.add_argument("--build-editor", action="store_true", help="Whether to build editor \
        (Very finicky and requires Qt)")
parser.add_argument("--smoke-vulkan", action="store_true", help="Runs the Windows Vulkan backend smoke tests")


def is_windows_env() -> bool:
    return sys.platform == "win32"


def read_cmake_cache_var(build_dir: str, key: str) -> str | None:
    cache_path = Path(build_dir) / "CMakeCache.txt"
    if not cache_path.is_file():
        return None

    prefix = key + ":"
    with cache_path.open("r", encoding="utf-8", errors="ignore") as cache_file:
        for line in cache_file:
            if line.startswith(prefix):
                _, _, value = line.partition("=")
                return value.strip()
    return None


def to_cmake_path(path: Path | str) -> str:
    return str(path).replace("\\", "/")


def ensure_clean_build_dir(build_dir: str, desired_generator: str | None = None) -> None:
    current_generator = read_cmake_cache_var(build_dir, "CMAKE_GENERATOR")
    if desired_generator is None or current_generator is None:
        return

    if current_generator != desired_generator:
        print(f"[reconfigure] removing incompatible build directory ({current_generator} -> {desired_generator})")
        shutil.rmtree(build_dir, ignore_errors=True)


def find_windows_toolchain() -> dict[str, str] | None:
    candidates = [
        Path(r"C:\Qt\Tools\mingw1310_64\bin"),
    ]

    env_root = os.environ.get("MINGW_ROOT")
    if env_root:
        candidates.insert(0, Path(env_root) / "bin")

    path_gcc = shutil.which("gcc")
    if path_gcc:
        candidates.insert(0, Path(path_gcc).resolve().parent)

    for bin_dir in candidates:
        gcc = bin_dir / "gcc.exe"
        gxx = bin_dir / "g++.exe"
        windres = bin_dir / "windres.exe"
        mingw_make = bin_dir / "mingw32-make.exe"
        if gcc.is_file() and gxx.is_file() and windres.is_file():
            toolchain = {
                "bin_dir": to_cmake_path(bin_dir),
                "gcc": to_cmake_path(gcc),
                "gxx": to_cmake_path(gxx),
                "windres": to_cmake_path(windres),
            }
            if mingw_make.is_file():
                toolchain["mingw_make"] = to_cmake_path(mingw_make)

            ninja = Path(r"C:\Qt\Tools\Ninja\ninja.exe")
            if ninja.is_file():
                toolchain["ninja"] = to_cmake_path(ninja)
            else:
                ninja_path = shutil.which("ninja")
                if ninja_path:
                    toolchain["ninja"] = to_cmake_path(ninja_path)

            return toolchain

    return None


def build_subprocess_env(toolchain: dict[str, str] | None = None) -> dict[str, str]:
    env = os.environ.copy()
    if toolchain is None:
        return env

    extra_paths = [toolchain["bin_dir"]]
    if "ninja" in toolchain:
        extra_paths.append(str(Path(toolchain["ninja"]).resolve().parent))

    existing_path = env.get("PATH", "")
    env["PATH"] = os.pathsep.join(extra_paths + [existing_path])
    return env


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

        commands = [
            # xdg-shell
            ["wayland-scanner", "client-header",
             "/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml",
             "src/include/rnative/xdg-shell-client-protocol.h"],

            ["wayland-scanner", "private-code",
             "/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml",
             "src/rnative/xdg-shell-client-protocol.c"],

            # xdg-toplevel-icon-v1
            ["wayland-scanner", "client-header",
             "/usr/share/wayland-protocols/staging/xdg-toplevel-icon/xdg-toplevel-icon-v1.xml",
             "src/include/rnative/xdg-toplevel-icon-v1-client-protocol.h"],

            ["wayland-scanner", "private-code",
             "/usr/share/wayland-protocols/staging/xdg-toplevel-icon/xdg-toplevel-icon-v1.xml",
             "src/rnative/xdg-toplevel-icon-v1-client-protocol.c"],
        ]

        for cmd in commands:
            print("> command:", " ".join(cmd))
            subprocess.run(cmd, check=True)

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
    args = ["--include-ext=cpp,hpp", "--exclude-dir=rnative,glm", "src/", "include/"]

    print("command: " + command + " " + " ".join(args))
    subprocess.run([command] + args)
    return 0


def is_executable(path):
    return os.path.isfile(path) and os.access(path, os.X_OK)


def run_tests():
    root_dir = Path(__file__).resolve().parent
    test_dir = root_dir / "bin" / "tests"
    test_env = root_dir / "bin"
    print("RocketGE: Test Runner")
    print("Tests from: " + str(test_dir))
    print("Test Env: " + str(test_env))
    if not test_dir.is_dir():
        print(f"Test dir '{test_dir}' not found")
        sys.exit(1)

    failed = False
    timed_out_tests = []
    test_timeout_seconds = 20

    count = 0.0
    for name in sorted(os.listdir(test_dir)):
        if (sys.platform == "win32" or os.path.exists("WinDeps")) and not name.endswith(".exe"):
            continue
        count += 1.0

    done = 0.0
    passed = 0.0
    failed_tests = []
    for name in sorted(os.listdir(test_dir)):
        if (sys.platform == "win32" or os.path.exists("WinDeps")) and not name.endswith(".exe"):
            continue
        path = test_dir / name

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
        try:
            result = subprocess.run(
                [str(path.resolve()), "--", "--unit-test"],
                cwd=str(test_env.resolve()),
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                timeout=test_timeout_seconds
            )
        except subprocess.TimeoutExpired:
            failed = True
            timed_out_tests.append(name)
            done += 1
            continue

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
        for test in timed_out_tests:
            print(test + " [timeout]", end=", ")
        print()
    # print(str(int(passed)) + "/" + str(int(done)) + " tests passed")
    sys.exit(1 if failed else 0)


def run_vulkan_smoke() -> int:
    root_dir = Path(__file__).resolve().parent
    suffix = ".exe" if is_windows_env() else ""
    smoke_targets = [
        root_dir / "bin" / "tests" / ("default_shader_test" + suffix),
        root_dir / "bin" / "examples" / ("moving_circle" + suffix),
        root_dir / "bin" / "examples" / ("custom_shader" + suffix),
    ]

    print("RocketGE: Vulkan Smoke Runner")
    print("Working directory: " + str(root_dir))
    print("Backend: vulkan:any")
    print("Mode: launch from repo root and require the process to stay alive for 5 seconds")
    print("Note: on Windows this verifies the native Vulkan backend path")

    timeout_seconds = 5
    failed = False

    for target in smoke_targets:
        if not target.is_file():
            print(f"FAIL: missing target '{target}'")
            failed = True
            continue

        command = [str(target.resolve()), "--renderer-backend", "vulkan:any"]
        print("> command:", " ".join(command))
        try:
            proc = subprocess.Popen(
                command,
                cwd=str(root_dir),
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
        except OSError as exc:
            print(f"FAIL: could not start '{target.name}': {exc}")
            failed = True
            continue

        try:
            output, _ = proc.communicate(timeout=timeout_seconds)
            print(f"FAIL: '{target.name}' exited before {timeout_seconds}s with code {proc.returncode}")
            if output:
                print(output[-4000:])
            failed = True
        except subprocess.TimeoutExpired:
            proc.terminate()
            try:
                proc.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.communicate()
            print(f"PASS: '{target.name}' stayed alive for {timeout_seconds}s")

    return 1 if failed else 0


def init_cmake(args):
    commands = ["cmake"]
    for command in commands:
        if not shutil.which(command):
            print(f"{command} was not found in PATH.")
            return 1
    linker = "mold" # Preferred
    if not shutil.which(linker) and not is_windows_env():
        print("[fallback] using ld instead of mold")
        linker = "ld"

    desired_generator = None
    toolchain = None
    env = None
    if is_windows_env():
        toolchain = find_windows_toolchain()
        if toolchain is None:
            print("No compatible MinGW toolchain was found on this Windows machine.")
            return 1
        desired_generator = "Ninja" if "ninja" in toolchain else "MinGW Makefiles"
        env = build_subprocess_env(toolchain)

    ensure_clean_build_dir("build", desired_generator)
    os.makedirs("build", exist_ok=True)

    backend = args.glfnldr_backend
    cmake_args = [
        "-S", ".",
        "-B", "build",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        "-DGLFNLDR_BACKEND=" + backend,
    ]
    if desired_generator in ["Ninja", "MinGW Makefiles"]:
        cmake_args.append("-DCMAKE_BUILD_TYPE=Debug")
    if shutil.which("ccache") and not is_windows_env():
        cmake_args.extend([
            "-DCMAKE_C_COMPILER_LAUNCHER=ccache",
            "-DCMAKE_CXX_COMPILER_LAUNCHER=ccache",
        ])
    for item in ["-DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=" + linker, \
                 "-DCMAKE_SHARED_LINKER_FLAGS=-fuse-ld=" + linker, \
                 "-DCMAKE_MODULE_LINKER_FLAGS=-fuse-ld=" + linker]:
        if not is_windows_env():
            cmake_args.append(item)

    if args.build_editor:
        cmake_args.append("-DBUILD_EDITOR=ON")
 
    if is_windows_env():
        cmake_args.extend(["-G", desired_generator])
        cmake_args.append("-DCMAKE_C_COMPILER=" + toolchain["gcc"])
        cmake_args.append("-DCMAKE_CXX_COMPILER=" + toolchain["gxx"])
        cmake_args.append("-DCMAKE_RC_COMPILER=" + toolchain["windres"])
        if desired_generator == "Ninja":
            cmake_args.append("-DCMAKE_MAKE_PROGRAM=" + toolchain["ninja"])
        elif "mingw_make" in toolchain:
            cmake_args.append("-DCMAKE_MAKE_PROGRAM=" + toolchain["mingw_make"])
        cmake_args.append("-DBUILD_SHARED_LIBS=OFF")
        cmake_args.append("-D__rge_WINDOWS__=ON")

    print("command: " + "cmake" + " " + " ".join(cmake_args))
    return subprocess.run(["cmake"] + cmake_args, env=env).returncode


def build(args):
    init_code = 0
    if not os.path.exists("build"):
        init_code = init_cmake(args)
    if init_code != 0:
        return init_code

    rnative_code = 0
    if not os.path.exists("build"):
        rnative_code = build_rnative()
    if rnative_code != 0:
        return rnative_code

    cpus = os.cpu_count()

    if os.path.exists("build/build.ninja") and (cpus + 2 != cpus * 2):
        cpus += 2 # Ninja scales better with 2 extra jobs

    build_args = ["--build", "build", "--parallel", str(cpus)]

    build_env = None
    if is_windows_env():
        toolchain = find_windows_toolchain()
        if toolchain is None:
            print("No compatible MinGW toolchain was found on this Windows machine.")
            return 1
        build_env = build_subprocess_env(toolchain)

    print("> command: " + "cmake" + " " + " ".join(build_args))
    prc = subprocess.run(["cmake"] + build_args, env=build_env)

    cmake_code = prc.returncode

    return cmake_code

def main() -> int:
    print("""________            ______      _______________________
___  __ \\______________  /________  /__  ____/__  ____/
__  /_/ /  __ \\  ___/_  //_/  _ \\  __/  / __ __  __/   
_  _, _// /_/ / /__ _  ,<  /  __/ /_ / /_/ / _  /___   
/_/ |_| \\____/\\___/ /_/|_| \\___/\\__/ \\____/  /_____/""")
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
    if args.smoke_vulkan:
        return_codes.append(run_vulkan_smoke())

    return max(return_codes, default=1)

if __name__ == "__main__":
    exit(main())

