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

#ifndef __ASH_COMMAND__
#define __ASH_COMMAND__

#include <string>

#include "database.hpp"

using std::string;

namespace ash {


/**
 * This class represents a user-entered command to be saved in the database.
 */
class Command : public DBObject {
  public:
    static void register_table();

  public:
    Command(const string command, const int rval, const int start,
            const int end, const int num, const string pipes);
    virtual ~Command();

    virtual const string get_name() const;
};


}  // namespace ash

#endif  /* __ASH_COMMAND__ */
