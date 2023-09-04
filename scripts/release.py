#!/usr/bin/env python3

"""
Generate a release package containing the .cyacd file, .hex file, register map, and scipts.

"""
from psoc_creator import PSoCConfig
import argparse
from pathlib import Path
import os
import shutil
import zipfile

parser = argparse.ArgumentParser()
parser.add_argument("--release-dir", action="store", default="release")
parser.add_argument("--config", action="store", default="ProjectConfig.yaml")


class Args:
    release_dir: str
    config: str


def create_zip(zip_path: Path, cfg: PSoCConfig):
    """"""
    # zf = zipfile.ZipFile(subdir.with_suffix(".zip"), "w", zipfile.ZIP_DEFLATED)
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zf:
        for f in cfg.release_files:
            zf.write(f, Path(f).name)


def main():
    args: Args = parser.parse_args()

    cfg = PSoCConfig.from_yaml_file(args.config)
    # Clean and build the project first.
    cfg.clean()
    cfg.build()

    # Create release folder and release sub dir
    os.makedirs(Path(args.release_dir), exist_ok=True)
    zipdir = Path(args.release_dir) / f"{cfg.project}_v{cfg.version}.zip"

    # Create zip archive.
    create_zip(zipdir, cfg)

    subdir = zipdir.with_suffix("")
    # Create uncompressed copy as well
    os.makedirs(subdir, exist_ok=True)
    # Copy release files into release dir
    for f in cfg.release_files:
        shutil.copy(f, subdir)


if __name__ == "__main__":
    main()
