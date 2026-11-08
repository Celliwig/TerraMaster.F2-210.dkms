#!/bin/bash

################################################################################
#
# Script to make a supplemental initrd image containing the necessary drivers
# built against a specific kernel version
#
################################################################################

DIR_REPO=`pwd`

DIR_BUILD="${DIR_REPO}/build.d"
DIR_BUILD_DTBS="/boot/dtbs/"
DIR_BUILD_MODS="/usr/lib/modules/"
DIR_DTS="${DIR_REPO}/dts"
DIR_SCRIPTS="${DIR_REPO}/scripts.d"
KERNEL_VERSION="${1}"
KERNEL_HEADERS="/usr/src/linux-headers-${KERNEL_VERSION}"

# Do checks
################################
# Check that the script is running from the repo root
if [ ! -f "${DIR_SCRIPTS}/mkinitrd.sh" ]; then
	echo "Error: script must be run from repo root!" >&2
	exit -1
fi

# Check that the required kernel header package has been installed
if [ ! -d "${KERNEL_HEADERS}" ]; then
	echo "Error: kernel headers not available, please install the necessary package!" >&2
	echo "	- apt-get install linux-headers-${KERNEL_VERSION}" >&2
	exit -1
fi

# Make the kernel modules
################################
# Detect number of CPUs
num_cpus=`grep -E '^processor\s*:\s*' /proc/cpuinfo |wc -l`

KDIR="${KERNEL_HEADERS}" make clean &>/dev/null
if [ ${?} -ne 0 ]; then
	echo "Error: failed to run: make clean!" >&2
	exit -1
fi
KDIR="${KERNEL_HEADERS}" make -j${num_cpus} &>/dev/null
if [ ${?} -ne 0 ]; then
	echo "Error: failed to run: make!" >&2
	exit -1
fi

# Make DTBS
################################
cd "${DIR_DTS}" &>/dev/null && make dtbs &>/dev/null
if [ ${?} -ne 0 ]; then
	echo "Error: failed to run: make dtbs!" >&2
	exit -1
fi
cd "${DIR_REPO}"


# Create initrd
################################
# Check build directory exists
if [ ! -d "${DIR_BUILD}" ]; then
	echo "Error: build directory missing: ${DIR_BUILD}!" >&2
	exit -1
fi

# Create temp directory
dir_temp=`mktemp -p "${DIR_BUILD}" -d 2>/dev/null`
if [ ${?} -ne 0 ]; then
	echo "Error: failed to create temporary directory!" >&2
	exit -1
fi

# Create necessary directories
dir_temp_dtbs="${dir_temp}/${DIR_BUILD_DTBS}"
mkdir -p "${dir_temp_dtbs}" &>/dev/null
if [ ${?} -ne 0 ]; then
	echo "Error: failed to create temporary DTBS directory: ${dir_temp_dtbs}" >&2
	exit -1
fi
dir_temp_mods="${dir_temp}/${DIR_BUILD_MODS}/${KERNEL_VERSION}/extras"
mkdir -p "${dir_temp_mods}" &>/dev/null
if [ ${?} -ne 0 ]; then
	echo "Error: failed to create temporary modules directory: ${dir_temp_mods}" >&2
	exit -1
fi

# Copy files
cp "${DIR_DTS}/"*.dtb "${dir_temp_dtbs}" &>/dev/null
if [ ${?} -ne 0 ]; then
	echo "Error: failed to copy DTBs!" >&2
	exit -1
fi
num_modules=0
for tmp_file in `find "${DIR_REPO}" -name '*.ko'`; do
	#echo "Copying ${tmp_file} -> ${dir_temp_mods}"
	cp "${tmp_file}" "${dir_temp_mods}" &>/dev/null
	if [ ${?} -ne 0 ]; then
		echo "Error: failed to copy module: ${tmp_file} -> ${dir_temp_mods}" >&2
		exit -1
	fi
	num_modules=$((${num_modules}+1))
done
if [ ${num_modules} -eq 0 ]; then
	echo "Error: failed to copy modules, none found!" >&2
	exit -1
fi

# Create initrd
cd "${dir_temp}" &>/dev/null
if [ ${num_modules} -eq 0 ]; then
	echo "Error: failed to change directory!" >&2
	exit -1
fi
find . -print0 2>/dev/null | cpio --null -o -H newc 2>/dev/null | gzip >"../initrd-rtd1296.img-${KERNEL_VERSION}" 2>/dev/null
if [ ${num_modules} -eq 0 ]; then
	echo "Error: creating initrd image!" >&2
	exit -1
fi
cd "${DIR_REPO}" &>/dev/null


# Clean-up
################################
rm -rf "${dir_temp}" &>/dev/null
if [ ${?} -ne 0 ]; then
	echo "Error: failed to remove temporary directory: ${dir_temp}" >&2
	exit -1
fi
