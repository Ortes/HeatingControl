# Heating control esp32

This project is the software of the [ESP32](https://www.espressif.com/en/products/hardware/esp-wroom-32/overview) put in a electrical heating. It offer a simple web interface as defined bellow, and can be control via vocal commands Google Home.
To get the heading working to a certain pourcentage it is turn on and off.
### Commands
  - url not containing '?' returns : { interval: <interval>, duration: <duration> }
  - "interval" as get parameter set the interval between 2 activation.
  - "duration" as get parameter set the duration of the activation.
  - "pourcent" as get parameter set the pourcentage of time activated scaled on 5 mins.
  - /pourcent get actual pourcentage.

### Installation

You have to install the esp-idf SDK beforehand.
Basically:
- Clone the repo recursively (https://github.com/espressif/esp-idf.git)
- Set 'IDF_PATH' env variable to the esp-idf path
- Fulfill python2 requirement.txt dependencies
- If python is not linked to python2 set 'python2' in menuconfig -> SDK tool configuration
See [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html) for more info.

Build:
```sh
$ make build
```

Flash:
```sh
$ make flash
```

Configure:
```sh
$ make menuconfig
```

Help:
```sh
$ make help
```
