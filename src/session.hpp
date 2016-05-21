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

#ifndef __ASH_SESSION__
#define __ASH_SESSION__

#include <map>
#include <string>

#include "database.hpp"

using std::map;
using std::string;

namespace ash {


/**
 * This class encapsulates session-specific data.
 */
class Session : public DBObject {
  public:
    static void register_table();

  public:
    Session();
    virtual ~Session();

    virtual const string get_close_session_sql() const;
    virtual const string get_name() const;
};


}  // namespace ash

#endif  /* __ASH_SESSION__ */

