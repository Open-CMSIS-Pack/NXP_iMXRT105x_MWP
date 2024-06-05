#!/usr/bin/env bash
# Version: 3.0
# Date: 2023-11-06
# This bash script generates doxygen documentation
#
# Pre-requisites:
# - bash shell (for Windows: install git for Windows)
# - doxygen 1.9.6

set -o pipefail

# Set version of gen pack library
# For available versions see https://github.com/Open-CMSIS-Pack/gen-pack/tags.
# Use the tag name without the prefix "v", e.g., 0.7.0
REQUIRED_GEN_PACK_LIB="0.10.0"

DIRNAME=$(dirname $(readlink -f $0))
GENDIR=../html
REQ_DXY_VERSION="1.9.6"

############ DO NOT EDIT BELOW ###########

# Set GEN_PACK_LIB_PATH to use a specific gen-pack library root
# ... instead of bootstrap based on REQUIRED_GEN_PACK_LIB
if [[ -f "${GEN_PACK_LIB_PATH}/gen-pack" ]]; then
  . "${GEN_PACK_LIB_PATH}/gen-pack"
else
  . <(curl -sL "https://raw.githubusercontent.com/Open-CMSIS-Pack/gen-pack/main/bootstrap")
fi

find_git
find_doxygen "${REQ_DXY_VERSION}"

if [ -z "${VERSION_FULL}" ]; then
  VERSION_FULL=$(git_describe "v")
fi

pushd "${DIRNAME}" > /dev/null || exit 1

echo_log "Generating documentation ..."

projectName=$(grep -E "PROJECT_NAME\s+=" iMXRT105x_MWP.dxy | sed -r -e 's/[^"]*"([^"]+)".*/\1/')
projectNumberFull="${VERSION_FULL}"
projectNumber="${projectNumberFull%+*}"
datetime=$(date -u +'%a %b %e %Y %H:%M:%S')
year=$(date -u +'%Y')

echo_log "\"${UTILITY_DOXYGEN}\" iMXRT105x_MWP.dxy"
"${UTILITY_DOXYGEN}" iMXRT105x_MWP.dxy

mkdir -p "${DIRNAME}/${GENDIR}/search/"
cp -f "${DIRNAME}/style_template/search.css" "${DIRNAME}/${GENDIR}/search/"
cp -f "${DIRNAME}/style_template/search.js" "${DIRNAME}/${GENDIR}/search/"
cp -f "${DIRNAME}/style_template/navtree.js" "${DIRNAME}/${GENDIR}/"
cp -f "${DIRNAME}/style_template/resize.js" "${DIRNAME}/${GENDIR}/"

sed -e "s/{datetime}/${datetime}/" "${DIRNAME}/style_template/footer.js.in" \
  | sed -e "s/{year}/${year}/" \
  | sed -e "s/{projectName}/${projectName}/" \
  | sed -e "s/{projectNumber}/${projectNumber}/" \
  | sed -e "s/{projectNumberFull}/${projectNumberFull}/" \
  > "${DIRNAME}/${GENDIR}/footer.js"

popd > /dev/null || exit 1

exit 0
