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
# This file sets variables that control the way modules are built
# thorughout the system. It should not be used to conditionally
# disable makefiles (the proper mechanism to control what gets
# included in a build is to use PRODUCT_PACKAGES in a product
# definition file).
#

BOWSER_COMMON_FOLDER := device/amazon/bowser-common

# inherit from common
-include device/amazon/omap4-common/BoardConfigCommon.mk

# Sensors
BOARD_USES_GENERIC_INVENSENSE := false

# Bluetooth
BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_BCM := true

# Camera
TI_OMAP4_CAMERAHAL_VARIANT := true
TI_CAMERAHAL_USES_LEGACY_DOMX_DCC := true
TI_CAMERAHAL_MAX_CAMERAS_SUPPORTED := 1
#TI_CAMERAHAL_DEBUG_ENABLED := true
#TI_CAMERAHAL_VERBOSE_DEBUG_ENABLED := true
#TI_CAMERAHAL_DEBUG_FUNCTION_NAMES := true
USE_CAMERA_STUB := false

# inherit from the proprietary version
-include vendor/amazon/bowser-common/BoardConfigVendor.mk

# Kernel
BOARD_KERNEL_PAGESIZE := 2048
TARGET_BOOTLOADER_BOARD_NAME := bowser

# Connectivity - Wi-Fi
BOARD_WPA_SUPPLICANT_DRIVER      := NL80211
WPA_SUPPLICANT_VERSION           := VER_0_8_X
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd
BOARD_WLAN_DEVICE                := bcmdhd
WIFI_DRIVER_FW_PATH_PARAM        := "/sys/module/bcmdhd/parameters/firmware_path"
WIFI_DRIVER_FW_PATH_STA          := "/vendor/firmware/fw_bcmdhd.bin"
PRODUCT_WIRELESS_TOOLS           := true

# Filesystem
BOARD_BOOTIMAGE_PARTITION_SIZE := 8388608
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 8388608
BOARD_CACHEIMAGE_PARTITION_SIZE := 681574400
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 929038336
BOARD_USERDATAIMAGE_PARTITION_SIZE := 13777223680
BOARD_FLASH_BLOCK_SIZE := 131072

# Dolby enhancements
OMAP_ENHANCEMENT_DOLBY_DDPDEC51_MULTICHANNEL := true
ifdef OMAP_ENHANCEMENT_DOLBY_DDPDEC51_MULTICHANNEL
    COMMON_GLOBAL_CFLAGS += -DOMAP_ENHANCEMENT_DOLBY_DDPDEC51_MULTICHANNEL
endif

# Graphics
BOARD_CREATE_AMAZON_HDCP_KEYS_SYMLINK := true

# Recovery
TARGET_RECOVERY_PRE_COMMAND := "echo 0 > /sys/block/mmcblk0boot0/force_ro; echo -n 7 | dd of=/dev/block/mmcblk0boot0 bs=1 count=1 seek=4104 ; sync; \#"
BOARD_CANT_BUILD_RECOVERY_FROM_BOOT_PATCH := true

# TWRP Config
TW_INTERNAL_STORAGE_PATH := "/data/media/0"
TW_INTERNAL_STORAGE_MOUNT_POINT := "data"
BOARD_HAS_NO_REAL_SDCARD := true
RECOVERY_SDCARD_ON_DATA := true
TW_MAX_BRIGHTNESS := 254
TW_CUSTOM_BATTERY_PATH := /sys/class/power_supply/bq27541

# ICS ril version
LEGACY_RIL := true

# hack the ota
TARGET_RELEASETOOL_OTA_FROM_TARGET_SCRIPT := ./$(COMMON_FOLDER)/releasetools/bowser_ota_from_target_files.py
TARGET_RELEASETOOL_MAKE_RECOVERY_PATCH_SCRIPT := ./$(COMMON_FOLDER)/releasetools/bowser_make_recovery_patch
