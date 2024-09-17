# Thingsboard Asset Tracker Sample

An example for a minimal asset tracking device that connects to
[Thingsboard](https://thingsboard.io/) over LTE-M and NB-IoT and sends its GPS position as telemetry
data using CoAP.

This code was created for the talk "How to create an asset tracker with Zephyr and Thingsboard in no time"
presented at the Open Source Summit Europe 2024.
Find the corresponding slides [here](<doc/Open Source Summit Europe 2024 Talk.pdf>).

## Overview

This project is made to run on a [Nordic nRF9160 Development Kit](https://www.nordicsemi.com/Products/Development-hardware/nRF9160-DK)
which contains a modem for LTE-M/NB-IoT and GNSS. The included GPS antenna must be
connected to the board. The code depends on the nRF Connect SDK v2.7.0 for its modem and
LTE link control libraries.

On startup the application establishes a cellular data connection and starts the GPS receiver.
Afterwards it opens a BSD socket for IP communication with a Thingsboard instance and
periodically sends GPS position information (latitude, longitude and timestamp) via CoAP
to Thingsboards's telemetry endpoint.

## Thingsboard Setup

Install your own Thingsboard instance as described in the
[Getting Started Guide for the Thingsboard Community Edition](https://thingsboard.io/docs/getting-started-guides/helloworld)
or use a hosted instance at [thingsboard.cloud](https://thingsboard.cloud).

Login and create a new device under `Entities -> Devices`. Give it a name, use the default profile
for now and click `Add`. Click on the new device in the table and then `Manage Credentials` to
view the access token for the device. You will need this later when configuring the firmware build
for the device.

## Device Firmware

### Prerequisites

First install Zephyr according to the
[Getting Started Guide](https://docs.zephyrproject.org/3.7.0/develop/getting_started/index.html)
but replace the command `west init ~/zephyrproject` with the following to install the nRF
Connect SDK which comes with its own copy of zephyr:

```bash
west init -m https://github.com/nrfconnect/sdk-nrf --mr v2.7.0 ~/zephyrproject
```

Additionally install python requirements of the nRC Connect SDk into the python virtual environment:

```bash
pip3 install -r nrf/scripts/requirements.txt
```

### Compiling

Checkout the project and initialize the workspace:

```bash
west init -m https://github.com/GCX-EMBEDDED/ossummit-asset-tracker asset-tracker
cd asset-tracker
west update
```

Build the application with

```bash
west build -b nrf9160dk/nrf9160/ns application/app
```

Connect the nRF9160 DK via USB and program the application with:

```bash
west flash --erase
```

### Configuration

In order to authenticate to Thingsboard the device uses its access token which you need
set with the option `CONFIG_TBAT_THINGSBOARD_ACCESS_TOKEN`. The `app/Kconfig` file defines more
project specific configuration options that you can modify if needed.

If you would like to use a debugger you can enable debug optimzation by adding the `debug.conf`
file to the configuration:

```bash
west build -b nrf9160dk/nrf9160/ns application/app -- -DEXTRA_CONF_FILE=debug.conf
```

In order to use the [nRF Connect Cellular Monitor](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fug_cellular_monitor%2FUG%2Fcellular_monitor%2Fintro.html)
to capture network traffic over the modem use this build command:

```bash
west build -b nrf9160dk/nrf9160/ns -S nrf91-modem-trace-uart application/app -- -DEXTRA_CONF_FILE=trace.conf
```
