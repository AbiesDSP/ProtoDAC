"""
Scripts to build PSoC Creator projects from the command line interface.
The project can be configured in "HeadphoneDAC.yaml"
"""
import os
import re
from pathlib import Path
import subprocess
import shutil
from dataclasses import dataclass
from dataclass_wizard import YAMLWizard


def version_from_file(version_file: Path) -> str:
    """Read the version from a file and return as a formatted string."""

    with open(version_file, "r") as f:
        alldata = f.read()

    major = int(re.findall(r"VERSION_MAJOR\s*(\d+)", alldata)[0])
    minor = int(re.findall(r"VERSION_MINOR\s*(\d+)", alldata)[0])
    patch = int(re.findall(r"VERSION_PATCH\s*(\d+)", alldata)[0])
    dev = int(re.findall(r"VERSION_DEV\s*(\d+)", alldata)[0])

    return f"{major}.{minor}.{patch}.{dev}"


@dataclass
class PSoCConfig(YAMLWizard):
    workspace: str
    projects: list[str]
    version: str
    build_type: str

    psoc_creator_location: str = "C:/Program Files (x86)/Cypress/PSoC Creator"
    psoc_creator_version: str = "4.4"
    sub_path: str = "CortexM3/ARM_GCC_541"

    def __post_init__(self):
        """If the version is given in a file, parse it."""
        version_file = Path(self.version)

        if version_file.exists():
            self.version = version_from_file(version_file)

    def clean(self):
        """Deletes all generated sources, cyfit file, and runs Clean in PSoC Creator for all projects in the workspace."""

        for prj in self.projects:
            prj_root = Path(prj)
            # Delete all of generated source and cyfit file.
            generated_source_dir = prj_root / "Generated_Source"
            cyfit = prj_root / f"{prj_root.stem}.cyfit"

            print(f"Deleting {generated_source_dir}")
            shutil.rmtree(generated_source_dir, ignore_errors=True)

            build_dir = prj_root / self.sub_path
            print(f"Deleting {build_dir}")
            shutil.rmtree(build_dir, ignore_errors=True)

            if cyfit.exists():
                print(f"Deleting {cyfit}")
                os.remove(cyfit)

        # Run the psoc clean command.
        cmd = [
            self.cyprjmgr_bin(),
            "-wrk",
            self.workspace,
            "-clean",
            "-prj",
            prj_root.stem,
        ]
        print(f"Cleaning Workspace {self.workspace}")
        subprocess.run(cmd, check=True)

    def build(self):
        """Build all projects in the workspace"""
        for prj in self.projects:
            cmd = [
                self.cyprjmgr_bin(),
                "-wrk",
                self.workspace,
                "-build",
                "-prj",
                Path(prj).stem,
                "-c",
                self.build_type,
            ]
            subprocess.run(cmd, check=True)

    def cyprjmgr_bin(self):
        """Path to cyprjmgr cli tool."""
        return (
            Path(self.psoc_creator_location)
            / self.psoc_creator_version
            / "PSoC Creator/bin/cyprjmgr.exe"
        )


if __name__ == "__main__":
    cfg = PSoCConfig.from_yaml_file("main_config.yaml")
    print(cfg)
