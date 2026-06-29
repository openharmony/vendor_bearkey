# vendor_bearkey

## 介绍

本仓库托管Bearkey产品-贝启BQ3588HM开发套件的hdf配置信息、产品定义信息。

## 目录

```
vendor/bearkey/bq3588       # 贝启BQ3588HM开发板
├── bluetooth               #蓝牙驱动和固件
├── default_app_config      #默认app配置
├── etc                     #hardware, product参数配置
├── hals
│   ├── audio               #audio alsa方案参数配置
├── hdf_config              #audio, camera, device_info, sensor等HDF驱动hcs文件配置    
├── image_conf              #ramdisk, system, updater分区大小参数配置
├── preinstall-config       #预装app json配置文件
├── resourceschedule        #cgroup相关json配置文件
├── security_config         #安全相关jso配置文件
├── sepolicy                #selinux权限配置文件
├── updater_config          #updater配置文件
├── window_config           #window 窗口配置xml文件
├── bundle.json             #编译组件配置
├── config.json             #产品功能配置
├── ohos.build              #编译组件控制脚本
└── product.gni             #product配置gni
```

## 使用方法

[BQ3588HM开发板使用说明]([device_board_bearkey-代码预览-device_board_bearkey:基于OpenHarmony的RK3588开发板项目 - AtomGit](https://gitcode.com/openharmony-sig/device_board_bearkey/blob/master/README.md))

## 许可说明

参见本目录下LICENSE文件

## 相关仓

* [device/board/bearkey](https://gitcode.com/openharmony-sig/device_board_bearkey)
* [device/soc/rockchip](https://gitcode.com/openharmony-sig/device_soc_rockchip)

