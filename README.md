# nRF52 FIDO U2F Security Key

> An Open-Source FIDO U2F implementation on nRF52 SoC

## Description

nRF52-U2F is an open-source FIDO U2F implementation on nRF52 SoC. Taking advantage of Nordic’s powerful SoC [nRF52840](https://www.nordicsemi.com/eng/Products/nRF52840) and [nRF5 SDK](https://www.nordicsemi.com/eng/Products/Bluetooth-low-energy/nRF5-SDK), it's quite easy to make a FIDO U2F key with a number of distinguishing features, such as USB HID class modules, comprehensive cryptography library with ARM® TrustZone® Cryptocell-310, reliable Device Firmware Update (DFU), etc.

The FIDO Universal 2nd Factor (U2F) is an open authentication standard that allows online services to augment the security of their existing password infrastructure by adding a strong second factor to user login. 

During registration and authentication, the user presents the second factor by simply pressing a button on a FIDO U2F key. The user can use their FIDO U2F key across all online services that support the protocol leveraging built-in support in web browsers.

FIDO U2F has been successfully deployed by large scale online services, including [Google](https://wiki.makerdiary.com/nrf52-u2f/guides/using-u2f-with-google), [Facebook](https://wiki.makerdiary.com/nrf52-u2f/guides/using-u2f-with-facebook), [Twitter](https://wiki.makerdiary.com/nrf52-u2f/guides/using-u2f-with-twitter), [GitHub](https://wiki.makerdiary.com/nrf52-u2f/guides/using-u2f-with-github), [GitLab](https://wiki.makerdiary.com/nrf52-u2f/guides/using-u2f-with-gitlab), and many more.

![](docs/images/fido-u2f-key-usage.png)

The FIDO U2F technical specifications are hosted by the open-authentication industry consortium known as the [FIDO Alliance](http://fidoalliance.org/). Learn more about U2F on https://fidoalliance.org/.

## Features

* Support Nordic nRF52840 System-on-Chip
	- ARM® Cortex®-M4F processor optimized for ultra-low power operation
	- Combining *Bluetooth 5*, *Bluetooth Mesh*, *Thread*, *IEEE 802.15.4*, *ANT* and *2.4GHz*
	- On-chip NFC-A tag
	- On-chip USB 2.0 (Full speed) controller
	- ARM TrustZone® Cryptocell-310 security subsystem
	- 1 MB FLASH and 256 KB RAM
* Standard FIDO U2F Protocol supported
* Driver-less installation on all major host platforms
* Multi-application support with concurrent application access without the need for serialization and centralized dispatching.
* Comprehensive cryptography library with ARM® TrustZone® Cryptocell-310
* Reliable Device Firmware Update (DFU)

## How it works?

The following diagram explains the basic process flow of FIDO U2F:

![](docs/images/how-u2f-works.png)

## Developers Wiki

We have provided developers wiki to make it a pleasure to develop with nRF52-U2F. 
Get what you need here or visit [https://wiki.makerdiary.com/nrf52-u2f](https://wiki.makerdiary.com/nrf52-u2f).

* [Getting Started with nRF52-U2F](https://wiki.makerdiary.com/nrf52-u2f/getting-started)
* [How to upgrade the nRF52-U2F Firmware?](https://wiki.makerdiary.com/nrf52-u2f/upgrading)
* [How to build the nRF52-U2F Firmware?](https://wiki.makerdiary.com/nrf52-u2f/building)
* [nRF52-U2F Users Guide](https://wiki.makerdiary.com/nrf52-u2f/guides/using-u2f-with-google/)

## Supported Boards

The following developmet boards are supported well and it's easy to port to other nRF52840 boards. More boards are planned and will show up gradually over time.

* [nRF52840-MDK](https://wiki.makerdiary.com/nrf52840-mdk) ---- [Get One!](https://store.makerdiary.com/collections/frontpage/products/nrf52840-mdk-iot-development-kit)

* [nRF52840 Micro Dev Kit USB Dongle](https://wiki.makerdiary.com/nrf52840-mdk-usb-dongle) ---- [Get One!](https://store.makerdiary.com/collections/frontpage/products/nrf52840-mdk-usb-dongle)


## Supported Services

There are a list of featured services that use FIDO U2F. More services will deploy U2F and will show up here over time.

* [Using nRF52-U2F with Google](https://wiki.makerdiary.com/nrf52-u2f/guides/using-u2f-with-google)
* [Using nRF52-U2F with Facebook](https://wiki.makerdiary.com/nrf52-u2f/guides/using-u2f-with-facebook)
* [Using nRF52-U2F with Twitter](https://wiki.makerdiary.com/nrf52-u2f/guides/using-u2f-with-twitter)
* [Using nRF52-U2F with GitHub](https://wiki.makerdiary.com/nrf52-u2f/guides/using-u2f-with-github)
* [Using nRF52-U2F with GitLab](https://wiki.makerdiary.com/nrf52-u2f/guides/using-u2f-with-gitlab)

## MIT License

Copyright (c) 2018 [makerdiary](https://makerdiary.com)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.