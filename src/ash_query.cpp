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
 * This program is designed to invoke saved queries in /etc/ash/queries and
 * ~/.ash/queries allowing multiple output formatting styles.
 */

#include "ash_query.hpp"

#include "command.hpp"
#include "config.hpp"
#include "database.hpp"
#include "flags.hpp"
#include "formatter.hpp"
#include "logger.hpp"
#include "queries.hpp"
#include "session.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>

using namespace ash;
using namespace flag;
using namespace std;


DEFINE_string(database, 'd', 0, "A history database to query.");
DEFINE_string(format, 'f', 0, "A format to display results.");
DEFINE_int(limit, 'l', 0, "Limit the number of rows returned.");
DEFINE_string(print_query, 'p', 0, "Print the query SQL.");
DEFINE_string(query, 'q', 0, "The name of the saved query to execute.");

DEFINE_flag(list_formats, 'F', "Display all available formats.");
DEFINE_flag(hide_headings, 'H', "Hide column headings from query results.");
DEFINE_flag(list_queries, 'Q', "Display all saved queries.");
DEFINE_flag(reverse, 'R', "display results in reverse order.");
DEFINE_flag(version, 0, "Show the version and exit.");


typedef map<string, string> RowsType;


/**
 * Prints a mapping of string to string using a fixed width between columns.
 */
void display(ostream & out, const RowsType & rows, const string & name) {
  RowsType::const_iterator i, e;
  const size_t XX = 4;  // The number of spaces between columns.
  size_t widths[2] = {name.size() + XX, string("Description").size() + XX};

  // Calculate the required widths for each column.
  for (i = rows.begin(), e = rows.end(); i != e; ++i) {
    widths[0] = max(widths[0], XX + (i -> first).size());
    widths[1] = max(widths[1], XX + (i -> second).size());
  }

  // Output the headings and the rows.
  cout << left
       << setw(widths[0]) << name
       << setw(widths[1]) << "Description" << endl;
  for (i = rows.begin(), e = rows.end(); i != e; ++i) {
    cout << left
         << setw(widths[0]) << i -> first
         << setw(widths[1]) << i -> second << endl;
  }
}


/**
 * Executes a query, printing the results to stdout according to the
 * user-chosen output format.
 */
int execute(const string & sql) {
  Config & config = Config::instance();

  // Get the filename backing the database we are about to query.
  string db_file(FLAGS_database);
  if (db_file == "") {
    if (config.get_string("HISTORY_DB") == "") {
      cerr << "Expected either --database or ASH_CFG_HISTORY_DB "
           << "to be defined." << endl;
      return 1;
    }
    db_file = config.get_string("HISTORY_DB");
  }

  // Prepare the DB for reading.
  Session::register_table();
  Command::register_table();
  Database db(db_file);

  // Get the intended Formatter before executing the query.
  string format = FLAGS_format == ""
    ? config.get_string("DEFAULT_FORMAT", "aligned")
    : FLAGS_format;
  Formatter * formatter = Formatter::lookup(format);
  if (!formatter) {
    cerr << "\nUnknown format: '" << format << "'" << endl;
    display(cerr << '\n', Formatter::get_desc(), "Format");
    return 1;
  }

  // Execute the query and display any results.
  ResultSet * rs = db.exec(sql, FLAGS_limit, FLAGS_reverse);
  formatter -> show_headings(!FLAGS_hide_headings);
  formatter -> insert(rs, cout);
  if (rs) delete rs;
  return 0;
}


/**
 * Query the history database.
 */
int main(int argc, char ** argv) {
  // Load the config from the environment.
  Config & config = Config::instance();

  if (argc == 1) {  // No flags.
    if (config.sets("DEFAULT_QUERY")) {
      return execute(config.get_string("DEFAULT_QUERY"));
    }
    if (!config.sets("HIDE_USAGE_FOR_NO_ARGS")) {
      Flag::parse(&argc, &argv, true);  // Sets the prog name in help output.
      Flag::show_help(cerr);
    }
    return 1;
  }

  // Parse the flags, removing from argv and argc.
  Flag::parse(&argc, &argv, true);

  // Abort if unrecognized flags were used on the command line.
  if (argc != 0 && !config.sets("IGNORE_UNKNOWN_FLAGS")) {
    cerr << "unrecognized flag: " << argv[0] << endl;
    Flag::show_help(cerr);
    return 1;
  }

  // Display version, if that's all that was wanted.
  if (FLAGS_version) {
    cout << ASH_VERSION << endl;
    return 0;
  }

  // Display available query names, if requested.
  if (FLAGS_list_queries) {
    display(cout, Queries::get_desc(), "Query");
    return 0;
  }

  // Initialize the available formatters.
  CsvFormatter::init();
  NullFormatter::init();
  SpacedFormatter::init();
  GroupedFormatter::init();

  // Diaplay the available format names.
  if (FLAGS_list_formats) {
    display(cout, Formatter::get_desc(), "Format");
    return 0;
  }

  // Print the requested query (both generic and actual, if different).
  if (FLAGS_print_query != "") {
    string sql = Queries::get_sql(FLAGS_print_query);
    string raw = Queries::get_raw_sql(FLAGS_print_query);
    if (raw == "") {
      cout << "Query not found: " << FLAGS_print_query << "\nAvailable:\n";
      display(cout, Queries::get_desc(), "Query");
      return 1;
    }
    cout << "Query: " << FLAGS_print_query << endl;
    if (raw != sql) {
      cout << "Template Form:\n" << raw << "\nActual SQL:\n";
    }
    cout << sql << endl;
    return 0;
  }

  // Make sure the requested query exists.
  string sql = Queries::get_sql(FLAGS_query);
  if (sql == "") {
    cout << "Query not found: " << FLAGS_query << "\nAvailable:\n";
    display(cout, Queries::get_desc(), "Query");
    return 1;
  }

  // Execute the requested query.
  return execute(sql);
}
