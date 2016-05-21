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

#include "_ash_log.hpp"

#include "command.hpp"
#include "config.hpp"
#include "database.hpp"
#include "flags.hpp"
#include "logger.hpp"
#include "session.hpp"
#include "unix.hpp"

#include <stdlib.h>  /* for exit, getenv */

#include <iostream>  /* for cerr, cout, endl */
#include <sstream>   /* for stringstream */


DEFINE_string(alert, 'a', 0, "A message to display to the user.");
DEFINE_string(command, 'c', 0, "The command to log.");
DEFINE_int(command_exit, 'e', 0, "The exit code of the command to log.");
DEFINE_string(command_pipe_status, 'p', 0, "The pipe states of the command to log.");
DEFINE_int(command_start, 's', 0, "The timestamp when the command started.");
DEFINE_int(command_finish, 'f', 0, "The timestamp when the command stopped.");
DEFINE_int(command_number, 'n', 0, "The builtin shell history command number.");
DEFINE_int(exit, 'x', 0, "The exit code to use when exiting.");

DEFINE_flag(version, 'V', "Prints the version and exits.");
DEFINE_flag(get_session_id, 'S', "Emits the session ID (or creates one).");
DEFINE_flag(end_session, 'E', "Ends the current session.");


using namespace ash;
using namespace flag;
using namespace std;


/**
 * Displays a brief message about how this is supposed to be used.
 */
void usage(ostream & out) {
  out << "\n\nThis program is not intended to be executed manually.\n\n"
      << "NOTE: See the man page for more details.\n";
  Flag::show_help(out);
  exit(1);
}


int main(int argc, char ** argv) {
  if (getenv("ASH_DISABLED")) return FLAGS_exit;

  // Load the config from the environment.
  Config & config = Config::instance();

  // Log the complete command, if debugging.
  stringstream ss;
  ss << "argv = '[0]='" << argv[0] << "'";
  for (int i = 1; i < argc; ++i)
    ss << ",[" << i << "]='" << argv[i] << "'";
  LOG(DEBUG) << ss.str();

  // Show usage if executed with no args.
  if (argc == 1 && !config.sets("HIDE_USAGE_FOR_NO_ARGS")) {
    Flag::parse(&argc, &argv, true);  // Sets the prog name in help output.
    usage(cerr);
  }

  // Parse the flags.
  Flag::parse(&argc, &argv, true);

  // Display the version and stop: -V
  if (FLAGS_version) {
    cout << ASH_VERSION << endl;
    return 0;
  }

  // Display a user alert: -a 'My alert'
  if (!FLAGS_alert.empty()) {
    cerr << FLAGS_alert << endl;
  }

  // Get the filename backing the history database.
  string db_file = config.get_string("HISTORY_DB");
  if (db_file == "") {
    usage(cerr << "\nExpected ASH_CFG_HISTORY_DB to be defined.");
  }

  // Register the tables expected in the program.
  Session::register_table();
  Command::register_table();

  // Emit the current session number, inserting one if none exists: -S
  if (FLAGS_get_session_id) {
    Database db = Database(db_file);
    stringstream ss;
    char * id = getenv("ASH_SESSION_ID");
    if (id) {
      ss << "select count(*) as session_cnt from sessions where id = " << id
         << " and duration is null;";
      ResultSet * rs = db.exec(ss.str());
      if (!rs || rs -> rows != 1) {
        cerr << "ERROR: session_id(" << id << ") not found, "
             << "creating new session." << endl << ss.str() << endl;
        id = 0;
      }
      ss.str("");
    }

    if (id) {
      cout << id << endl;
    } else {
      Session session;
      cout << db.insert(&session) << endl;
    }
  }

  // Insert a command into the DB if there's a command to insert.
  const bool command_flag_used = !FLAGS_command.empty()
    || FLAGS_command_exit
    || !FLAGS_command_pipe_status.empty()
    || FLAGS_command_start
    || FLAGS_command_finish
    || FLAGS_command_number;

  if (command_flag_used) {
    Database db = Database(db_file);
    Command com(FLAGS_command, FLAGS_command_exit, FLAGS_command_start,
      FLAGS_command_finish, FLAGS_command_number, FLAGS_command_pipe_status);
    db.insert(&com);
  }

  // End the current session in the DB: -E
  if (FLAGS_end_session) {
    char * id = getenv("ASH_SESSION_ID");
    if (id == NULL) {
      LOG(ERROR) << "Can't end the current session: ASH_SESSION_ID undefined.";
    } else {
      Session session;
      Database db = Database(db_file);
      db.exec(session.get_close_session_sql());
    }
  }

  // Set the exit code to match what the previous command exited: -e 123
  return FLAGS_exit;
}

