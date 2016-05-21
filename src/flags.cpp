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

#include "flags.hpp"

#include <ctype.h>   /* for isgraph */
#include <libgen.h>  /* for basename */
#include <stdlib.h>  /* for atoi */
#include <string.h>  /* for strdup */

namespace flag {

using namespace std;


static unsigned int longest_long_name = 0;
static string prog_name;


// Special Flags.
DEFINE_flag(help, 0, "Display flags for this command.");


/**
 * Inserts the default help output for all registered flags into the argument
 * ostream.
 *
 *   sh$ ash_log --help
 *   Usage: ash_log [options]
 *         --help  Display this message.
 *   sh$
 */
void Flag::show_help(ostream & out) {
  char * program_name = strdup(prog_name.c_str());
  out << "\nUsage: " << basename(program_name);

  list<Flag *> & flags = Flag::instances();
  if (flags.empty()) return;

  out << " [options]";
  for (list<Flag *>::iterator i = flags.begin(); i != flags.end(); ++i) {
    out << "\n" << **i;
  }
  out << "\n" << endl;
}


/**
 * Parses the main method argc and argv values.  If the remove_flags argument is
 * true - the flags and values are removed from the flag list.
 */
int Flag::parse(int * p_argc, char *** p_argv, const bool remove_flags) {
  int argc = *p_argc;
  char ** argv = *p_argv;

  // Grab the program name from argv[0] for --help output later.
  prog_name = argv[0];

  // Put the options into an array, as getopt expects them.
  struct option * options = new struct option[Flag::options().size() + 1];
  int x = 0;
  typedef list<struct option>::iterator iter;
  for (iter i = Flag::options().begin(), e = Flag::options().end(); i != e; ++i) {
    options[x++] = *i;
  }
  // This sentinel is needed to prevent the getopt library from segfaulting
  // when an unknown long option is seen first on the list of flags.
  struct option sentinel = {0, 0, 0, 0};
  options[x] = sentinel;

  // Parse the arguments.
  for (int c = 0, index = 0; c != -1; index = 0) {
    c = getopt_long(argc, argv, Flag::codes().c_str(), options, &index);
    switch (c) {
      case -1: break;

      case 0: {  // longopt with no short name.
        const string long_name = options[index].name;
        Flag * flag = Flag::long_names()[long_name];
        if (flag) flag -> set(optarg);
        if (flag == &FLAGS_OPT_help) {
          Flag::show_help(cout);
          delete [] options;
          exit(0);
        }
        break;
      }

      case '?': {  // unknown option.
        Flag::show_help(cerr);
        delete [] options;
        exit(1);
      }

      default: {  // short option
        if (Flag::short_names().find(c) == Flag::short_names().end()) {
          // This should never happen.
          cerr << "ERROR: failed to find a flag matching '" << c << "'" << endl;
        } else {
          Flag * flag = Flag::short_names()[c];
          if (flag) flag -> set(optarg);
        }
        break;
      }
    }
  }

  // Trim the non-argument params from argv.
  if (remove_flags) {
    for (*p_argc = 0; optind < argc; ++optind, ++(*p_argc)) {
      argv[*p_argc] = argv[optind];
    }
  }

  delete [] options;
  return 0;
}


/**
 * Adds a flag to the map of flag names to values.  Detects name collisions.
 */
template <typename T>
void safe_add(map<const T, Flag *> & known, const T key, Flag * value) {
  if (known.find(key) != known.end()) {
    cerr << "ERROR: ambiguous flags defined: duplicate key: "
         << "'" << key << "'\n" << *known[key] << "\n" << *value << endl;
  }
  known[key] = value;
}


/**
 * Returns true if all characters in the input string are isgraph.
 */
bool all_isgraph(const char * input) {
  for (int i = 0; input && input[i] != '\0'; ++i) {
    if (!isgraph(input[i]))
      return false;
  }
  return true;
}


/**
 * Constructs a Flag object.
 *
 * Args:
 *   ln: the long name of the flag (if any).
 *   sn: a single character shor name of the flag (if any).
 *   ds: the human-readable description of the flag.
 *   ha: bool; true if this flag requires an argument, otherwise false.
 */
Flag::Flag(const char * ln, const char sn, const char * ds, const bool ha)
  : long_name(ln), short_name(sn), description(ds), has_arg(ha)
{
  Flag::instances().push_back(this);

  // Map the names to this Flag object (if names are valid).
  if (short_name && isgraph(short_name))
    safe_add(Flag::short_names(), short_name, this);
  if (all_isgraph(long_name)) {
    safe_add(Flag::long_names(), string(long_name), this);
  } else {
    cerr << "WARNING: Flag long name '" << long_name
         << "' is not legal and will be ignored." << endl;
  }

  // Keep track of the longest name for later help output formatting.
  string temp(long_name);
  if (has_arg) {
    temp += "=VALUE";
  }
  if (temp.length() > longest_long_name) {
    longest_long_name = temp.length();
  }

  // Create an option struct and add it to the list.
  struct option opt = {ln, has_arg ? 1 : 0, 0, sn};
  Flag::options().push_back(opt);

  // Add the short_name to a flag_code string.
  if (short_name) {
     if (isgraph(short_name)) {
      Flag::codes().push_back(short_name);
      if (has_arg) {
        Flag::codes().push_back(':');
      }
    } else {
      cerr << "WARNING: Flag short name character '" << short_name
           << "' is not legal and will be ignored." << endl;
    }
  }
}


/**
 * Destroys a Flag - this is required because there are virtual functions in
 * the class.
 */
Flag::~Flag() {
  // Nothing to do.
}


/**
 * Inserts this Flag into an ostream and then returns it.
 */
ostream & Flag::insert(ostream & out) const {
  if (short_name)
    out << "  -" << short_name;
  else
    out << "    ";

  if (long_name) {
    int padding = 2 + longest_long_name - string(long_name).length();
    out << "  --" << long_name;
    if (has_arg) {
      out << "=VALUE";
      padding -= 6;
    }
    out << string(padding, ' ');
  }

  if (description) out << description;
  return out;
}


/**
 * Inserts a Flag into the ostrea.
 */
ostream & operator << (ostream & out, const Flag & flag) {
  return flag.insert(out);
}


/**
 * Initializes an IntFlag.
 */
IntFlag::IntFlag(const char * ln, const char sn, int * val, const int dv, const char * ds)
  : Flag(ln, sn, ds, true), value(val)
{
  *value = dv;
}


/**
 * Sets the value of this IntFlag.
 *
 *   Args:
 *     optarg: the value to convert to an int using atoi.
 */
void IntFlag::set(const char * optarg) {
  *value = atoi(optarg);
}


/**
 * Inserts this IntFlag into an ostream.
 */
ostream & IntFlag::insert(ostream & out) const {
  Flag::insert(out);
  if (value && *value) {
    out << "  Default: " << *value;
  }
  return out;
}


/**
 * Initialize a StringFlag.
 */
StringFlag::StringFlag(const char * ln, const char sn, string * val, const char * dv, const char * ds)
  : Flag(ln, sn, ds, true), value(val)
{
  set(dv);
}


/**
 * Set the value of this StringFlag.
 */
void StringFlag::set(const char * optarg) {
  if (optarg) {
    *value = string(optarg);
  } else {
    value -> clear();
  }
}


/**
 * Inserts this StringFlag into an ostream.
 */
ostream & StringFlag::insert(ostream & out) const {
  Flag::insert(out);
  if (!value -> empty()) {
    out << "  Default: '" << *value << "'";
  }
  return out;
}


/**
 * Initializes a BoolFlag.
 */
BoolFlag::BoolFlag(const char * ln, const char sn, bool * val, const bool dv, const char * ds, const bool has_arg)
  : Flag(ln, sn, ds, has_arg), value(val)
{
  *val = dv;
}


/**
 * Sets the value of this flag by parsing the argument string.
 */
void BoolFlag::set(const char * optarg) {
  if (optarg) {
    string opt(optarg);
    if (opt == "true") {
      *value = true;
    } else if (opt == "false") {
      *value = false;
    } else {
      cerr << "ERROR: boolean flags must be either true or false.  Got '"
           << optarg << "'" << endl;
    }
  } else {
    *value = true;
  }
}


/**
 * Inserts this BoolFlag into the argument ostream.
 */
ostream & BoolFlag::insert(ostream & out) const {
  Flag::insert(out);
  if (has_arg) {
    out << "  Default: " << (*value ? "true" : "false");
  }
  return out;
}


}  // namespace flag
