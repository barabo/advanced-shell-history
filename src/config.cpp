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


#include "config.hpp"

#include <stdlib.h>  /* for getenv */
#include <unistd.h>  /* for environ */

#include <iostream>

using namespace ash;
using namespace std;


extern char ** environ;  /* populated by unistd.h */


/**
 * Construct a Config object, setting defaults.
 */
Config::Config()
  : values(), is_loaded(false)
{
  // NOTHING TO DO!
}


/**
 * Returns a char * value for an environment variable prefixed with ASH_CFG_.
 */
char * get_ash_env(const string & key) {
  if (key.find("ASH_CFG_", 0, 8) == 0)
    return getenv(key.c_str());
  return getenv((string("ASH_CFG_") + key).c_str());
}


/**
 * Returns true if the environment actually contains this variable.
 */
bool Config::has(const string & key) const {
  return get_ash_env(key) != NULL;
}


/**
 * Returns true if the environment contains this value and it equals 'true'.
 */
bool Config::sets(const string & key, const bool dv) const {
  char * env = get_ash_env(key);
  return env ? "true" == string(env) : dv;
}


/**
 * Returns the int value of the requested environment variable.
 */
int Config::get_int(const string & key, const int dv) const {
  char * env = get_ash_env(key);
  return env ? atoi(env) : dv;
}


/**
 * Returns the char * value of the requested environment variable.
 */
const char * Config::get_cstring(const string & key, const char * dv) const {
  char * env = get_ash_env(key);
  return env ? string(env).c_str() : dv;
}


/**
 * Returns the string value of the requested environment variable.
 */
string Config::get_string(const string & key, const string & dv) const {
  char * env = get_ash_env(key);
  return env ? string(env) : dv;
}


/**
 * Returns a singleton Config instance.
 */
Config & Config::instance() {
  static Config _instance;
  if (!_instance.is_loaded) {
    // Find all environment variables matching a common prefix ASH_CFG_
    for (int i = 0; environ[i] != NULL; ++i) {
      string line = environ[i];
      if (line.substr(0, 8) == "ASH_CFG_") {
        int first_equals = line.find_first_of('=');
        string key = line.substr(8, first_equals - 8);
        string value = line.substr(first_equals + 1);
        _instance.values[key] = value;
      }
    }
    _instance.is_loaded = true;
  }
  return _instance;
}

