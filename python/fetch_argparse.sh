#!/bin/bash
#
#
#   Copyright 2012 Carl Anderson
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#
# This script attempts to fetch the latest argparse tarball using either
# wget or curl (whichever is available).
#
# Alternatively, you can download the tarball yourself and leave it in this
# directory to be unpacked by this script.
#
# Note: this script is invoked by the Makefile, you should not need to invoke
#       it yourself!
#
set -e
set -u

PYTHON_BIN="/usr/bin/python"


##
# Logs a message and quits.
#
function fatal() {
  echo "${@}" >/dev/stderr
  exit 1
}


##
# Attempts to download a URL using either wget or curl.
#
# Args:
#   1: url to fetch.
#   2: [optional] file to save.
#
function fetch() {
  if [ -z "${FETCH_UTIL:-}" ]; then
    if which wget &>/dev/null; then
      FETCH_UTIL="wget -q"
      FETCH_OPT="-O"

    elif which curl &>/dev/null; then
      FETCH_UTIL="curl -s"
      FETCH_OPT="-o"

    fi
  fi

  if [ -z "${FETCH_UTIL:-}" ]; then
    fatal "This script requires either curl or wget be installed."
  fi
  ${FETCH_UTIL} "${1}" ${FETCH_OPT:-} "${2:-/dev/stdout}"
}


##
# Attempts to guess the most recent version of argparse available and download
# it.
#
function get_argparse() {
  local download_url="http://code.google.com/p/argparse/downloads/list"
  if [ -z "${ARGPARSE_VER:-}" ]; then
    echo "Inspecting ${download_url} to guess latest version..."
    ARGPARSE_VER="$( fetch ${download_url} \
      | grep -m1 'href=".*argparse-.*\.tar\.gz"' \
      | sed -e 's:.*href=".*argparse-\([0-9.]*\)\.tar\.gz".*:\1:' \
    )"
  fi
  if [ -z "${ARGPARSE_VER:-}" ]; then
    fatal "Failed to find an argparse download here: ${download_url}"
  fi

  ARGPARSE_DIR="argparse-${ARGPARSE_VER}"
  ARGPARSE_ZIP="${ARGPARSE_DIR}.tar.gz"
  ARGPARSE_URL="http://argparse.googlecode.com/files/${ARGPARSE_ZIP}"

  fetch "${ARGPARSE_URL}" "${ARGPARSE_ZIP}"
}


# Make sure we already have Python.
if [ ! -e "${PYTHON_BIN}" ]; then
  echo "Expected to find a python binary at: ${PYTHON_BIN}"
  exit 1
fi


# If python 2.7 or above is installed, do nothing - argparse is included.
python_version="$( "${PYTHON_BIN}" --version 2>&1 )"
max_python_version="$( echo -e "${python_version}\nPython 2.7" \
  | sort --general-numeric-sort \
  | tail -n1
)"
if [[ "$python_version" == "$max_python_version" ]]; then
  echo "Argparse is already built-in to Python 2.7 and later.  Exiting."
  exit 0
fi

# Download the tarball, if necessary.
if [ -e argparse-*.tar.gz ]; then
  ARGPARSE_ZIP="$( ls argparse-*.tar.gz | head -n1 )"
  ARGPARSE_DIR="${ARGPARSE_ZIP%.tar.gz}"
else
  get_argparse
fi

# Unpack the tarball and move the contents into this directory.
if ! tar -xvzpf "${ARGPARSE_ZIP}"; then
  # This can happen if a fetch was cancelled by Ctrl-C halfway through.
  echo -e "\nFailed to unzip ${ARGPARSE_ZIP} - is it corrupt?\n"
  echo -e "Consider deleting ${ARGPARSE_ZIP} and retrying build.\n"
  exit 1
fi

mv -f "${ARGPARSE_DIR}"/argparse.py advanced_shell_history/

# Clean up.
rm -rf "${ARGPARSE_DIR}" "${ARGPARSE_ZIP}"
