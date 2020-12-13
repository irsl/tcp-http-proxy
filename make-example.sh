#!/bin/bash
export OPENWRT_SDK_ROOT=/home/kali/openwrt-sdk/openwrt-sdk-19.07.5-ath79-generic_gcc-7.5.0_musl.Linux-x86_64
export STAGING_DIR="$OPENWRT_SDK_ROOT/staging_dir/"
GCC="$STAGING_DIR/toolchain-mips_24kc_gcc-7.5.0_musl/bin/mips-openwrt-linux-musl-gcc"
$GCC tcp-http-proxy.c -o tcp-http-proxy
