# Using nRF52 U2F Security Key with GitHub

This guide describes how to use the nRF52 U2F Security Key with GitHub.

## Requirements

* Latest version of Google Chrome browser (or at least version 38) or Opera browser

* [nRF52840-MDK](https://store.makerdiary.com/collections/frontpage/products/nrf52840-mdk-iot-development-kit) or [nRF52840 Micro Dev Kit USB Dongle](https://store.makerdiary.com/collections/frontpage/products/nrf52840-mdk-usb-dongle) with the nRF52-U2F firmware

* A GitHub Account

!!! note
	If no firmware available, please follow these guides to prepare the correct firmware: [Upgrading the firmware](../upgrading/) or [Building the firmware](../building).

## Configuring two-factor authentication using FIDO U2F

1. You must have already configured 2FA via a TOTP mobile app or via SMS.

2. Download and install [Google Authenticator](https://support.google.com/accounts/answer/1066447?hl=en).

3. Ensure that you have the nRF52 U2F Security Key inserted into your computer.

4. In the upper-right corner of any page, click your profile photo, then click **Settings**.

5. In the user settings sidebar, click **Security**.

6. Next to "Security keys", click **Add**.

7. Under "Security keys", click **Register new device**.

8. Type a nickname for the security key, then click **Add**.

9. When the BLUE LED begins to blink, press the button on the key to have it authenticate against GitHub.

![](images/register-u2f-key-with-github.gif)


## Signing in using your key

Now you can sign in to your GitHub account with the security key you add before.

1. On your computer, [sign in to GitHub](https://github.com/login). Your device will detect that your account has a security key.

2. Insert your security key into the USB port.

3. When the BLUE LED begins to blink, press the button on the key.

	![](images/sign-in-to-github-with-u2f.png)


**Congratulations!** You can use your key each time you sign in to your GitHub account.

## Create an Issue

Interested in contributing to this project? Want to report a bug? Feel free to click here:

<a href="https://github.com/makerdiary/nrf52-u2f/issues/new"><button data-md-color-primary="marsala"><i class="fa fa-github"></i> Create an Issue</button></a>
