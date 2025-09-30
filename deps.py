#!/bin/python3.13

import sys
import os


def main() -> None:
    file = "CMakeLists.txt"
    with open(file, "r") as f:
        data = f.read()
        lines = data.split("\n")
        dependencies = ""
        depstart_idx = 0
        for line in lines:
            if line.startswith("target_link_libraries(${PROJECT_NAME}"):
                dependencies = line.split(" ")
                depstart_idx = 2
                break

        dependencies_total = []
        for dep in dependencies[depstart_idx:]:
            depname = dep
            if (depname.endswith(")")):
                depname = dep[:-1]

            dependencies_total.append(depname)
        
        print(" | ".join(dependencies_total))


if __name__ == "__main__":
    main()
