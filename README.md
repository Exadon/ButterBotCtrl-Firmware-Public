# ButterBot Controller Firmware (community fork)

> A community fork of [CircuitMess/ButterBotCtrl-Firmware-Public](https://github.com/CircuitMess/ButterBotCtrl-Firmware-Public).
> It works with both the stock and the forked robot firmware.

This is one of three pieces that work together - the **robot firmware**
([Exadon/ButterBot-Firmware-Public](https://github.com/Exadon/ButterBot-Firmware-Public), which also has the overview),
this **controller firmware**, and **Dimension Hopper**, the Windows app that updates and sets up both
([tools/updater](https://github.com/Exadon/ButterBot-Firmware-Public/tree/master/tools/updater) in the robot repository).

[![The controller's features in action](https://img.youtube.com/vi/ISTigiyBVUY/hqdefault.jpg)](https://www.youtube.com/shorts/ISTigiyBVUY)

*The controller in action (click to watch). ButterBot's AI mode: [video](https://www.youtube.com/shorts/QA3H6BbMbJw).*

## What the fork adds

### AI conversations
- Shows what the robot is doing while it talks to its AI brain: *Listening… reluctantly*, *Heard you. Unfortunately.*
  and an animated *Consulting my brain…* while the PC thinks.
- Copes with screens a newer robot might send that it doesn't know (shows "..." instead of crashing).

### Driving and menus
- Joystick centre calibration and hysteresis, so the stick doesn't drift or flicker between menu items.
- An RC drive speed setting; an exit hint in RC mode, and pressing exits.
- Push the stick to open the menu; *Shut Up* selects and *Manual Override* goes back; *Settings* is back on top.
- Screens ignore the release of a button that was pressed before they opened (no more accidental selections).
- Action list folders, chat and portal windows, and *Open a Portal* in the menu, matching the robot fork's new commands.

### Sturdier
- Pairing recovers from a failed attempt instead of getting stuck; *Forget pairing* from Dimension Hopper.
- No crash on a truncated message from the robot or on a mismatched window update.
- Settings survive firmware updates.
- A fork badge on the boot screen, so you can tell which firmware is running.

### USB console (used by Dimension Hopper)
A status line every 5 seconds (battery, link, firmware, why it last restarted), a live diagnostics stream for the
*Diagnostics* tab, pairing logs, a `forget` command, and `scr`, which sends what the screen shows so Dimension
Hopper can mirror it live (only the rows that changed, compressed).

## Problems and ideas

Issues for all three pieces are collected in one place: the
[robot repository's issues](https://github.com/Exadon/ButterBot-Firmware-Public/issues). Its
[troubleshooting guide](https://github.com/Exadon/ButterBot-Firmware-Public/blob/master/tools/updater/README.md#troubleshooting)
explains what the robot's messages mean.

## Installing a release

Every `v*` tag builds a single merged image on GitHub Actions and attaches it to the [releases page](../../releases).
The easiest way to install it is **Dimension Hopper** (download `DimensionHopper.exe` from the
[robot fork's releases](https://github.com/Exadon/ButterBot-Firmware-Public/releases)): in the Controller slot, choose
*Latest GitHub release* and pick `Exadon/ButterBotCtrl-Firmware-Public`. Updates keep your pairing and settings.

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
