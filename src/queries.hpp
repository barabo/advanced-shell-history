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
#ifndef __ASH_QUERIES__
#define __ASH_QUERIES__

#include <list>
#include <map>
#include <string>

namespace ash {


using std::list;
using std::map;
using std::string;


/**
 * A container for all queries specified in /etc/ash/queries or in the user
 * defined ~/.ash/queries file.
 */
class Queries {
  public:
    static void add(string & name, string & desc, string & sql);
    static bool has(const string & name);

    static list<string> get_names();

    static map<string, string> get_desc();
    static string get_desc(const string & name);

    static map<string, string> get_sql();
    static string get_raw_sql(const string & name);
    static string get_sql(const string & name);

  private:
    static void lazy_load();

  private:
    static map<string, string> descriptions;
    static map<string, string> queries;

  private:
    Queries();
};


}  // namespace ash

#endif  // __ASH_QUERIES__

