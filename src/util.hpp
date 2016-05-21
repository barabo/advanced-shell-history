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

#ifndef __ASH_UTIL__
#define __ASH_UTIL__


#include <string>

using std::string;

namespace ash {


/**
 * This class is intended to hold all the commonly-used helper methods.
 */
class Util {
  public:
    static string to_string(int);
};


} // namespace ash

#endif  /* __ASH_UTIL__ */
