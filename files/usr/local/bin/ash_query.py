#!/usr/bin/python
#
# Copyright 2012 Carl Anderson
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
"""A script to query command history from a sqlite3 database.

This script fetches data from a command history database, using one of several
user-defined queries.

TOOD(cpa): add logging to this at some point.
"""
from __future__ import print_function

__author__ = 'Carl Anderson (carl.anderson@gmail.com)'
__version__ = '0.7r0'


import csv
import os
import re
import sys

# Allow the local advanced_shell_history library to be imported.
_LIB = '/usr/local/lib'
if _LIB not in sys.path:
  sys.path.append(_LIB)

from advanced_shell_history import util


class Flags(util.Flags):
  """A class to manage all the flags for the command logger."""

  arguments = (
    ('d', 'database', 'DB', str, 'a history database to query'),
    ('f', 'format', 'FMT', str, 'a format to display results'),
    ('l', 'limit', 'LINES', int, 'a limit to the number of lines returned'),
    ('p', 'print_query', 'NAME', str, 'print the query SQL'),
    ('q', 'query', 'NAME', str, 'the name of the saved query to execute'),
  )

  flags = (
    ('F', 'list_formats', 'display all available formats'),
    ('H', 'hide_headings', 'hide column headings from query results'),
    ('Q', 'list_queries', 'display all saved queries'),
  )

  def __init__(self):
    """Initialize the Flags."""
    util.Flags.__init__(self, Flags.arguments, Flags.flags)


class Queries(object):
  """A class to store all the queries available to ash_query.py.

  Queries are parsed from /usr/local/etc/advanced-shell-history/queries and
  ~/.ash/queries and are made available to the command line utility.

  TODO(cpa): if there is an error in the file, something should be printed.
  """
  queries = []
  show_headings = True
  parser = re.compile(r"""
    \s*(?P<query_name>[A-Za-z0-9_-]+)\s*:\s*{\s*
      description\s*:\s*
        (?P<description>
          "([^"]|\\")*"      # A double-quoted string.
        )\s*
      sql\s*:\s*{
        (?P<sql>
          (
            [$]{[^}]*}     | # Shell variable expressions: ${FOO} or ${BAR:-0}
            [^}]             # Everything else in the query.
          )*
        )
      }\s*
    }""", re.VERBOSE)

  @classmethod
  def Init(cls):
    if cls.queries: return

    # Load the queries from the system query file, and also the user file.
    data = []
    system_queries = util.Config().GetString('SYSTEM_QUERY_FILE')
    user_queries = os.path.join(os.getenv('HOME'), '.ash', 'queries')
    for filename in (system_queries, user_queries):
      if not filename or not os.path.exists(filename): continue
      lines = [x for x in open(filename).readlines() if x and x[0] != '#']
      data.extend([x[:-1] for x in lines if x[:-1]])

    # Parse the loaded config files.
    cls.queries = {}  # {name: (description, sql)}
    for match in cls.parser.finditer('\n'.join(data)):
      query_name = match.group('query_name')
      description = match.group('description') or '""'
      cls.queries[query_name] = (description[1:-1], match.group('sql'))

  @classmethod
  def Get(cls, query_name):
    if not query_name or not query_name in cls.queries: return (None, None)
    raw = cls.queries[query_name][1]
    sql = os.popen('/bin/cat <<EOF_ASH_SQL\n%s\nEOF_ASH_SQL' % raw).read()
    return (raw, sql)

  @classmethod
  def PrintQueries(cls):
    data = sorted([(query, desc) for query, (desc, _) in cls.queries.items()])
    data.insert(0, ['Query', 'Description'])
    AlignedFormatter.PrintRows(data)


class Formatter(object):
  """A base class for an object that formats query results into a stream."""
  formatters = []
  separator = '   '
  show_headings = True

  def __init__(self, name, desc):
    Formatter.formatters.append(self)
    self.name = name
    self.desc = desc

  @classmethod
  def PrintTypes(cls):
    data = sorted([(x.name, x.desc) for x in cls.formatters])
    data.insert(0, ['Format', 'Description'])
    AlignedFormatter.PrintRows(data)

  @classmethod
  def Get(cls, name):
    for fmt in cls.formatters:
      if fmt.name == name:
        return fmt
    return None

  @classmethod
  def GetWidths(cls, rows):
    widths = [0 for _ in rows[0]]
    max_column_width = 80  # TODO(cpa): make this configurable.
    # Skip the headings row, if that flag was specified.
    if not cls.show_headings:
      rows = rows[1:]

    # Calculate the min widths of each column.
    for row in rows:
      i = 0
      for col in row:
        if col:
          widths[i]= max(widths[i], min(max_column_width, len(str(col))))
        i += 1
    return widths


class AlignedFormatter(Formatter):
  @classmethod
  def PrintRows(cls, rows):
    # Print the result set rows aligned.
    widths = Formatter.GetWidths(rows)
    fmt = Formatter.separator.join(['%%%ds' % -width for width in widths])
    for row in rows:
      print(fmt % tuple(row))

  def Print(self, rs):
    AlignedFormatter.PrintRows(rs)


class AutoFormatter(Formatter):
  def GetGroupedLevelCount(self, rows, widths):
    """Get the optimal number of levels to group, minimizing screen area.

    Examine the columns from left to right simulating how much screen space
    would be saved by grouping that column.  If there is a net reduction in
    screen 'area', then the column will be grouped.  Otherwise it will not.

    Store area of output after simulating grouping at each level successively.
    the rightmost minimum area will be chosen.

    For example, consider the following areas after simulating grouping:
      areas = [100, 90, 92, 90, 140, 281]

    With 1 level of grouping and with 3 levels of grouping we get the same
    screen area, however the rightmost value is chosen, so the return value
    will be 3.
    """
    rows = rows[1:]  # Skip headings.
    XX = len(Formatter.separator)
    width = sum(widths) + XX * (len(widths) - 1)
    length = len(rows)
    min_area = length * width
    areas = [min_area for _ in widths]

    for c in xrange(len(widths)):
      # Test each row in the column to see if it is a duplicate of the previous
      # row.  If so, it will be de-duped in the output.  If not, it means an
      # extra row will be added, so we adjust the length variable accordingly.
      prev = None
      for row in rows:
        if prev != row[c]:
          length += 1
          prev = row[c]

      # To calculate the new width, we need to consider both the width of the
      # grouped column and the width of the remaining columns.  We also need to
      # consider the width of the indent.
      width = max(width - widths[c], widths[c]) + XX * (c + 1)
      min_area = min(length * width, min_area)
      if c < len(widths) - 1:
        areas[c + 1] = width * length

    # Find the rightmost minimum area from all simulated areas.
    for c in xrange(len(widths), 0, -1):
      if areas[c - 1] == min_area:
        return c - 1
    return 0

  def Print(self, rs):
    """Prints a result set using the minimum screen space possible."""
    if not rs: return
    widths = Formatter.GetWidths(rs)
    levels = self.GetGroupedLevelCount(rs, widths)
    cols = len(widths)

    # Print the headings.
    # Each grouped heading appears on its own row, with the following row
    # indented one extra separator.
    if Formatter.show_headings:
      for c in xrange(cols):
        if c < levels:
          grouped_header = '%s\n%s' % (rs[0][c], Formatter.separator * (c + 1))
          sys.stdout.write(grouped_header)
        else:
          parts = ['%%%ds' % -w for w in widths[c:-1]] + ['%s']
          fmt = Formatter.separator.join(parts)
          print(fmt % rs[0][c:])
          break

    # Print the result set values.
    prev = [None for _ in xrange(levels)]
    for row in rs[1:]:
      for c in xrange(cols):
        value = row[c]
        if c < levels:
          if value != prev[c]:
            # Within the grouped range, but the value needs to be printed.
            sys.stdout.write(str(value))
            if c < cols - 1:
              sys.stdout.write('\n' + Formatter.separator * (c + 1))
              for x in xrange(c, levels):
                prev[x] = None
            prev[c] = value
          else:
            # Grouped case: only print the indent.
            sys.stdout.write(Formatter.separator)
        else:
          # Normal case: non-grouped columns.
          parts = ['%%%ds' % -w for w in widths[c:-1]] + ['%s']
          fmt = Formatter.separator.join(parts)
          print(fmt % tuple(row)[c:])
          break


class CSVFormatter(Formatter):
  """Prints a result set with values separated by commas.

  Non-numeric values are quoted, regardless of whether they need quoting.
  """
  def Print(self, rs):
    if not Formatter.show_headings:
      rs = rs[1:]
    if rs:
      writer = csv.writer(sys.stdout, quoting=csv.QUOTE_NONNUMERIC)
    for row in rs:
      writer.writerow(tuple(row))


class NullFormatter(Formatter):
  """Prints a result set with values delimited by a null character (\0)."""
  def Print(self, rs):
    if not Formatter.show_headings:
      rs = rs[1:]
    for row in rs:
      print('\0'.join([str(x) for x in row]))


def InitFormatters():
  """Create instances of each Formatter available to ash_query.py."""
  AlignedFormatter('aligned', 'Columns are aligned and separated with spaces.')
  AutoFormatter('auto', 'Redundant values are automatically grouped.')
  CSVFormatter('csv', 'Columns are comma separated with strings quoted.')
  NullFormatter('null', 'Columns are null separated with strings unquoted.')


def main(argv):
  # Setup.
  util.InitLogging()

  # Print an alert if one was specified.
  flags = Flags()

  # If no arguments were given, it may be best to show --help.>>
  if len(argv) == 1:
    config = util.Config()
    if config.Sets('DEFAULT_QUERY'):
      flags.query = config.GetString('DEFAULT_QUERY')
    elif not config.GetBool('HIDE_USAGE_FOR_NO_ARGS'):
      flags.PrintHelp()

  # Initialize the formatters that will display the results of the query.
  InitFormatters()
  Formatter.show_headings = not flags.hide_headings
  if flags.list_formats:
    Formatter.PrintTypes()
    return 0

  # Read the queries from the config files.
  Queries.Init()
  Queries.show_headings = not flags.hide_headings
  if flags.list_queries:
    Queries.PrintQueries()

  elif flags.print_query:
    raw, sql = Queries.Get(flags.print_query)
    if not raw:
      sys.stderr.write('Query not found: %s\n' % flags.print_query)
      return 1
    if raw.strip() != sql.strip():
      msg = 'Query: %s\nTemplate Form:\n%s\nActual SQL:\n%s'
      print(msg % (flags.print_query, raw, sql))
    else:
      print('Query: %s\n%s' % (flags.print_query, sql))

  elif flags.query:
    # Get the formatter to be used to print the result set.
    default = util.Config().GetString('DEFAULT_FORMAT') or 'aligned'
    format_name = flags.format or default
    fmt = Formatter.Get(format_name)
    if not fmt:
      sys.stderr.write('Unknown format: %s\n' % format_name)
      return 1

    sql = Queries.Get(flags.query)[1]
    rs = util.Database().Fetch(sql, limit=flags.limit)
    fmt.Print(rs)

  return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv))
