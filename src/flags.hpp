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
   This class provides flags for programs, much in the same way that Google
   gflags does, although with fewer bells and whistles and no major system
   dependencies (only getopt).
*/

#ifndef __ASH_FLAGS__
#define __ASH_FLAGS__

#include <getopt.h>  /* for struct option */

#include <iostream>
#include <list>
#include <map>
#include <string>

namespace flag {

using std::ostream;
using std::list;
using std::map;
using std::string;


/**
 * Clients of this library define flags like this:
 *   DEFINE_int(example, "E", -1, "An example flag.");
 *
 * This example defines an integer-value flag that can either be specified
 * on the command line by --example or -E.  The default value is -1 and when
 * users invoke the command with the --help flag the "An example flag."
 * description is printed.
 *
 * Clients must also parse the main method argc and argv as follows:
 *   Flag::parse(&argc, &argv, true);
 *
 * Clients then use this flag in code by referencing it as follows:
 *   if (FLAG_example > 42) return;
 *
 * Clients wishing to use flags that don't require values should use the
 * DEFINE_flag macro.
 *
 * Clients wishing to not specify a single-character shortcut version should
 * use 0 instead of a quoted character.  For example:
 *   DEFINE_int(example, 0, -1, "An example flag (with no shortcut).");
 *
 * Clients wishing to only specify a single-character shortcut should name
 * the flag the single-character they want.  For example:
 *   DEFINE_int(e, 0, -1, "An example single-character-only flag.");
 *
 * Clients wishing to have the flag-related parameters stripped from argv and
 * the count adjusted in argc should use the 'true' option for Flag::parse.
 * By leaving this false, the flag-related arguments are left in argv.
 */

#define DEFINE_int(long_name, short_name, default_val, desc) \
static int FLAGS_ ## long_name; \
static flag::IntFlag FLAGS_OPT_ ## long_name(#long_name, short_name, \
  &FLAGS_ ## long_name, default_val, desc)

#define DEFINE_string(long_name, short_name, default_val, desc) \
static string FLAGS_ ## long_name; \
static flag::StringFlag FLAGS_OPT_ ## long_name(#long_name, short_name, \
  &FLAGS_ ## long_name, default_val, desc)

#define DEFINE_bool(long_name, short_name, default_val, desc) \
static bool FLAGS_ ## long_name; \
static flag::BoolFlag FLAGS_OPT_ ## long_name(#long_name, short_name, \
  &FLAGS_ ## long_name, default_val, desc, true)

#define DEFINE_flag(long_name, short_name, desc) \
static bool FLAGS_ ## long_name; \
static flag::BoolFlag FLAGS_OPT_ ## long_name(#long_name, short_name, \
  &FLAGS_ ## long_name, false, desc, false)

#define STATIC_SINGLETON(name, type_name) \
static type_name & name() { \
  static type_name rval; \
  return rval; \
}

/**
 * This class makes it easy to implement command-line flags.  Class instances
 * are created by the preprocessor macros defined above.
 */
class Flag {
  // STATIC
  public:
    static int parse(int * argc, char *** argv, const bool remove_flags);
    static void show_help(ostream & out);

  private:
    STATIC_SINGLETON(codes, string)
    STATIC_SINGLETON(options, list<struct option>)
    STATIC_SINGLETON(instances, list<Flag *>)
    // Note: these two maps do not use the STATIC_SINGLETON preprocessor macro
    //       because the type name contains a comma; this confuses the parser.
    //STATIC_SINGLETON(short_names, map<const char, Flag *>)
    //STATIC_SINGLETON(long_names, map<const string, Flag *>)
    static map<const char, Flag *> & short_names() {
      static map<const char, Flag *> rval;
      return rval;
    }
    static map<const string, Flag *> & long_names() {
      static map<const string, Flag *> rval;
      return rval;
    }

  // NON-STATIC
  public:
    Flag(const char * long_name, const char short_name, const char * desc,
         const bool has_arg=false);
    virtual ~Flag();

    virtual ostream & insert(ostream & out) const;

    virtual void set(const char * optarg) = 0;

  private:
    const char * long_name;
    const char short_name;
    const char * description;
  protected:
    const bool has_arg;

  // DISABLED
  private:
    Flag(const Flag & other);
    Flag & operator = (const Flag & other);
};


/**
 * Inserts a Flag into an ostream.
 */
ostream & operator << (ostream & out, const Flag & flag);


/**
 * A command-line flag containing an integer value.
 */
class IntFlag : public Flag {
  public:
    IntFlag(const char * ln, const char sn, int * val, const int dv,
            const char * ds);
    virtual ~IntFlag() {}
    virtual void set(const char * optarg);
    virtual ostream & insert(ostream & out) const;

  private:
    int * value;
};


/**
 * A command-line flag containing a string value.
 */
class StringFlag : public Flag {
  public:
    StringFlag(const char * ln, const char sn, string * val, const char * dv,
               const char * ds);
    virtual ~StringFlag() {}
    virtual void set(const char * optarg);
    virtual ostream & insert(ostream & out) const;

  private:
    string * value;
};


/**
 * A command-line flag containing a bool value.
 */
class BoolFlag : public Flag {
  public:
    BoolFlag(const char * ln, const char an, bool * val, const bool dv,
             const char * ds, const bool has_arg);
    virtual ~BoolFlag() {}
    virtual void set(const char * optarg);
    virtual ostream & insert(ostream & out) const;

  private:
    bool * value;
};

}  // namespace flag

#endif  /* __ASH_FLAGS__ */
