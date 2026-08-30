#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-only

set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
OUT_DIR=${OUT_DIR:-"${ROOT_DIR}/out"}
DEFCONFIG=${DEFCONFIG:-vendor/sm8150-qgki_defconfig}
DEVICE=${DEVICE:-cepheus}
ARCH=${ARCH:-arm64}

CONFIG_DIR="${ROOT_DIR}/arch/${ARCH}/configs"
DEVICE_CONFIG_DIR="${CONFIG_DIR}/vendor/xiaomi"

case "${DEVICE}" in
	cepheus)
		DEVICE_CONFIGS=(
			"${DEVICE_CONFIG_DIR}/sm8150-common.config"
			"${DEVICE_CONFIG_DIR}/cepheus.config"
		)
		;;
	*)
		echo "error: unsupported DEVICE '${DEVICE}'" >&2
		exit 1
		;;
esac

if [[ -n "${CROSS_COMPILE:-}" ]]; then
	CROSS_COMPILE_ARG=("CROSS_COMPILE=${CROSS_COMPILE}")
else
	CROSS_COMPILE_ARG=()
fi

if [[ -n "${CLANG_PREBUILT_BIN:-}" ]]; then
	PATH="${ROOT_DIR}/${CLANG_PREBUILT_BIN}:${PATH}"
	export PATH
fi

MAKE_ARGS=(
	"ARCH=${ARCH}"
	"O=${OUT_DIR}"
	"${CROSS_COMPILE_ARG[@]}"
)

if [[ "${LLVM:-1}" == "1" ]]; then
	MAKE_ARGS+=(LLVM=1 LLVM_IAS=1)
fi

mkdir -p "${OUT_DIR}"

echo "Configuring ${DEVICE} from ${DEFCONFIG}"
KCONFIG_CONFIG="${OUT_DIR}/.config" \
	"${ROOT_DIR}/scripts/kconfig/merge_config.sh" -m \
	"${CONFIG_DIR}/${DEFCONFIG}" "${DEVICE_CONFIGS[@]}"
make -C "${ROOT_DIR}" "${MAKE_ARGS[@]}" olddefconfig

if [[ "${BUILD_CONFIG_ONLY:-0}" == "1" ]]; then
	exit 0
fi

echo "Building kernel and device trees"
make -C "${ROOT_DIR}" "${MAKE_ARGS[@]}" -j"${JOBS:-$(nproc)}" \
	Image.gz dtbs dtbo.img dtb.img

BOOT_DIR="${OUT_DIR}/arch/${ARCH}/boot"
ARTIFACT_DIR="${OUT_DIR}/boot"
[[ -f "${BOOT_DIR}/Image.gz" ]] || {
	echo "error: kernel image was not generated" >&2
	exit 1
}
[[ -f "${BOOT_DIR}/dtb.img" ]] || {
	echo "error: kernel DTB image was not generated" >&2
	exit 1
}
[[ -f "${BOOT_DIR}/dtbo.img" ]] || {
	echo "error: DTBO image was not generated" >&2
	exit 1
}

mkdir -p "${ARTIFACT_DIR}"
cp "${BOOT_DIR}/Image.gz" "${ARTIFACT_DIR}/kernel"
cp "${BOOT_DIR}/dtb.img" "${ARTIFACT_DIR}/kernel_dtb"
cp "${BOOT_DIR}/dtbo.img" "${ARTIFACT_DIR}/dtbo.img"

echo "Built artifacts:"
printf '  %s\n' "${ARTIFACT_DIR}/kernel" \
	"${ARTIFACT_DIR}/kernel_dtb" "${ARTIFACT_DIR}/dtbo.img"