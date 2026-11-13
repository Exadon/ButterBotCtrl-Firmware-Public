#!/usr/bin/env bash
# CI helper: build the controller firmware inside the espressif/idf docker image.
#
# Why the pinned component manager: the controller pulls LVGL (and LovyanGFX) in through Kconfig-gated
# dependencies in components/CMF/idf_component.yml. On a fresh build the component manager first runs before
# LVGL is downloaded, so kconfgen doesn't know any CONFIG_LV_* symbol yet and drops them from sdkconfig.
# idf-component-manager 2.5.2 backs sdkconfig up before that first pass and restores it afterwards; the
# older one bundled in the espressif/idf:v5.5.3 image does not, so LVGL ended up compiled with its Kconfig
# defaults (calendar/menu/msgbox widgets on, different sizes and layout settings) - an app ~110 KB larger
# with an off-centre boot logo and no battery icons on the home screen. The local Windows install uses 2.5.2.
set -euo pipefail
cd "$(dirname "$0")/.."
git config --global --add safe.directory "$PWD"

echo "component manager in the image: $(python -m pip show idf-component-manager | grep -i '^version')"
python -m pip install --quiet "idf-component-manager==2.5.2"
python -m pip show idf-component-manager | grep -i '^version'

idf.py build

# Guard: the build must not have rewritten the committed sdkconfig, and LVGL must have been compiled with it.
if ! git diff --quiet -- sdkconfig; then
    echo "ERROR: the build changed sdkconfig (Kconfig values were lost):"
    git diff --stat -- sdkconfig
    git diff -- sdkconfig | head -80
    exit 1
fi
if ! grep -q '^#define CONFIG_LV_COLOR_DEPTH ' build/config/sdkconfig.h; then
    echo "ERROR: build/config/sdkconfig.h has no LVGL settings; LVGL would be built with its defaults"
    exit 1
fi
if grep -q '^#define CONFIG_LV_USE_CALENDAR ' build/config/sdkconfig.h; then
    echo "ERROR: build/config/sdkconfig.h has LVGL's default widget set, not the project's"
    exit 1
fi

bash scripts/ci_merge.sh
