"""Script to build and clean the PSoC Creator Project"""
from psoc_creator import PSoCConfig
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--clean", action="store_true")


class Args:
    clean: bool


def main():
    args: Args = parser.parse_args()

    cfg = PSoCConfig.from_yaml_file("HeadphoneDAC.yaml")
    if args.clean:
        cfg.clean()
    cfg.build()


if __name__ == "__main__":
    main()
