# device-model

This application emulates various peripherals on CHERI platform:
1. Altera Modular Scatter-Gather DMA engine (mSGDMA)
2. ARM Generic ECAM PCI-controller
3. Intel E1000 ethernet device
4. AHCI controller and memory-backed Serial ATA device

CHERI CPU used by this application is synthesized on Altera FPGA and prototyped on Altera DE4 board.

This is bare-metal software, i.e. it runs on a dedicated CPU core of CHERI processor.

### Diagrams ###

![alt text](https://raw.githubusercontent.com/CTSRD-CHERI/device-model/master/diagrams/main.jpg)

![alt text](https://raw.githubusercontent.com/CTSRD-CHERI/device-model/master/diagrams/map.jpg)

![alt text](https://raw.githubusercontent.com/CTSRD-CHERI/device-model/master/diagrams/pci_mem.jpg)

### Build CheriBSD with a beri_manager driver and bm (BERI Manager) application

See the device-model branch of CheriBSD repository:
https://github.com/CTSRD-CHERI/cheribsd

Note: cheribuild is required to build this project.

    $ git clone -b device-model https://github.com/CTSRD-CHERI/cheribsd
    $ make -j4 TARGET=mips TARGET_ARCH=mips64 buildworld
    $ make -j4 TARGET=mips TARGET_ARCH=mips64 KERNCONF=BERI_DE4_USBROOT buildkernel

### Make rootfs image if you don't have one
    $ make -j4 TARGET=mips TARGET_ARCH=mips64 -DNO_ROOT DESTDIR=${HOME}/world-mips64 installworld
    $ make -j4 TARGET=mips TARGET_ARCH=mips64 -DNO_ROOT DESTDIR=${HOME}/world-mips64 distribution
    $ cd ${HOME}/world-mips64
    $ echo '# Altera JTAG UART' >> etc/ttys
    $ echo 'ttyj0 "/usr/libexec/getty std.115200" xterm on secure' >> etc/ttys
    $ echo 'ttyj1 "/usr/libexec/getty std.115200" xterm on secure' >> etc/ttys
    $ echo 'hostname="mips64"' >> etc/rc.conf
    $ echo 'sendmail_enable="NONE"' >> etc/rc.conf
    $ echo 'cron_enable="NO"' >> etc/rc.conf
    $ echo "/dev/da0  /       ufs     ro      1       1" > etc/fstab
    $ echo "./etc/fstab type=file uname=root gname=wheel mode=0644" >> METALOG
    $ echo "./etc/rc.conf type=file uname=root gname=wheel mode=0644" >> METALOG
    $ makefs -B big -D -f 30000 -o version=2 -s 1200m mips64.img METALOG

### Build device-model under FreeBSD

    $ sudo pkg install llvm-cheri
    $ git clone --recursive https://github.com/CTSRD-CHERI/device-model
    $ cd device-model
    $ gmake

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

This enables peripheral window (register interface).

    $ bm -rRl /var/tmp/dm.bin

### Open a terminal to the device-model on your host (development) machine

    $ nios2-terminal -i 2

### To emulate Altera mSGMDA:
    $ devctl disable atse1
    $ devctl disable msgdma2
    $ devctl disable msgdma3
    $ sleep 1
    $ devctl enable msgdma2
    $ devctl enable msgdma3
    $ devctl enable atse1
    $ ifconfig atse1 10.4.0.2/24

### To emulate PCIe bus and connected devices:
    $ devctl disable pci0
    $ devctl enable pci0

Note: both mSGMDA and e1000 PCI-e device use the same Altera FIFO, which means emulating both of them same time leads to undefined behaviour.

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
	  <0x02000000 0 0x7fb20000 0x7fb20000 0 0x00020000	//e1000 bar 0
	   0x01000000 0 0x00000010 0x00000010 0 0x00010000	//e1000 bar 2
	   0x43000000 0 0x7fb40000 0x7fb40000 0 0x00010000	//e1000 bar 1
	   0x02000000 0 0x7fb50000 0x7fb50000 0 0x00010000>;	//ahci-hd bar 0

	// PCI_DEVICE(3)  INT#(1)  CONTROLLER(PHANDLE)  CONTROLLER_DATA(1)

	interrupt-map = < 0x000 0 0  1  &beripic0  0x13 	//e1000
			  0x800 0 0  1  &beripic0  0x14 >; 	//ahci
	interrupt-map-mask = <0x800 0 0 7>;
};
```

### FreeBSD kernel configuration file
```
...
device		pci
device		em
device		ahci
...
```

### Physical memory organization

| Start      | End        | Size   | Description                          |
| ---------- | ---------- | ------ | ------------------------------------ |
| 0x00000000 | 0x40000000 |   1 GB | Entire physical memory of SoC        |
| 0x10000000 | 0x20000000 | 256 MB | Reserved by FreeBSD for device-model |
| 0x10000000 | 0x10800000 |   8 MB | device-model static data             |
| 0x10800000 | 0x11000000 |   8 MB | device-model malloc()                |
| 0x11000000 | 0x13000000 |  32 MB | free space                           |
| 0x13000000 | 0x20000000 | 208 MB | AHCI SATA device memory block        |

### IOMMU

IOMMU module translates addresses for various device-model peripherals and manages BERI TLB.

Only Altera FIFO-based models are currently evaluated.

To enable IOMMU module build device-model project with DM_IOMMU=1 environment variable.

On the FreeBSD side, add the following IOMMU devices to your DTS file:

```
               va_space0: iommu-va@7fb60000 {
                       compatible = "beri,iommu-va-space";
                       reg = <0x0 0x20000000>;
               };

               va_space1: iommu-va@7fb60100 {
                       compatible = "beri,iommu-va-space";
                       reg = <0x20000000 0x20000000>;
               };

               iommu0: iommu@7fb60000 {
                       compatible = "beri,iommu";
                       reg = <0x7fb60000 0x100>;
                       va-region = <&va_space0>;
                       status = "okay";
               };

               iommu1: iommu@7fb60100 {
                       compatible = "beri,iommu";
                       reg = <0x7fb60100 0x100>;
                       va-region = <&va_space1>;
                       status = "okay";
               };
```

Add the "xdma,iommu" property to the nodes of the models, for example

```
               dm_msgdma0: msgdma@7fb04080 {
                       xdma,iommu = <&iommu0>;
               };

               dm_msgdma1: msgdma@7fb04000 { 
                       xdma,iommu = <&iommu1>;
               };
```

![alt text](https://raw.githubusercontent.com/CTSRD-CHERI/device-model/master/images/de4.jpg)
