#!/usr/bin/env python3
import argparse
import subprocess
import sys


parser: argparse.ArgumentParser = argparse.ArgumentParser()
parser.add_argument("--get-deps", action="store_true", help="Shows the dependencies needed to build")
parser.add_argument("--build-rnative", action="store_true", help="Builds the dependencies for RNative")


def get_deps() -> None:
    deps: list[str] = ["GLFW", "SDL", "OpenGL", "GLEW", "OpenAL", "miniz"]
    info: list[str] = [">= 3.4", "== 2.0", "any", ">= 1.5", "(OpenAL-soft) any", ">= 3.0"]

    for dep, ver in zip(deps, info):
        print(f"{dep}: {ver}")


def build_rnative() -> int:
    print("checking operating system... ", end="")
    if sys.platform.startswith("linux"):
        print("linux")
        print("generate source and header for: xdg-toplevel-icon-v1")
        gen_header = ["wayland-scanner", "client-header", "/usr/share/wayland-protocols/staging/xdg-toplevel-icon/xdg-toplevel-icon-v1.xml", "src/include/rnative/xdg-toplevel-icon-v1-client-protocol.h"]
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


def main() -> int:
    args = parser.parse_args()

    if args.get_deps:
        get_deps()
    elif args.build_rnative:
        return build_rnative()
    else:
        parser.print_help()
        return 1

    return 0


if __name__ == "__main__":
    exit(main())
