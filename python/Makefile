#
#   Copyright 2016 Carl Anderson
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
VERSION := 0.8r0
VERSIONED := _ash_log.py advanced_shell_history/*.py ash_query.py

.default: version

version:
	sed -i -e "s:^__version__ = .*:__version__ = '${VERSION}':" ${VERSIONED}

clean:
	find . -type f -name '*.pyc' | xargs rm -f
