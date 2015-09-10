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

ifeq ($(TARGET_PREBUILT_KERNEL),)
  LOCAL_KERNEL := device/amazon/tate/kernel
else
  LOCAL_KERNEL := $(TARGET_PREBUILT_KERNEL)
endif

PRODUCT_AAPT_CONFIG := normal large
PRODUCT_AAPT_PREF_CONFIG := tvdpi
# A list of dpis to select prebuilt apk, in precedence order.
PRODUCT_AAPT_PREBUILT_DPI := hdpi

PRODUCT_PROPERTY_OVERRIDES := \
    wifi.interface=wlan0 \
    wifi.supplicant_scan_interval=15 \

BOARD_VENDOR := amazon


# libhwui flags
PRODUCT_PROPERTY_OVERRIDES += \
    debug.hwui.render_dirty_regions=false

# Set default USB interface
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
    persist.sys.usb.config=mtp

include frameworks/native/build/tablet-7in-hdpi-1024-dalvik-heap.mk

PRODUCT_COPY_FILES += \
    $(LOCAL_KERNEL):kernel \
    device/amazon/tate/init.bowser-common.rc:/root/init.bowser-common.rc
    device/amazon/tate/init.omap4.rc:/root/init.omap4.rc
    device/amazon/tate/gps.conf:/system/etc/gps.conf






PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/tablet_core_hardware.xml:system/etc/permissions/tablet_core_hardware.xml \
    frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml \
    frameworks/native/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
    frameworks/native/data/etc/android.hardware.sensor.gyroscope.xml:system/etc/permissions/android.hardware.sensor.gyroscope.xml \
    frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/native/data/etc/android.software.sip.voip.xml:system/etc/permissions/android.software.sip.voip.xml \
    frameworks/native/data/etc/android.hardware.usb.host.xml:system/etc/permissions/android.hardware.usb.host.xml \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/native/data/etc/android.hardware.ethernet.xml:system/etc/permissions/android.hardware.ethernet.xml


PRODUCT_COPY_FILES += \
    device/amazon/tate/prebuilt/usr/idc/AtmelTouch.idc:system/usr/idc/AtmelTouch.idc \
    device/amazon/tate/prebuilt/usr/idc/cyttsp4-i2c.idc:system/usr/idc/cyttsp4-i2c.idc \
    device/amazon/tate/prebuilt/usr/keylayout/gpio-keys.kl:system/usr/keylayout/gpio-keys.kl

PRODUCT_PACKAGES += \
    libwpa_client \
    hostapd \
    dhcpcd.conf \
    wpa_supplicant \
    wpa_supplicant.conf \
    com.android.future.usb.accessory

PRODUCT_PACKAGES += \
    lights.bowser \
    audio.primary.bowser \
    power.bowser \
    audio.a2dp.default \
    audio.usb.default \
    audio.r_submix.default \
    audio.hdmi.bowser

# Filesystem management tools
PRODUCT_PACKAGES += \
    e2fsck \
    setup_fs

PRODUCT_CHARACTERISTICS := tablet,nosdcard

# we have enough storage space to hold precise GC data
PRODUCT_TAGS += dalvik.gc.type-precise

# media config xml file
PRODUCT_COPY_FILES += \
    device/asus/grouper/media_profiles.xml:system/etc/media_profiles.xml

# media codec config xml file
PRODUCT_COPY_FILES += \
    frameworks/av/media/libstagefright/data/media_codecs_google_audio.xml:system/etc/media_codecs_google_audio.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_telephony.xml:system/etc/media_codecs_google_telephony.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_video.xml:system/etc/media_codecs_google_video.xml \
    device/amazon/tate/media_codecs.xml:system/etc/media_codecs.xml

# audio mixer paths
PRODUCT_COPY_FILES += \
    device/amazon/tate/prebuilt/etc/mixer_paths.xml:/system/etc/mixer_paths.xml

# audio policy configuration
PRODUCT_COPY_FILES += \
    device/amazon/tate/prebuilt/etc/audio_policy.conf:/system/etc/audio_policy.conf \

###########################################################################
#
# Data that is going to be sorted out later, in other words I am still
# figuring these out...            __
#                                  ||
#                                 _||_
#                                 \  /
#                                  \/

PRODUCT_COPY_FILES += \
    frameworks/av/media/libstagefright/data/media_codecs_ffmpeg.xml:system/etc/media_codecs_ffmpeg.xml \
    device/amazon/tate/prebuilt/etc/smc_normal_world_android_cfg.ini:/system/etc/smc_normal_world_android_cfg.ini \
    device/amazon/tate/prebuilt/etc/wifi/wpa_supplicant.conf:system/etc/wifi/wpa_supplicant.conf \
    device/amazon/tate/prebuilt/vendor/etc/audio_effects.conf:/system/vendor/etc/audio_effects.conf \
    device/amazon/tate/bluetooth/bt_vendor.conf:system/etc/bluetooth/bt_vendor.conf 


ADDITIONAL_BUILD_PROPERTIES += \
    wifi.interface=wlan0 \
    wifi.supplicant_scan_interval=120 \
    ro.opengles.version=131072 \
    com.ti.omap_enhancement=true \
    omap.enhancement=true \
    ro.crypto.state=unencrypted \
    persist.lab126.chargeprotect=1 \
    persist.sys.usb.config=mtp,adb \
    persist.sys.root_access=3 \
    ro.bq.gpu_to_cpu_unsupported=1 \
    media.stagefright.cache-params=18432/20480/15 \
    ro.ksm.default=1 \
    camera2.portability.force_api=1

ADDITIONAL_BUILD_PROPERTIES += \
    persist.demo.hdmirotationlock=true \
    persist.lab126.touchnoisereject=1 \
    ro.nf.profile=2 \
    ro.nf.level=512 \
    omap.audio.power=PingPong \
    dolby.audio.sink.info=speaker \
    ro.camera.sound.forced=0 \
    ro.camera.video_size=1280x720



# Audio Support
PRODUCT_PACKAGES += \
    libaudioutils \
    Music \
    tinyplay \
    tinymix \
    tinycap \
    audio_policy.default \

# DRM
PRODUCT_PACKAGES += \
    libwvm \

# Misc / Testing
PRODUCT_PACKAGES += \
    evtest \
    strace \
    libjni_pinyinime \
    sh

PRODUCT_PACKAGES += \    
    camera.omap4

PRODUCT_COPY_FILES += \
    device/amazon/tate/default.prop:/root/default.prop \


PRODUCT_PACKAGES += \
    uim-sysfs \
    libbt-vendor

PRODUCT_PACKAGES += \
    sensors.omap4 \
    libinvensense_hal \
    libmllite \
    libmplmpu \






