#!/usr/bin/python
#
# Copyright 2017 Carl Anderson
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
"""A module to log commands and shell session details into a sqlite3 database.

This module logs a command into a command history database, while also gathering
system-specific metadata.
"""
from __future__ import print_function

__author__ = 'Carl Anderson (carl.anderson@gmail.com)'
__version__ = '0.8r0'


import logging
import os
import sys

# Allow the local advanced_shell_history library to be imported.
_LIB = '/usr/local/lib'
if _LIB not in sys.path:
  sys.path.append(_LIB)

from advanced_shell_history import unix
from advanced_shell_history import util


class Flags(util.Flags):
  """The flags needed for the _ash_log.py script to work."""

  arguments = (
    ('a', 'alert', 'MSG', str, 'a message to display to the user'),
    ('c', 'command', 'CMD', str, 'a command to log'),
    ('e', 'command_exit', 'CODE', int, 'the exit code of the command to log'),
    ('p', 'command_pipe_status', 'CSV', str, 'the pipe states of the command to log'),
    ('s', 'command_start', 'TS', int, 'the timestamp when the command started'),
    ('f', 'command_finish', 'TS', int, 'the timestamp when the command stopped'),
    ('n', 'command_number', 'NUM', int, 'the builtin shell history command number'),
    ('x', 'exit', 'CODE', int, 'the exit code to use when exiting'),
  )

  flags = (
    ('S', 'get_session_id', 'emits the session ID (or creates one)'),
    ('E', 'end_session', 'ends the current session'),
  )

  def __init__(self):
    util.Flags.__init__(self, Flags.arguments, Flags.flags)


class Session(util.Database.Object):
  """An abstraction of a shell session to store to the history database."""

  def __init__(self):
    """Initialize a Session, populating session values."""
    util.Database.Object.__init__(self, 'sessions')
    self.values = {
      'time_zone': unix.GetTimeZone(),
      'start_time': unix.GetTime(),
      'ppid': unix.GetPPID(),
      'pid': unix.GetPID(),
      'tty': unix.GetTTY(),
      'uid': unix.GetUID(),
      'euid': unix.GetEUID(),
      'logname': unix.GetLoginName(),
      'hostname': unix.GetHostName(),
      'host_ip': unix.GetHostIp(),
      'shell': unix.GetShell(),
      'sudo_user': unix.GetEnv('SUDO_USER'),
      'sudo_uid': unix.GetEnv('SUDO_UID'),
      'ssh_client': unix.GetEnv('SSH_CLIENT'),
      'ssh_connection': unix.GetEnv('SSH_CONNECTION')
    }

  def GetCreateTableSql(self):
    return '''
CREATE TABLE sessions ( 
  id integer primary key autoincrement, 
  hostname varchar(128), 
  host_ip varchar(40), 
  ppid int(5) not null, 
  pid int(5) not null, 
  time_zone str(3) not null, 
  start_time integer not null, 
  end_time integer, 
  duration integer, 
  tty varchar(20) not null, 
  uid int(16) not null, 
  euid int(16) not null, 
  logname varchar(48), 
  shell varchar(50) not null, 
  sudo_user varchar(48), 
  sudo_uid int(16), 
  ssh_client varchar(60), 
  ssh_connection varchar(100) 
)'''

  def Close(self):
    """Closes this session in the database."""
    sql = '''
      UPDATE sessions
      SET
        end_time = ?,
        duration = ? - start_time
      WHERE id == ?;
    '''
    ts = unix.GetTime()
    util.Database().Execute(sql, (ts, ts, unix.GetEnvInt('ASH_SESSION_ID'),))


class Command(util.Database.Object):
  """An abstraction of a command to store to the history database."""
  def __init__(self, command, rval, start, finish, number, pipes):
    util.Database.Object.__init__(self, 'commands')
    self.values = {
      'session_id': unix.GetEnvInt('ASH_SESSION_ID'),
      'shell_level': unix.GetEnvInt('SHLVL'),
      'command_no': number,
      'tty': unix.GetTTY(),
      'euid': unix.GetEUID(),
      'cwd': unix.GetCWD(),
      'rval': rval,
      'start_time': start,
      'end_time': finish,
      'duration': finish - start,
      'pipe_cnt': len(pipes.split('_')),
      'pipe_vals': pipes,
      'command': command
    }
    # If the user changed directories, CWD will be the new directory, not the
    # one where the command was actually entered.
    if rval == 0 and (command == 'cd' or command.startswith('cd ')):
      self.values['cwd'] = unix.GetEnv('OLDPWD')

  def GetCreateTableSql(self):
    return '''
CREATE TABLE commands (
  id integer primary key autoincrement,
  session_id integer not null,
  shell_level integer not null,
  command_no integer,
  tty varchar(20) not null,
  euid int(16) not null,
  cwd varchar(256) not null,
  rval int(5) not null,
  start_time integer not null,
  end_time integer not null,
  duration integer not null,
  pipe_cnt int(3),
  pipe_vals varchar(80),
  command varchar(1000) not null,
UNIQUE(session_id, command_no)
)'''


def main(argv):
  # If ASH_DISABLED is set, we skip everything and exit without error.
  if os.getenv('ASH_DISABLED'): return 0

  # Setup.
  util.InitLogging()

  # Log the command, if debug logging is enabled.
  if logging.getLogger().isEnabledFor(logging.DEBUG):
    command = []
    for arg in argv:
      command.append('[%d]=\'%s\'' % (len(command), arg))
    logging.debug('argv = "' + ','.join(command) + '"')

  # Print an alert if one was specified.
  flags = Flags()
  if flags.alert:
    print(flags.alert, file=sys.stderr)

  # If no arguments were given, it may be best to show --help.
  if len(argv) == 1 and not util.Config().GetBool('HIDE_USAGE_FOR_NO_ARGS'):
    flags.PrintHelp()

  # Create the session id, if not already set in the environment.
  session_id = os.getenv('ASH_SESSION_ID')
  if flags.get_session_id:
    if session_id is None:
      session_id = Session().Insert()
    print(session_id)

  # Insert a new command into the database, if one was supplied.
  command_flag_used = bool(flags.command
    or flags.command_exit
    or flags.command_pipe_status
    or flags.command_start
    or flags.command_finish
    or flags.command_number)
  if command_flag_used:
    Command(
      flags.command, flags.command_exit, flags.command_start,
      flags.command_finish, flags.command_number, flags.command_pipe_status
    ).Insert()

  # End the current session.
  if flags.end_session:
    Session().Close()

  # Return the desired exit code.
  return flags.exit


if __name__ == '__main__':
  sys.exit(main(sys.argv))
