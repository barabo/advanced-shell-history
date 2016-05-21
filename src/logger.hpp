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
/*

  This utility provides a simple logger complete with various levels of
  visibility.  This is essentially a small subset interface of the Google
  logging library (glog) with no extra dependencies.

*/
#ifndef __ASH_LOGGER__
#define __ASH_LOGGER__

#include <fstream>
using std::ostream;
using std::ofstream;

namespace ash {


/**
 * The severity levels to which logged messages can be set.  This allows
 * users to easily add logged messages that are intended only to be seen
 * while debugging.
 */
enum Severity {
  DEBUG,
  INFO,
  WARNING,
  ERROR,
  FATAL,
  UNKNOWN
};


/**
 * This class extends the ostream base to take advantage of predefined
 * templates while internally passing input to an ofstream which writes to
 * a designated log file.
 */
class Logger : public ostream {
  public:
    Logger(const Severity level);
    ~Logger();

    /**
     * This templated method simply hands-off insertion to the underlying
     * ofstream.
     */
    template <typename insertable>
    ostream & operator << (insertable something) {
      return log << something;
    }

    /**
     * This is needed to handle the special case where a stream manipulator
     * is inserted into a Logger first.  For example: LOG(INFO) << endl;
     */
    typedef std::basic_ostream<char, std::char_traits<char> > StreamType;
    typedef StreamType & (*Manipulator) (StreamType &);
    ostream & operator << (const Manipulator & manipulate) {
      return manipulate(log);
    }

  private:
    ofstream log;
    Severity level;

  // DISALLOWED:
  private:
    Logger(const Logger & other);
    Logger & operator = (const Logger & other);
};


/**
 * This macro allows users to use LOG(DEBUG) instead of Logger(DEBUG).
 */
#ifndef LOG
#define LOG(level) (Logger(level))
#endif


}  // namespace: ash

#endif  /* __ASH_LOGGER__ */
