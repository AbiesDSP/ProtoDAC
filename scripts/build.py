from pathlib import Path
from psoc_creator import PSoCConfig
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--clean", action="store_true")
parser.add_argument("--create", action="store_true")
parser.add_argument("--build", action="store_true")


def main():
    args = parser.parse_args()

    cfg = PSoCConfig.from_yaml_file("HeadphoneDAC.yaml")
    if args.clean:
        cfg.clean()
    if args.build:
        cfg.build()


if __name__ == "__main__":
    main()
