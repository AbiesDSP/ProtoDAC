# USB Headphone DAC

Uses a PSoC 5LP microcontroller as a USB to I2S bridge.

# Installation and Building

Use PSoC Creator 4.4 to build the PSoC project. Make sure this program has been installed and set up.

If there are any errors building, you may need to "update components" in the psoc creator project.

Clone and build the project
```bash
git clone https://github.com/AbiesDSP/HeadphoneDAC.git
cd HeadphoneDAC
```

Install the requirements (Optionally create a virtual environment first)

```bash
pip install -r requirements.txt
```

There are scripts to build the project, and update the firmware
on the device.

Update the firmware from a release

```bash
# unzip release
cd release_dir/HeadphoneDAC_vx.x.x.x
python bootload.py
```

Build the project and update the device's firmware

```bash
python ./scripts/buildp.py
python ./scripts/bootload.py --dev
```



### View Log messages
```bash
python ./scripts/listen.py
```

### Using Virtual environments (Optional)
Set up a python virtual environment.

Windows:
```bash
python -m venv venv
. ./venv/Scripts/activate
```

Linux/Mac:
```bash
python -m venv venv
. ./venv/bin/activate
```

### Running Tests (Optional)

Use CMake/Python/VSCode to build and run tests. Catch2 test framework required for tests.

