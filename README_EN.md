# vendor_bearkey

## 介绍

This repository is used to store HDF configuration information, product definition information for the corresponding development board (product form) of BQ3588HM.

## 目录

```
vendor/bearkey/bq3588       # Bearkey BQ3588HM
├── bluetooth               # bluetooth drivers and firmware
├── default_app_config      # Default app configuration
├── etc                     # Hardware, product parameter configuration
├── hals
│   ├── audio               # Audio ALSA solution parameter configuration
├── hdf_config              # HDF driver hcs file configuration for audio, camera, device_info, sensor, etc.      
├── image_conf              # Ramdisk, system, updater partition size parameter configuration
├── modules                 # Kernel modules build script
├── preinstall-config       # Pre-installed app JSON configuration file
├── resourceschedule        # Cgroup related JSON configuration files
├── security_config         # Security related JSON configuration files
├── updater_config          # Updater configuration file
├── window_config           # Window configuration XML file
├── bundle.json             # Build component configuration
├── config.json             # Product feature configuration
├── ohos.build              # Build component control script
└── product.gni             # Product configuration gni
```

## Usage

[BQ3588 Development Board User Guide]([device_board_bearkey/README.md-代码预览-device_board_bearkey:基于OpenHarmony的RK3588开发板项目 - AtomGit](https://gitcode.com/openharmony-sig/device_board_bearkey/blob/master/README_EN.md))

## License

See the LICENSE file in this directory.

## 相关仓

* [device/board/bearkey](https://gitcode.com/openharmony-sig/device_board_bearkey)
* [device/soc/rockchip](https://gitcode.com/openharmony-sig/device_soc_rockchip)

