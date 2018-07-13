# device-model

This application emulates various devices on CHERI platform.

CHERI CPU used by this application is synthesized on Altera FPGA and prototyped on Altera DE4 board.

This is bare-metal software, i.e. it runs on a dedicated CPU core of CHERI processor.

### Build under FreeBSD

    $ sudo pkg install llvm-cheri
    $ git clone --recursive https://github.com/CTSRD-CHERI/device-model
    $ cd device-model
    $ make

![alt text](https://raw.githubusercontent.com/CTSRD-CHERI/device-model/master/images/de4.jpg)
