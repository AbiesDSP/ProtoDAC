# USB Headphone DAC

Uses a PSoC 5LP microcontroller as a USB to I2S bridge.

# Installation and Building

Use PSoC Creator 4.4 to build the PSoC project. Make sure this program has been installed and set up.

If there are any errors building, you may need to "update components" in the psoc creator project.

## Python scripts

There are scripts to build the project, and update the firmware
on the device.

Set up a python virtual environment.

```bash
python -m venv venv
. ./venv/Scripts/activate
```

or on Linux/Mac
```bash
python -m venv venv
. ./venv/bin/activate
```

Install the requirements

```bash
pip install -r requirements.txt
```

then build the project and update the
device's firmware

```bash
python ./scripts/buildp.py
python ./scripts/bootload.py
```

### View Log messages
```bash
python ./scripts/listen.py
```

### Running Tests (Optional)

Use CMake/Python/VSCode to build and run tests. Catch2 test framework required for tests.

