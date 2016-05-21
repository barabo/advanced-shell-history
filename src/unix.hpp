/*
   Copyright 2011 Carl Anderson

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef __ASH_UNIX__
#define __ASH_UNIX__

#include <string>

using std::string;

namespace ash {
namespace unix {

/**
 * This namespace provides a variety of UNIX-related functions.
 *
 * All functions return a quoted-string value, an int string value or a string
 * representation of the keyword "null".  All are intended to be inserted
 * directly into SQL queries.
 */
const string cwd();
const string env(const char * name);
const string env_int(const char * name);
const string euid();
const string host_ip();
const string host_name();
const string login_name();
const string pid();
const string ppid();
const string shell();
const string time();
const string time_zone();
const string tty();
const string uid();

}  // namespace unix
}  // namespace ash

#endif  /* __ASH_UNIX__ */
