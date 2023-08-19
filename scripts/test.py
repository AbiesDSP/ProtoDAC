import argparse
import subprocess
from pathlib import Path
import shutil

src_dir = Path()
build_dir = Path("build")

cmake_path = Path("C:/msys64/mingw64/bin/cmake.exe")
cmake_cfg = [cmake_path, "-S", src_dir, "-B", build_dir, "-G", "Ninja"]
cmake_build = [cmake_path, "--build", build_dir]

tests_exe = build_dir / "tests" / "run_all_tests.exe"
run_all_tests = [tests_exe]

parser = argparse.ArgumentParser("Test Runner")
parser.add_argument("tag", default="", nargs="*")
parser.add_argument("--clean", action="store_true")
parser.add_argument("--configure", action="store_true")


def main():
    args = parser.parse_args()

    # Delete build dir.
    if args.clean:
        shutil.rmtree(build_dir, ignore_errors=True)

    # Configure CMake
    if args.configure:
        subprocess.run(cmake_cfg, check=True)

    # Build
    subprocess.run(cmake_build, check=True)

    # Run tests
    run_all_tests.extend(args.tag)
    subprocess.run(run_all_tests)


if __name__ == "__main__":
    main()
