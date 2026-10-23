# vendor_bearkey

## Introduction

This repository is used to store HDF configuration information, product definition information for the corresponding development board (product form) of Bearkey BQ3588HM.

## Directory

```text
/vendor/bearkey/bq3588
.
├── bluetooth               # Bluetooth drivers and firmware
├── config.json             # Product feature configuration
├── default_app_config      # Default app configuration
├── etc                     # Hardware, product parameter configuration
├── hals
│   ├── audio               # Audio ALSA solution parameter configuration
├── hdf_config              # HDF driver hcs file configuration for audio, camera, device_info, sensor, etc.
├── image_conf              # Ramdisk, system, updater partition size parameter configuration
├── preinstall-config       # Pre-installed app JSON configuration file
├── product.gni             # Product configuration gni
├── resourceschedule        # Cgroup related JSON configuration files
├── security_config         # Security related JSON configuration files
├── sepolicy                # SELinux permission configuration file
├── updater_config          # Updater configuration file
└── window_config           # Window configuration XML file
```

## Usage

[BQ3588 Development Board User Guide](https://gitcode.com/openharmony-sig/device_board_bearkey/blob/master/README_EN.md)

## License

See the LICENSE file in this directory.

## Related Repositories

[device_board_bearkey](https://gitcode.com/openharmony-sig/device_board_bearkey)

[device_soc_rockchip](https://gitcode.com/openharmony/device_soc_rockchip)
