## Installing the nRF5 SDK

Download the SDK package from [developer.nordicsemi.com](https://developer.nordicsemi.com/).

The current version we are using is `15.2.0`, it can be downloaded directly here: [nRF5_SDK_v15.2.0_9412b96.zip](https://www.nordicsemi.com/eng/nordic/download_resource/59011/94/96002302/116085)

Extract the zip file into the `nrf52-u2f/nrf_sdks/` directory. This should give you the following folder structure:

``` info
./nrf52-u2f/
├── LICENSE.md
├── README.md
├── boards
├── certs
├── docs
├── external
├── firmware
├── include
├── material
├── mkdocs.yml
├── nrf_sdks
│   ├── README.md
│   └── nRF5_SDK_15.2.0_9412b96
├── open_bootloader
├── source
└── tools
```

Config the toolchain path in `makefile.windows` or `makefile.posix` depending on platform you are using. That is, the `.posix` should be edited if your are working on either Linux or macOS. These files are located in:

``` sh
<nRF5 SDK>/components/toolchain/gcc
```

Open the file in a text editor and make sure that the `GNU_INSTALL_ROOT` variable is pointing to your GNU Arm Embedded Toolchain install directory. For example:

``` sh
GNU_INSTALL_ROOT := $(HOME)/gcc-arm-none-eabi/gcc-arm-none-eabi-6-2017-q2-update/bin/
GNU_VERSION := 6.3.1
GNU_PREFIX := arm-none-eabi
```
