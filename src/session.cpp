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

#include "session.hpp"

#include "unix.hpp"

#include <sstream>


using namespace ash;
using namespace std;


/**
 * Registers this table for use in the Database.
 */
void Session::register_table() {
  string name = "sessions";
  stringstream ss;
  ss << "CREATE TABLE IF NOT EXISTS " << name << " ( \n"
     << "  id integer primary key autoincrement, \n"
     << "  hostname varchar(128), \n"
     << "  host_ip varchar(40), \n"
     << "  ppid int(5) not null, \n"
     << "  pid int(5) not null, \n"
     << "  time_zone str(3) not null, \n"
     << "  start_time integer not null, \n"
     << "  end_time integer, \n"
     << "  duration integer, \n"
     << "  tty varchar(20) not null, \n"
     << "  uid int(16) not null, \n"
     << "  euid int(16) not null, \n"
     << "  logname varchar(48), \n"
     << "  shell varchar(50) not null, \n"
     << "  sudo_user varchar(48), \n"
     << "  sudo_uid int(16), \n"
     << "  ssh_client varchar(60), \n"
     << "  ssh_connection varchar(100) \n"
     << ");";
  DBObject::register_table(name, ss.str());
}


/**
 * Initialize a Session object.
 */
Session::Session() {
  values["time_zone"] = unix::time_zone();
  values["start_time"] = unix::time();
  values["ppid"] = unix::ppid();
  values["pid"] = unix::pid();
  values["tty"] = unix::tty();
  values["uid"] = unix::uid();
  values["euid"] = unix::euid();
  values["logname"] = unix::login_name();
  values["hostname"] = unix::host_name();
  values["host_ip"] = unix::host_ip();
  values["shell"] = unix::shell();
  values["sudo_user"] = unix::env("SUDO_USER");
  values["sudo_uid"] = unix::env("SUDO_UID");
  values["ssh_client"] = unix::env("SSH_CLIENT");
  values["ssh_connection"] = unix::env("SSH_CONNECTION");
}


/**
 * This is required since it was declared virtual in the base class.
 */
Session::~Session() {
  // Nothing to do here.
}


/**
 * Return the name of the table backing this class.
 */
const string Session::get_name() const {
  return "sessions";
}


/**
 * Returns a query to finalize this Session in the sessions table.
 */
const string Session::get_close_session_sql() const {
  stringstream ss;
  ss << "UPDATE sessions \n"
     << "SET \n"
     << "  end_time = " << unix::time() << ", \n"
     << "  duration = " << unix::time() << " - start_time \n"
     << "WHERE id == " << unix::env("ASH_SESSION_ID") << "; ";
  return ss.str();
}
