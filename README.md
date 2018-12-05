# device-model

This application emulates various devices on CHERI platform.

CHERI CPU used by this application is synthesized on Altera FPGA and prototyped on Altera DE4 board.

This is bare-metal software, i.e. it runs on a dedicated CPU core of CHERI processor.

### Build FreeBSD with a beri_manager driver and bm (BERI Manager) application

    $ git clone -b dma https://github.com/bukinr/freebsd-head
    $ make -j4 TARGET=mips TARGET_ARCH=mips64 buildworld
    $ make -j4 TARGET=mips TARGET_ARCH=mips64 KERNCONF=BERI_DE4_USBROOT buildkernel

### Build device-model under FreeBSD

    $ sudo pkg install llvm-cheri
    $ git clone --recursive https://github.com/CTSRD-CHERI/device-model
    $ cd device-model
    $ make

### Boot FreeBSD kernel on a 1st core of BERI system

    $ cd ctsrd/cherilibs/trunk/tools/debug
    $ ./berictl -j loadsof -z DE4_BERI_EPW.sof.bz2
    $ ./berictl -j loadbin -z kernel.bz2 0x100000
    $ ./berictl -j boot && ./berictl -j console

### Receive device-model on a FreeBSD that runs on a 1st core

    $ ifconfig atse0 10.10.0.2/24
    $ /etc/rc.d/var start
    $ nc -l 1234 > /var/tmp/dm.bin

### On your host (development) machine, send binary

    $ nc 10.10.0.2 1234 < dm.bin

### Program device-model to DDR memory from a FreeBSD that runs on a 1st core. Start execution DM.

    $ bm -rRl /var/tmp/dm.bin

### Open a terminal to the device-model on your host (development) machine

    $ nios2-terminal -i 2

### Re-attach device drivers and enable peripheral window (register interface).

    $ devctl disable atse1
    $ devctl disable msgdma2
    $ devctl disable msgdma3
    $ sleep 1
    $ devctl enable msgdma2
    $ devctl enable msgdma3
    $ devctl enable atse1

    $ ifconfig atse1 10.4.0.2/24

### Example device-tree node for PCI-e device
```
pcie@7fb10000 {
	compatible = "pci-host-ecam-generic";
	device_type = "pci";
	#interrupt-cells = <1>;
	#address-cells = <3>;
	#size-cells = <2>;

	reg = <0x7fb10000 0xe0000>;
	reg-names = "PCI ECAM";

	//attributes      pci_addr   cpu_addr         size
	ranges =
	  <0x02000000 0 0x40000000 0x7fb20000 0 0x00020000	//e1000 bar 0
	   0x01000000 0 0x00000000 0x00000010 0 0x00010000	//e1000 bar 2
	   0x43000000 0 0x60000000 0x7fb40000 0 0x00010000	//e1000 bar 1
	   0x02000000 0 0x80000000 0x7fb50000 0 0x00010000>;	//ahci-hd bar 0

	// PCI_DEVICE(3)  INT#(1)  CONTROLLER(PHANDLE)  CONTROLLER_DATA(1)

	interrupt-map = < 0x000 0 0  1  &beripic0  0x13 	//e1000
			  0x800 0 0  1  &beripic0  0x14 >; 	//ahci
	interrupt-map-mask = <0x800 0 0 7>;
};
```

### Physical memory organization

| Start      | End        | Description                          |
| ---------- | ---------- | ------------------------------------ |
| 0x00000000 | 0x40000000 | Entire physical memory of SoC        |
| 0x10000000 | 0x20000000 | Reserved by FreeBSD for device-model |
| 0x10000000 | 0x10800000 | device-model static data             |
| 0x10800000 | 0x11000000 | device-model malloc()                |
| 0x11000000 | 0x20000000 | AHCI SATA device memory backend      |

![alt text](https://raw.githubusercontent.com/CTSRD-CHERI/device-model/master/images/de4.jpg)
