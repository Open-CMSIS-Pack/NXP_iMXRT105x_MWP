#!/usr/bin/env bash
# Version: 3.0
# Date: 2023-11-06
# This bash script generates doxygen documentation
#
# Pre-requisites:
# - bash shell (for Windows: install git for Windows)
# - doxygen 1.9.2

set -o pipefail

# Set version of gen pack library
# For available versions see https://github.com/Open-CMSIS-Pack/gen-pack/tags.
# Use the tag name without the prefix "v", e.g., 0.7.0
REQUIRED_GEN_PACK_LIB="0.9.2"

DIRNAME=$(dirname $(readlink -f $0))
REQ_DXY_VERSION="1.9.2"

############ DO NOT EDIT BELOW ###########

# Set GEN_PACK_LIB_PATH to use a specific gen-pack library root
# ... instead of bootstrap based on REQUIRED_GEN_PACK_LIB
if [[ -f "${GEN_PACK_LIB_PATH}/gen-pack" ]]; then
  . "${GEN_PACK_LIB_PATH}/gen-pack"
else
  . <(curl -sL "https://raw.githubusercontent.com/Open-CMSIS-Pack/gen-pack/main/bootstrap")
fi

find_doxygen "${REQ_DXY_VERSION}"

pushd "${DIRNAME}" > /dev/null || exit 1

echo_log "Generating documentation ..."

echo_log "\"${UTILITY_DOXYGEN}\" iMXRT105x_MWP.dxy"
"${UTILITY_DOXYGEN}" iMXRT105x_MWP.dxy

popd > /dev/null || exit 1

exit 0
