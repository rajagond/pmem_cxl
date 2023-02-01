# pmem_cxl
Autumn 2022 - Spring 2023

##### How to emulate CXL using qemu and kvm?

###### References

[Emulate cxl using qemu and kvm](https://stevescargall.com/2022/01/20/how-to-emulate-cxl-devices-using-kvm-and-qemu/)

###### Requirements

- Host OS - Ubuntu 22.04
- Guest OS - Ubuntu 22.04
- Qemu - v2.0v4
- Virtualization is enabled in the BIOS
  `ls lscpu | grep
   Virtualization Virtualization:                  VT-x`

- ```bash
  sudo apt install linux-headers-$(uname -r) build-essential
  sudo apt install git gcc g++ autoconf automake asciidoc asciidoctor xmlto libtool pkg-config libglib2.0-dev
  sudo apt install libfabric-dev libfabric-dev doxygen graphviz pandoc libkmod-dev kmod
  sudo apt-get install libncurses5-dev libncursesw5-dev
  sudo apt-get install -y libudev-dev uuid-dev libjson-c-dev libiniparser-dev libkeyutils-dev
  sudo apt-get install -y bash-completion ninja-build sparse libpixman-1-dev
  sudo apt install qemu-kvm virt-manager virtinst libvirt-clients bridge-utils libvirt-daemon-system cloud-init genisoimage
  sudo apt-get install qemu-utils
  ```

###### Install Qemu

```shell
mkdir ~/cxl ; cd ~/cxl
git clone https://gitlab.com/bwidawsk/qemu
cd qemu
git branch 
* cxl-2.0v4
mkdir build ; cd build
../configure --prefix=/opt/qemu-cxl
# Since this is a development branch of QEMU, I want to install it under /opt/qemu-cxl so it doesn’t interfere with any existing or future QEMU installation. Alternatively, don’t run ‘make install’ and source the binaries and libraries from the build directory.
make -j all
make install
```

###### Configure the Host Networking

Confirm IP forwarding is enabled for IPv4 and/or IPv6 on the host (0=Disabled, 1=Enabled):

`$ sudo cat /proc/sys/net/ipv4/ip_forward`

`1`

`$ sudo cat /proc/sys/net/ipv6/conf/default/forwarding`

1

If necessary, activate forwarding temporarily until the next reboot:

`$ sudo echo 1 > /proc/sys/net/ipv4/ip_forward`

`$ sudo echo 1 > /proc/sys/net/ipv6/conf/all/forwarding`

For a permanent setup create the following file:

`$ sudo vim /etc/sysctl.d/50-enable-forwarding.conf`

\# local customizations

\#

\# enable forwarding for dual stack

net.ipv4.ip_forwarding=1
net.ipv6.conf.all.forwarding=1

###### Download a Guest OS using cloud images

```shell
# Confirm the OS base directory exists
ls -ld /var/lib/libvirt/boot

#! /bin/bash

# Download Ubuntu 22.04 cloud image
FILE=/var/lib/libvirt/boot/jammy-server-cloudimg-amd64.qcow2
if [[ ! -f "$FILE" ]]; then
    sudo wget https://cloud-images.ubuntu.com/jammy/current/jammy-server-cloudimg-amd64.img -O /var/lib/libvirt/boot/jammy-server-cloudimg-amd64.qcow2
fi

# Create a new disk image from the cloud image for our new guest VM called “CXL-Test”:

sudo qemu-img convert \
  -f qcow2 \
  -O qcow2 \
  /var/lib/libvirt/boot/jammy-server-cloudimg-amd64.qcow2 \
  /var/lib/libvirt/images/CXL-ubuntu.qcow2

# Grow the disk image by 20GiB
sudo qemu-img resize /var/lib/libvirt/images/CXL-ubuntu.qcow2 +20G

# Review the image information 
sudo qemu-img info /var/lib/libvirt/images/CXL-ubuntu.qcow2

sudo virt-install --connect qemu:///system \
--name CXL-ubuntu --memory 4096 --cpu host --vcpus 4 \
--os-type linux --os-variant ubuntu22.04 \
--import --graphics none \
--disk /var/lib/libvirt/images/CXL-ubuntu.qcow2,format=qcow2,bus=virtio \
--network network=default \
--network bridge=virbr0,model=virtio \
--cloud-init root-password-generate=yes,disable=yes

# The temporary root password is displayed in the first few seconds. You need to make a note of this one-time password! It should be similar to the following (where * is your actual password):

# Starting install…
# Password for first root login is: ****************
# Installation will continue in 10 seconds (press Enter to skip)…

# -- Now, update the Kernel for CXL support after changing password of root user --
uname -r # current kernel version
sudo apt update && sudo apt upgrade # update and upgrade
sudo apt install wget # install wget
wget https://raw.githubusercontent.com/pimlie/ubuntu-mainline-kernel.sh/master/ubuntu-mainline-kernel.sh # script
chmod +x ubuntu-mainline-kernel.sh # make it executable
sudo mv ubuntu-mainline-kernel.sh /usr/local/bin/ # move it /usr/local/bin/
ubuntu-mainline-kernel.sh -c # to find latest available version to install
ubuntu-mainline-kernel.sh -r # list all available version
sudo ubuntu-mainline-kernel.sh -i # install latest available version
sudo reboot
uname -r # this time you will see latest version
# Now, install cxl-cli, ndctl and daxctl package on guest OS
sudo apt install libcxl-dev ndctl daxctl libndctl-dev libdaxctl-dev libpmem-dev libcxl1 libdaxctl1
sudo apt update && sudo apt upgrade # update and upgrade
poweroff
```

###### Configure CXL

The following will configured two cxl devices in the guest

```shell
#! /bin/bash

sudo /opt/qemu-cxl/bin/qemu-system-x86_64 -drive file=/var/lib/libvirt/images/CXL-ubuntu.qcow2,format=qcow2,index=0,media=disk,id=hd \
-m 4G,slots=8,maxmem=8G \
-smp 4 \
-machine type=q35,accel=kvm,nvdimm=on,cxl=on \
-enable-kvm \
-nographic \
-net nic,model=e1000 \
-net user,hostfwd=tcp::2222-:22 \
-object memory-backend-ram,size=4G,id=mem0 \
-numa node,nodeid=0,cpus=0-3,memdev=mem0 \
-object memory-backend-file,id=cxl-mem1,share=on,mem-path=cxl-window1,size=512M \
-object memory-backend-file,id=cxl-label1,share=on,mem-path=cxl-label1,size=1K \
-object memory-backend-file,id=cxl-label2,share=on,mem-path=cxl-label2,size=1K \
-device pxb-cxl,id=cxl.1,bus=pcie.0,bus_nr=52,uid=0,len-window-base=1,window-base[0]=0x4c00000000,memdev[0]=cxl-mem1 \
-device cxl-rp,id=rp0,bus=cxl.1,addr=0.0,chassis=0,slot=0,port=0 \
-device cxl-rp,id=rp1,bus=cxl.1,addr=1.0,chassis=0,slot=1,port=1 \
-device cxl-type3,bus=rp0,memdev=cxl-mem1,id=cxl-pmem0,size=256M,lsa=cxl-label1 \
-device cxl-type3,bus=rp1,memdev=cxl-mem1,id=cxl-pmem1,size=256M,lsa=cxl-label2

ls -1 /dev/cxl
# output
# mem0
# mem1
cxl list -M
# [
#  {
#    "memdev":"mem0",
#    "pmem_size":268435456,
#    "ram_size":0
#  },
#  {
#    "memdev":"mem1",
#    "pmem_size":268435456,
#    "ram_size":0
#  }
#]
```


