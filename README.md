# Philips LivingColors 1st generation component for ESPHome.

[![GitHub Release][releases-shield]][releases]
[![Licence][license-shield]][license]
[![Maintainer][maintainer-shield]][maintainer]  
[![Github Sponsors][github-shield]][github]
[![PayPal][paypal-shield]][paypal]
[![BuyMeCoffee][buymecoffee-shield]][buymecoffee]
[![Patreon][patreon-shield]][patreon]

## Introduction

To set up this LivingColors component you first need to place a top-level SPI and CC2500 component
which defines the pins to use.

## Hardware required

- ESP8266 or ESP32
- CC2500 transceiver

These need to be wired as described on the [ESPHome CC2500 component](https://github.com/rrooggiieerr/esphome-cc2500)

![](wiring3.jpg)

## ESPHome example configuration:

```
esphome:
  name: livingcolors1

external_components:
  - source:
      type: git
      url: https://github.com/rrooggiieerr/esphome-cc2500
      # ref: 0.0.2
    components: [cc2500]
  - source:
      type: git
      url: https://github.com/rrooggiieerr/esphome-livingcolors1
      # ref: 0.0.2
    components: [livingcolors1]

esp8266:
  board: nodemcu

# Enable logging
logger:
  level: DEBUG

spi:
  clk_pin: GPIO14
  mosi_pin: GPIO13
  miso_pin: GPIO12

cc2500:
  cs_pin: GPIO15
  gdo2_pin: GPIO5
  output_power: 0xFF

livingcolors1:

light:
  - platform: livingcolors1
    name: "LivingColors"
    address: 0x...
    restore_mode: RESTORE_DEFAULT_OFF
```

## Detecting the Living Colors address

Enable info loglevel in your ESPHome configuration and install, when a new Living Colors address is
detected the address will be printed to the ESPHome console

## Credits

[Phil](https://github.com/philipplive)'s [Philips Living Colors (Gen 1) mit NodeMCU (ESP8266) + CC2500 steuern](https://github.com/philipplive/Philips-Living-Colors-Gen1-with-NodeMCU) was helpful for making this component.

## Support my work

Do you enjoy using this ESPHome component? Then consider supporting my work using one of the
following platforms, your donation is greatly appreciated and keeps me motivated:

[![Github Sponsors][github-shield]][github]
[![PayPal][paypal-shield]][paypal]
[![BuyMeCoffee][buymecoffee-shield]][buymecoffee]
[![Patreon][patreon-shield]][patreon]

## Hire me

If you're in need for a freelance ESP developer for your project please contact me, you can find my
email address on [my GitHub profile](https://github.com/rrooggiieerr).

[releases]: https://github.com/rrooggiieerr/esphome-livingcolors1/releases
[releases-shield]: https://img.shields.io/github/v/release/rrooggiieerr/esphome-livingcolors1?style=for-the-badge
[license]: ./LICENSE
[license-shield]: https://img.shields.io/github/license/rrooggiieerr/esphome-livingcolors1?style=for-the-badge
[maintainer]: https://github.com/rrooggiieerr
[maintainer-shield]: https://img.shields.io/badge/MAINTAINER-%40rrooggiieerr-41BDF5?style=for-the-badge
[paypal]: https://paypal.me/seekingtheedge
[paypal-shield]: https://img.shields.io/badge/PayPal-00457C?style=for-the-badge&logo=paypal&logoColor=white
[buymecoffee]: https://www.buymeacoffee.com/rrooggiieerr
[buymecoffee-shield]: https://img.shields.io/badge/Buy%20Me%20a%20Coffee-ffdd00?style=for-the-badge&logo=buy-me-a-coffee&logoColor=black
[github]: https://github.com/sponsors/rrooggiieerr
[github-shield]: https://img.shields.io/badge/sponsor-30363D?style=for-the-badge&logo=GitHub-Sponsors&logoColor=ea4aaa
[patreon]: https://www.patreon.com/seekingtheedge/creators
[patreon-shield]: https://img.shields.io/badge/Patreon-F96854?style=for-the-badge&logo=patreon&logoColor=white