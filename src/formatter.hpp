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

/**
 * This class provides a base and some concrete examples of classes that are
 * designed to take a DB ResultSet and output the contents in a formatted way.
 */

#ifndef __ASH_FORMATTER__
#define __ASH_FORMATTER__

#include <iostream>
#include <map>
#include <string>

namespace ash {

using std::map;
using std::ostream;
using std::string;

class ResultSet;  // forward declaration.


/**
 * Abstract base class for an object that inserts a ResultSet into an ostream.
 */
class Formatter {
  // STATIC:
  public:
    static Formatter * lookup(const string & name);
    static map<string, string> get_desc();

  private:
    static map<string, Formatter *> instances;

  // NON-STATIC:
  public:
    virtual ~Formatter();

    virtual void insert(const ResultSet * rs, ostream & out) const = 0;

    void show_headings(bool show);

  protected:
    Formatter(const string & name, const string & description);

  protected:
    const string name, description;
    bool do_show_headings;

  // DISALLOWED:
  private:
    Formatter(const Formatter & other);
    Formatter & operator = (const Formatter & other);
};


/**
 * This is a convenience macro intended to be used only in this header.
 * It defines a class that must implement a static init and non-static insert.
 */
#define FORMATTER(NAME) \
  class NAME ## Formatter : public Formatter { \
    public: \
      static void init(); \
  \
    public: \
      virtual ~NAME ## Formatter() {} \
      virtual void insert(const ResultSet * rs, ostream & out) const; \
  \
    protected: \
      NAME ## Formatter(const string & name, const string & description) \
        : Formatter(name, description) {} \
  }


/**
 * Singleton class that converts a result set into output, spacing all columns
 * evenly using spaces to align columns.  Each column is as wide as its widest
 * element.
 */
FORMATTER(Spaced);


/**
 * Singleton class that converts a result set into output, delimiting all
 * columns with commas.
 */
FORMATTER(Csv);


/**
 * Singleton class that converts a result set into output, collapsing repeated
 * values on the left edge if the net result saves printed space.
 */
FORMATTER(Grouped);


/**
 * Singleton class that converts a result set into output, delimiting all
 * columns with \0 characters (NULL).
 */
FORMATTER(Null);


}  // namespace ash

#endif  /* __ASH_FORMATTER__ */
