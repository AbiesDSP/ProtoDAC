#!/usr/bin/env python3

"""Script to build and clean the PSoC Creator Project"""
from psoc_creator import PSoCConfig
import argparse

parser = argparse.ArgumentParser("Build the PSoC Creator Project")
parser.add_argument("--clean", action="store_true")
parser.add_argument("--config", action="store", default="ProjectConfig.yaml")


class Args:
    clean: bool
    config: str


def main():
    args: Args = parser.parse_args()

    cfg = PSoCConfig.from_yaml_file(args.config)

    if args.clean:
        cfg.clean()
    cfg.build()


if __name__ == "__main__":
    main()
