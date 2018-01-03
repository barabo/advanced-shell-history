#
# Copyright 2018 Carl Anderson
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
"""A library for general use by the advanced shell history utilities.

This library provides components used by _ash_log.py and ash_query.py for
logging, flag parsing, configuration and database management.
"""

__author__ = 'Carl Anderson (carl.anderson@gmail.com)'
__version__ = '0.8r2'


import argparse
import logging
import os
import sqlite3
import sys


class Flags(argparse.ArgumentParser):
  """A class to manage all the flags for this advanced shell history utility."""

  class Formatter(argparse.HelpFormatter):
    """A simple formatter whith a slightly wider set of flag names."""
    def __init__(self, prog):
      argparse.HelpFormatter.__init__(self, prog, max_help_position=44)

  def __init__(self, arguments=None, flags=None):
    """Initialize the Flags."""
    parser = argparse.ArgumentParser(formatter_class=Flags.Formatter)

    # Add the standard argument-taking flags.
    for short_flag, long_flag, metavar, arg_type, help_text in arguments or []:
      parser.add_argument('-' + short_flag, '--' + long_flag, metavar=metavar,
                          type=arg_type, help=help_text)

    # Add the standard no-argument-taking flags.
    for short_flag, long_flag, help_text in flags or []:
      parser.add_argument('-' + short_flag, '--' + long_flag,
                          action='store_true', help=help_text)

    # Add a flag to display the version and exit.
    parser.add_argument('-V', '--version', action='version', version=__version__,
                        help='prints the version and exits')

    self._parser = parser
    self.flags = parser.parse_args().__dict__
    self.__dict__.update(self.flags)

  def PrintHelp(self):
    """Prints the help menu."""
    self._parser.print_help()


class Config(object):
  """A class to manage the configuration environment variables.

  All environment variables beginning with the prefix 'ASH_CFG_' are loaded
  and made accessible conveniently through an instance of this class.

  For example:
    ASH_CFG_HISTORY_DB='/foo/' becomes { 'HISTORY_DB': '/foo/' }
  """

  def __init__(self):
    """Initialize a Config instance, reading os.environ for variables."""
    # Select all the environment variables starting with 'ASH_CFG_' and strip
    # off the leading ASH_CFG_ portion to use as the name of the variable.
    self.variables = dict(
      [(x[8:], y) for x, y in os.environ.items() if x.startswith('ASH_CFG_')]
    )

  def GetBool(self, variable):
    """Returns a bool value for a config variable, or None if not set."""
    value = self.GetString(variable)
    return value and value.strip() == 'true'

  def GetString(self, variable):
    """Returns a string value for a config variable, or None if not set."""
    if self.Sets(variable):
      return self.variables[variable.upper().strip()]

  def Sets(self, variable):
    """Returns true when the argument variable exists in the environment."""
    return variable and variable.upper().strip() in self.variables


def InitLogging():
  """Initializes the logging module.

  Uses the following shell environment variables to configure the logger:
    ASH_CFG_LOG_DATE_FMT - to format the date strings in the log file.
    ASH_CFG_LOG_LEVEL - to set the logging level (DEBUG, INFO, etc).
    ASH_CFG_LOG_FILE - the filename where the logger will write.
    ASH_SESSION_ID - the session id to include in the logged output.

  Lines are written in roughly this format:
    2012-07-17 23:59:59 PDT: SESSION 123: DEBUG: argv = "[0]='ls'"
  """
  session_id = os.getenv('ASH_SESSION_ID') or 'NEW'
  config = Config()
  level = config.GetString('LOG_LEVEL') or 'INFO'
  level = hasattr(logging, level) and getattr(logging, level) or logging.DEBUG
  fmt = '%(asctime)sSESSION ' + session_id + ': %(levelname)s: %(message)s'
  kwargs = {
      'datefmt': config.GetString('LOG_DATE_FMT'),
      'filename': config.GetString('LOG_FILE'),
      'format': fmt,
      'level': level,
  }
  logging.basicConfig(**kwargs)


class Database(object):
  """A wrapper around a database connection."""

  # The name of the sqlite3 file backing the saved command history.
  filename = None

  class Object(object):
    """A construct for objects to be inserted into the Database."""
    def __init__(self, table_name):
      self.values = {}
      self.table_name = table_name
      sql = '''
        select sql
        from sqlite_master
        where
          type = 'table'
          and name = ?;
      '''
      # Check that the table exists, creating it if not.
      db = Database()
      cur = db.cursor
      try:
        cur.execute(sql, (table_name,))
        rs = cur.fetchone()
        if not rs:
          cur.execute(self.GetCreateTableSql() + ';')
          db.connection.commit()
        elif rs[0] != self.GetCreateTableSql().strip():
          logging.warning('Table %s exists, but has an unexpected schema.',
                          table_name)
      finally:
        cur.close()

    def Insert(self):
      """Insert the object into the database, returning the new rowid."""
      sql = 'INSERT INTO %s ( %s ) VALUES ( %s )' % (
        self.table_name,
        ', '.join(self.values),
        ', '.join(['?' for _ in self.values])
      )
      return Database().Execute(sql, tuple(self.values.values()))

  def __init__(self):
    """Initialize a Database with an open connection to the history database."""
    if Database.filename is None:
      Database.filename = Config().GetString('HISTORY_DB')
    if Database.filename is None:
      logging.error('Missing ASH_CFG_HISTORY_DB variable?')
    self.connection = sqlite3.connect(Database.filename)
    self.connection.row_factory = sqlite3.Row
    self.cursor = self.connection.cursor()

  def Execute(self, sql, values):
    try:
      self.cursor.execute(sql, values)
      logging.debug('executing query: %s, values = %r', sql, values)
      return self.cursor.lastrowid
    except sqlite3.IntegrityError as e:
      logging.debug('constraint violation: %r', e)
    finally:
      self.connection.commit()
      self.cursor.close()
    return 0

  @classmethod
  def SanityCheck(cls, sql):
    return sql and sqlite3.complete_statement(sql)

  def Fetch(self, sql, params=(), limit=None):
    """Execute a select query and return the result set."""
    if self.SanityCheck(sql):
      try:
        self.cursor.execute(sql, params)
        row = self.cursor.fetchone()
        if not row: return None
        headings = tuple(row.keys())
        fetched = 1
        if limit is None or limit <= 0:
          rows = self.cursor.fetchall()
        else:
          rows = []
          while fetched < limit:
            row = self.cursor.fetchone()
            if not row: break
            rows.append(row)
            fetched += 1
        rows.insert(0, headings)
        rows.insert(1, row)
        return rows
      except sqlite3.Error as e:
        print >> sys.stderr, 'Failed to execute query: %s (%s)' % (sql, params)
        return None
      finally:
        self.cursor.close()
        self.cursor = None
