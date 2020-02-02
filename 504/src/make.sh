#!/bin/bash
#clang -target bpf , see https://www.kernel.org/doc/html/latest/bpf/bpf_devel_QA.html#q-clang-flag-for-target-bpf
# also : https://stackoverflow.com/questions/56975861/error-compiling-ebpf-c-code-out-of-kernel-tree

C_FLAGS="-Wno-unused-value -Wno-pointer-sign -Wno-compare-distinct-pointer-types -Wno-gnu-variable-sized-type-not-at-end -Wno-address-of-packed-member -Wno-tautological-compare -Wno-unknown-warning-option"

function magic_make {
DEFAULT=$1
KERN="$DEFAULT"_kern
USER="$DEFAULT"_user

clang -nostdinc -isystem `clang -print-file-name=include` \
	-D__KERNEL__ -D__ASM_SYSREG_H -D__TARGET_ARCH_x86 \
	$C_FLAGS \
	-I../common/ \
	-include /usr/src/linux-headers-`uname -r`/include/linux/kconfig.h \
	-I/usr/src/linux-headers-`uname -r`/include/ \
	-I/usr/src/linux-headers-`uname -r`/include/uapi/ \
	-I/usr/src/linux-headers-`uname -r`/include/generated/uapi/ \
	-I/usr/src/linux-headers-`uname -r`/arch/x86/include \
	-I/usr/src/linux-headers-`uname -r`/arch/x86/include/uapi/ \
	-I/usr/src/linux-headers-`uname -r`/arch/x86/include/generated/ \
	-I/usr/src/linux-headers-`uname -r`/arch/x86/include/generated/uapi/ \
    -I/usr/src/linux-headers-`uname -r`/tools/lib/ \
    -include asm_goto_workaround.h \
	-O2 -emit-llvm -c "$KERN".c -o -| llc -march=bpf -filetype=obj -o "compiled/$KERN".o

gcc "$USER".c ../common/bpf_load.c ~/thesis/libbpf/src/libbpf.a -I../tools/include/ -I../tools/perf/ -I../common/ -I/home/giogge/thesis/libbpf/include/uapi/ -I/home/giogge/thesis/libbpf/src/ -lelf -o compiled/$DEFAULT

}

targets=( hellotrace tracex1 cpustat open_tp passfd readbuff buffermap readiter)

for t in "${targets[@]}" ; do
	echo "making ...$t"
	magic_make $t
done
