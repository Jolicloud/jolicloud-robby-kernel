build_arch	= i386
header_arch	= x86_64
asm_link	= x86
defconfig	= defconfig
flavours        = jolicloud jolicloud-atom
build_image	= bzImage
kernel_file	= arch/$(build_arch)/boot/bzImage
install_file	= vmlinuz

server_sub	= virtual

loader		= grub

skipdbg		= true
skipabi		= true
