#!/usr/bin/env bash
# Version: 3.0
# Date: 2023-11-06
# This bash script generates a CMSIS Software Pack:
#

set -o pipefail

# Set version of gen pack library
# For available versions see https://github.com/Open-CMSIS-Pack/gen-pack/tags.
# Use the tag name without the prefix "v", e.g., 0.7.0
REQUIRED_GEN_PACK_LIB="0.10.0"

# Set default command line arguments
DEFAULT_ARGS=(-c "v")

# Pack warehouse directory - destination
# Default: ./output
#
# PACK_OUTPUT=./output

# Temporary pack build directory,
# Default: ./build
#
# PACK_BUILD=./build

# Specify directory names to be added to pack base directory
# An empty list defaults to all folders next to this script.
# Default: empty (all folders)
#
PACK_DIRS="
  CMSIS
  Documentation
  Scripts
  SDK
  Templates
"

# Specify file names to be added to pack base directory
# Default: empty
#
PACK_BASE_FILES="
  LICENSE
"

# Specify file names to be deleted from pack build directory
# Default: empty
#
PACK_DELETE_FILES="
  Documentation/Doxygen
  SDK/usb_device_config.h.patch
"

# Specify patches to be applied
# Default: empty
#
PACK_PATCH_FILES="
  SDK/usb_device_config.h.patch
"

# Specify addition argument to packchk
# Default: empty
#
# PACKCHK_ARGS=()

# Specify additional dependencies for packchk
# Default: empty
#
PACKCHK_DEPS="
  ARM.CMSIS-Driver.pdsc
  NXP.MIMXRT1051_DFP.pdsc
  NXP.MIMXRT1052_DFP.pdsc
"

# Optional: restrict fallback modes for changelog generation
# Default: full
# Values:
# - full      Tag annotations, release descriptions, or commit messages (in order)
# - release   Tag annotations, or release descriptions (in order)
# - tag       Tag annotations only
#
# PACK_CHANGELOG_MODE="<full|release|tag>"

#
# custom pre-processing steps
#
# usage: preprocess <build>
#   <build>  The build folder
#
# shellcheck disable=SC2317
function preprocess() {
  # add custom steps here to be executed
  # before populating the pack build folder

  # Fetch SDK from release assets
  TAG=$(${UTILITY_GIT} describe --tags --abbrev=0 --match="v*")
  echo "Fetching SDK_2_15_100_EVKB-IMXRT1050 from '${TAG}' ..."
  ${UTILITY_GHCLI} release download "${TAG}" -p SDK_2_15_100_EVKB-IMXRT1050.zip

  # Extract SDK (NXP)
  echo "Extracting SDK_2_15_100_EVKB-IMXRT1050 ..."
  unarchive "SDK_2_15_100_EVKB-IMXRT1050.zip" "$1/SDK_NXP"

  # Create the $1/SDK folder
  mkdir -p "$1/SDK"

  # Convert the BSD licence to unix style EOL and redirect (copy) the converted BSD license to the $1/SDK/ folder
  ${UTILITY_EOL_CONVERTER['CRLF-to-LF']} < "$1/SDK_NXP/COPYING-BSD-3" > "$1/SDK/COPYING-BSD-3"

  # Convert the SW-Content-Register.txt to unix style EOL, exclude not included SW components for this Pack, 
  # and redirect (copy) the converted and updated SW-Content-Register.txt to the $1/SDK/ folder"
  ${UTILITY_EOL_CONVERTER['CRLF-to-LF']} < "$1/SDK_NXP/SW-Content-Register.txt" | \
    sed -e '/SDK_Peripheral_Driver/,/^$/d' \
        -e '/SDK_Device/,/^$/d'            \
        -e '/SDK_Components/,/^$/d'        \
        -e '/cmsis_drivers/,/^$/d'         \
        -e '/sdmmc/,/^$/d'                 \
        -e '/wifi_tx_pwr_limits/,/^$/d'    \
        -e '/segger_rtt/,/^$/d'            \
        -e '/SDK_Examples/,/^$/d'          \
    > "$1/SDK/SW-Content-Register.txt"

  # Add middleware directory to the PACK image
  mkdir -p "$1/SDK/middleware"
  cp -rf "$1/SDK_NXP/middleware/usb" "$1/SDK/middleware"
  
  # Add components directory to the PACK image
  mkdir -p "$1/SDK/components"
  cp -rf "$1/SDK_NXP/components/osa" "$1/SDK/components"

  # Remove SDK (NXP)
  rm -r "$1/SDK_NXP"

  # Generate documentation
  ./Documentation/Doxygen/gen_doc.sh

  return 0
}

#
# custom post-processing steps
#
# usage: postprocess <build>
#   <build>  The build folder
#
function postprocess() {
  # add custom steps here to be executed
  # after populating the pack build folder
  # but before archiving the pack into output folder
  return 0
}

############ DO NOT EDIT BELOW ###########

# Set GEN_PACK_LIB_PATH to use a specific gen-pack library root
# ... instead of bootstrap based on REQUIRED_GEN_PACK_LIB
if [[ -f "${GEN_PACK_LIB_PATH}/gen-pack" ]]; then
  . "${GEN_PACK_LIB_PATH}/gen-pack"
else
  . <(curl -sL "https://raw.githubusercontent.com/Open-CMSIS-Pack/gen-pack/main/bootstrap")
fi

gen_pack "${DEFAULT_ARGS[@]}" "$@"

exit 0
