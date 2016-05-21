#
#   Copyright 2011 Carl Anderson
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

REV := r0
VERSION  := 0.7
RVERSION := ${VERSION}${REV}
TMP_ROOT := /tmp
TMP_DIR  := ${TMP_ROOT}/ash-${VERSION}
TMP_FILE := ${TMP_DIR}.tar.gz
MAN_DIR  := /usr/share/man/man1
SRC_DEST := ..

BEGIN_URL := https://github.com/barabo/advanced-shell-history

.PHONY: all build build_c build_python clean fixperms install install_c install_python man mrproper src_tarball src_tarball_minimal uninstall version
all:	build man

new:	clean all

version:
	sed -i -e "/^VERSION :=/s/:= .*/:= ${RVERSION}/" python/Makefile src/Makefile

build_python: version
	@ printf "\nCompiling source code...\n"
	@ cd python && make
	chmod 555 python/*.py python/advanced_shell_history/*.py
	cp -af python/*.py files/usr/local/bin
	cp -af python/advanced_shell_history/*.py files/usr/local/lib/advanced_shell_history/
	chmod 775 python/*.py python/*/*.py

build_c: version
	@ printf "\nCompiling source code...\n"
	@ cd src && make
	chmod 555 src/_ash_log src/ash_query
	cp -af src/_ash_log src/ash_query files/usr/local/bin

build: build_python build_c

man:
	@ printf "\nGenerating man pages...\n"
	sed -e "s:__VERSION__:Version ${RVERSION}:" man/_ash_log.1 \
	  | sed -e "s:__DATE__:$$( \
	      stat -c %y man/_ash_log.1 \
	        | cut -d' ' -f1 ):" \
	  | gzip -9 -c > ./files${MAN_DIR}/_ash_log.1.gz
	sed -e "s:__VERSION__:Version ${RVERSION}:" man/ash_query.1 \
	  | sed -e "s:__DATE__:$$( \
	      stat -c %y man/ash_query.1 \
	        | cut -d' ' -f1 ):" \
	  | gzip -9 -c > ./files${MAN_DIR}/ash_query.1.gz
	cp -af ./files${MAN_DIR}/_ash_log.1.gz ./files${MAN_DIR}/_ash_log.py.1.gz
	cp -af ./files${MAN_DIR}/ash_query.1.gz ./files${MAN_DIR}/ash_query.py.1.gz
	chmod 644 ./files${MAN_DIR}/*ash*.1.gz

fixperms:
	chmod 644 files/usr/lib/advanced_shell_history/* files/etc/ash/*

overlay.tar.gz: fixperms
	@ cd files && \
	sudo tar -cpzf ../overlay.tar.gz $$( \
	  find . -type f -o -type l \
	    | grep -v '\.svn' \
	)

install: build man overlay.tar.gz uninstall
	@ echo "\nInstalling files:"
	sudo tar -xpv --no-same-owner -C / -f overlay.tar.gz
	@ printf "\n 0/ - Install completed!\n<Y    See: ${BEGIN_URL}\n/ \\ \n"

install_python: build_python man overlay.tar.gz uninstall
	@ printf "\nInstalling Python Advanced Shell History...\n"
	@ echo "\nInstalling files:"
	sudo tar -xpv --no-same-owner -C / -f overlay.tar.gz
	@ printf "\n 0/ - Install completed!\n<Y    See: ${BEGIN_URL}\n/ \\ \n"

install_c: build_c man overlay.tar.gz uninstall
	@ printf "\nInstalling C++ Advanced Shell History...\n"
	@ echo "\nInstalling files:"
	sudo tar -xpv --no-same-owner -C / -f overlay.tar.gz
	@ printf "\n 0/ - Install completed!\n<Y    See: ${BEGIN_URL}\n/ \\ \n"

uninstall:
	@ printf "\nUninstalling Advanced Shell History...\n"
	sudo rm -rf /etc/ash /usr/lib/advanced_shell_history
	sudo rm -f /usr/local/bin/_ash_log /usr/local/bin/ash_query
	sudo rm -f /usr/local/bin/_ash_log.py /usr/local/bin/ash_query.py
	sudo rm -f ${MAN_DIR}/_ash_log.1.gz ${MAN_DIR}/ash_query.1.gz
	sudo rm -f ${MAN_DIR}/_ash_log.py.1.gz ${MAN_DIR}/ash_query.py.1.gz
	sudo rm -f ${MAN_DIR}/advanced_shell_history

tarball:
	mkdir -p ${TMP_DIR}
	rsync -Ca * ${TMP_DIR}
	cd ${TMP_ROOT} && tar -czpf ${TMP_FILE} ./ash-${VERSION}/
	rm -rf ${TMP_DIR}

src_tarball_minimal: mrproper tarball
	mv ${TMP_FILE} ${SRC_DEST}/ash-${RVERSION}-minimal.tar.gz

src_tarball: clean tarball
	mv ${TMP_FILE} ${SRC_DEST}/ash-${RVERSION}.tar.gz

mrproper: clean
	rm -f python/advanced_shell_history/argparse.py

clean:	version
	@ printf "\nCleaning temp and trash files...\n"
	cd src && make distclean
	rm -f files/usr/local/bin/_ash_log*
	rm -f files/usr/local/bin/ash_query*
	rm -f files/usr/local/lib/advanced_shell_history/util.py
	rm -f files/usr/local/lib/advanced_shell_history/__init__.py
	rm -f files/usr/local/lib/advanced_shell_history/unix.py
	rm -f files/usr/local/lib/advanced_shell_history/argparse.py
	rm -f files/usr/share/man/man1/_ash_log.py.1.gz
	rm -f files/usr/share/man/man1/_ash_log.1.gz
	rm -f files/usr/share/man/man1/ash_query.1.gz
	rm -f files/usr/share/man/man1/ash_query.py.1.gz
	rm -f python/advanced_shell_history/*.pyc
	rm -rf ${TMP_DIR} ${TMP_FILE} overlay.tar.gz
