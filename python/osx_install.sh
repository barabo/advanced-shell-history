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
# NOTE: this script is intended to be used by OSX users who do not want to
#       install the Xcode command line utils.
#
# NOTE: this script should be used as a last resort and NOT as the primary
#       method of installing the system.  It might not be maintained.
#

set -e
set -u

# From the directory where this script lives:
cd ${0%/*}

# Fetch the argparse library, if needed on this system.
[ -e advanced_shell_history/argparse.py ] || ./fetch_argparse.sh

# Taken from the master makefile.
REV="$( svn up | cut -d' ' -f3 | cut -d. -f1 | sed -e 's:^:.r:' )"
VERSION="$( grep '^VERSION[ ]*:=[ ]*' ../Makefile | awk '{ print $3 }' )"
RVERSION="${VERSION:-0}${REV:-}"

# Taken from the python/Makefile version target.
sed -i -e "s:^__version__ = .*:__version__ = '${RVERSION}':" \
  _ash_log.py \
  advanced_shell_history/*.py \
  ash_query.py

# From the parent directory, prepare the overlay for /
cd ..

# Taken from the makefile build_python target.
chmod 555 python/*.py python/advanced_shell_history/*.py
cp -af python/*.py files/usr/local/bin
cp -af python/advanced_shell_history/*.py \
  files/usr/local/lib/advanced_shell_history/
chmod 775 python/*.py python/*/*.py

# Taken from the makefile fixperms target.
chmod 644 files/usr/lib/advanced_shell_history/* files/etc/ash/*

# Taken from the makefile man pages target.
MAN_DIR=/usr/share/man/man1
printf "\nGenerating man pages...\n"
sed -e "s:__VERSION__:Version ${RVERSION}:" man/_ash_log.1 \
  | sed -e "s:__DATE__:$( \
      stat -c %y man/_ash_log.1 \
        | cut -d' ' -f1 ):" \
  | gzip -9 -c > ./files${MAN_DIR}/_ash_log.py.1.gz
sed -e "s:__VERSION__:Version ${RVERSION}:" man/ash_query.1 \
  | sed -e "s:__DATE__:$( \
      stat -c %y man/ash_query.1 \
        | cut -d' ' -f1 ):" \
  | gzip -9 -c > ./files${MAN_DIR}/ash_query.py.1.gz
chmod 644 ./files${MAN_DIR}/*ash*.1.gz

# Taken from the makefile overlay.tar.gz target.
cd files
sudo tar -cpzf ../overlay.tar.gz $( \
  find . -type f -o -type l \
    | grep -v '\.svn' \
)
cd -

# Taken from master makefile uninstall target.
printf "\nUninstalling Advanced Shell History...\n"
sudo rm -rf /usr/lib/advanced_shell_history
sudo rm -f /usr/local/bin/_ash_log.py /usr/local/bin/ash_query.py
sudo rm -f ${MAN_DIR}/_ash_log.1.py.gz ${MAN_DIR}/ash_query.1.py.gz
sudo rm -f ${MAN_DIR}/advanced_shell_history

# If the intent is simply to uninstall, just do that here before installing.
if [[ "${1:-}" == "--uninstall" ]]; then
  echo "Uninstall complete."
  exit 0
fi

# Taken from the master makefile install_python target.
BEGIN_URL="http://code.google.com/p/advanced-shell-history/wiki/HOWTO_Begin"
printf "\nInstalling Python Advanced Shell History...\n"
echo -e "\nInstalling files:"
sudo tar -xpv --no-same-owner -C / -f overlay.tar.gz
printf "\n 0/ - Install completed!\n<Y    See: ${BEGIN_URL}\n/ \\ \n"

