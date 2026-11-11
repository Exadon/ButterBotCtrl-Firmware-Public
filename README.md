# ButterBot Controller Firmware

> **This is a community fork** of [CircuitMess/ButterBotCtrl-Firmware-Public](https://github.com/CircuitMess/ButterBotCtrl-Firmware-Public),
> the companion to the robot fork [Exadon/ButterBot-Firmware-Public](https://github.com/Exadon/ButterBot-Firmware-Public).
> It works with both the stock and the forked robot firmware.

## What's different in this fork

- **No crash on truncated robot messages:** a BLE notification shorter than its header claims used to be read past its end. It's now dropped.
- **No crash on bad window updates:** live updates to an open window are only applied if they match the size the window was created with. Before, an empty update dereferenced null.
- **Settings survive firmware updates.** They used to reset to defaults whenever the settings layout changed.
- A leftover debug `printf` on every BLE event is now a debug log.

## Installing a release

Every `v*` tag builds a single merged image on GitHub Actions and attaches it to the [releases page](../../releases).
The easiest way to install it is **Dimension Hopper** (in the robot fork's `tools/updater`): in the Controller slot,
choose *Latest GitHub release* and pick `Exadon/ButterBotCtrl-Firmware-Public`.

## Building

To build the ButterBot Controller firmware, you'll need the ESP-IDF. You can find the getting
started guide [here](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/).
The production firmware is built using IDF version 5.5.3.

All required components are contained in the repository, and managed components (LVGL,
esp_new_jpeg) are fetched automatically by the IDF component manager during the build.

In the root directory of the project:

**To build the firmware** run ```idf.py build```

**To upload the firmware to the device** run ```idf.py -p <PORT> flash```. Replace `<PORT>` with
the port the controller is attached to, for ex. ```COM6``` or ```/dev/ttyACM0```.

### Asset archives

The SPIFFS filesystem image is built automatically from the pre-generated archives in
[spiffs_image](spiffs_image). If you modify the source assets in
[spiffs_archives](spiffs_archives), regenerate the archives by running
[scripts/archive.sh](scripts/archive.sh) from the project root. See
[scripts/archive.sh.md](scripts/archive.sh.md) for details.

## Restoring the stock firmware

To restore the stock firmware, you can download the prebuilt binary on
the [releases page](https://github.com/CircuitMess/ButterBotCtrl-Firmware-Public/releases) of
this repository and flash it manually using esptool:

```shell
esptool -c esp32s3 -b 921600 -p <PORT> write_flash 0 ButterBotCtrl-Firmware.bin
```

Alternatively, you can also do so using [CircuitBlocks](https://code.circuitmess.com/) by
logging in, clicking the "Restore Firmware" button in the top-right corner, and following the
on-screen instructions.
