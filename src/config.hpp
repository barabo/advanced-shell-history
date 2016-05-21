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

#ifndef __ASH_CONFIG__
#define __ASH_CONFIG__

#include <map>
#include <string>

using std::map;
using std::string;

namespace ash {


/**
 * This class contains all the environment variable values for variables named
 * with a common prefix: ASH_CFG_
 */
class Config {
  // STATIC:
  public:
    static Config & instance();

  // NON-STATIC:
  public:
    bool has(const string & key) const;
    bool sets(const string & key, const bool dv=false) const;
    int get_int(const string & key, const int dv=1) const;
    const char * get_cstring(const string & key, const char * dv="") const;
    string get_string(const string & key, const string & dv="") const;

  private:
    Config();

  private:
    map<string, string> values;
    bool is_loaded;

  private:  // DISALLOWED:
    Config(const Config & other);
    Config & operator=(const Config & other);
};


} // namespace ash

#endif  /* __ASH_CONFIG__ */
