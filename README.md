# TerraMaster.F2-210.dkms
TerraMaster F2-210 DKMS modules


## Usage: DKMS
To use (<pkg_version> cam be found in the file 'dkms.conf', as PACKAGE_VERSION):
* cd /usr/src
* sudo git clone https://github.com/Celliwig/TerraMaster.F2-210.dkms.git
* sudo ln -s TerraMaster.F2-210.dkms/ rtd1296-<pkg_version>
* sudo dkms add rtd1296/<pkg_version> -k $(uname -r)
* sudo dkms build rtd1296/<pkg_version> -k $(uname -r)
* sudo dkms install rtd1296/<pkg_version> -k $(uname -r)
* cd dts
* sudo make dtbs
* sudo mkdir /boot/dtbs
* sudo cp rtd-1296-tm-f2-210.dtb /boot/dtbs

Edit the file '/etc/initramfs-tools/modules', and add:
```
rtk-reset
rtk-clk-rtd129x-ctlr
rtk-clk-cgc
rtd119x_wdt
rtc-rtd119x
rtd-rng
irq-rtd129x
8250_rtk
pinctrl-rtd129x
gpio-rtd129x
rtk-sata-phy
ahci_dwc
sd_mod
dm_mod
```
Then update the initrd with:
* sudo update-initramfs -u -k $(uname -r)

The system's bootloader will also need to be updated to load the DTB, but that is bootloader specific.
For GRUB, add this to each kernel entry:
```
devicetree /dtbs/rtd-1296-tm-f2-210.dtb
```


## Usage: Manual
To manually build the modules against a specific kernel version:
* KDIR=/usr/src/linux-headers-6.12.63+deb13-arm64/ make
