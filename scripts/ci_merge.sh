#!/usr/bin/env bash
# CI helper: merge the controller build into one full-flash image at build/ButterBotCtrl-merged.bin.
# The controller's sdkconfig uses flash size 'detect', which esptool merge_bin rejects; 'keep' leaves
# the size in the bootloader header as built (same as the official release images).
# Lives in a script because the CI action word-splits its 'command' input and drops quotes.
set -euo pipefail
cd "$(dirname "$0")/../build"
sed 's/--flash_size detect/--flash_size keep/' flash_args > flash_args_merge
python -m esptool --chip esp32s3 merge_bin -o ButterBotCtrl-merged.bin @flash_args_merge
ls -l ButterBotCtrl-merged.bin
