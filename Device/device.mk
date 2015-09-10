#
# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

DEVICE_FOLDER := device/amazon/tate
TARGET_BOOTLOADER_BOARD_SUBTYPE := tate

PRODUCT_PROPERTY_OVERRIDES := \
    ro.carrier=wifi-only

PRODUCT_COPY_FILES += \
    device/amazon/tate/fstab.tate:/root/fstab.bowser \
    device/amazon/tate/init.bowser.rc:root/init.bowser.rc \
    device/amazon/tate/init.bowser.usb.rc:root/init.bowser.usb.rc \
    device/amazon/tate/init.recovery.bowser.rc:root/init.recovery.bowser.rc \
    device/amazon/tate/ueventd.bowser.rc:root/ueventd.bowser.rc

# the actual meat of the device-specific product definition
$(call inherit-product, device/asus/grouper/device-common.mk)
$(call inherit-product, device/amazon/bowser-common/common.mk)

# inherit from the non-open-source side, if present
$(call inherit-product-if-exists, vendor/amazon/tate/tate-vendor.mk)
$(call inherit-product-if-exists, vendor/amazon/omap4-common/omap4-common-vendor-540_120.mk)

PRODUCT_PROPERTY_OVERRIDES += \
    ro.carrier=wifi-only

DEVICE_PACKAGE_OVERLAYS := \
    device/amazon/tate/overlay

# Device settings
ADDITIONAL_BUILD_PROPERTIES += \
    ro.sf.lcd_density=213 \
    persist.hwc.mirroring.region=0:0:800:1280 \
    persist.hwc.mirroring.transform=3 \
    omap.audio.mic.main=DMic0L \
    omap.audio.mic.sub=DMic0R \
    sys.usb.vid=1949 \
    sys.usb.pid=0007 \
    usb.vendor=1949 \
    usb.product.adb=0007 \
    usb.product.mtpadb=0007 \
    ro.cwm.forbid_format=/bootloader,/xloader,/misc

# RIL turn off
ADDITIONAL_BUILD_PROPERTIES += \
    keyguard.no_require_sim=1 \
    ro.radio.use-ppp=no \
    ro.config.nocheckin=yes \
    ro.radio.noril=yes
