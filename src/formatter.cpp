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

#include "formatter.hpp"

#include <algorithm>
#include <iomanip>
#include <vector>

#include "database.hpp"
#include "logger.hpp"


using namespace ash;
using namespace std;


/**
 * A mapping of name to singleton instance of all initialized Formatters.
 */
map<string, Formatter *> Formatter::instances;


/**
 * Returns the Formatter singleton matching the argument name, if found;
 * otherwise returns NULL.
 */
Formatter * Formatter::lookup(const string & name) {
  return instances.find(name) == instances.end() ? 0 : instances[name];
}


/**
 * Returns a map of formatter names to descriptions.
 */
map<string, string> Formatter::get_desc() {
  map<string, string> rval;
  map<string, Formatter *>::iterator i, e;

  for (i = instances.begin(), e = instances.end(); i != e; ++i) {
    if (i -> second)
      rval[i -> first] = i -> second -> description;
  }
  return rval;
}


/**
 * Creates a Formatter, making sure it has a unique name among all Formatters.
 */
Formatter::Formatter(const string & n, const string & d)
  : name(n), description(d), do_show_headings(true)
{
  if (lookup(name)) {
    LOG(FATAL) << "Conflicting formatters declared: " << name;
  }
  instances[name] = this;
}


/**
 * Destroys this formatter.
 */
Formatter::~Formatter() {
  instances[name] = 0;
}


/**
 * Sets the internal state controlling whether headings are shown or not.
 */
void Formatter::show_headings(bool show) {
  do_show_headings = show;
}


/**
 * Makes this Formatter avaiable for use within the program.
 */
void SpacedFormatter::init() {
  static SpacedFormatter instance("aligned",
    "Columns are aligned and separated with spaces.");
}


/**
 * Returns the maximum widths required for each column in a result set.
 */
vector<size_t> get_widths(const ResultSet * rs, bool do_show_headings) {
  vector<size_t> widths;
  const size_t XX = 4;  // The number of spaces between columns.

  // Initialize with the widths of the headings.
  size_t c = 0;
  ResultSet::HeadersType::const_iterator i, e;
  for (i = (rs -> headers).begin(), e = (rs -> headers).end(); i != e; ++i, ++c)
    if (do_show_headings)
      widths.push_back(XX + i -> size());
    else
      widths.push_back(XX);

  // Limit the width of columns containing very wide elements.
  size_t max_w = 80;  // TODO(cpa): make this a flag or configurable.

  // Loop ofer the rs.data looking for max column widths.
  for (size_t r = 0; r < rs -> rows; ++r) {
    for (size_t c = 0; c < rs -> columns; ++c) {
      widths[c] = max(widths[c], min(max_w, XX + (rs -> data[r][c]).size()));
    }
  }

  return widths;
}


/**
 * Calculates the ideal width for each column and inserts column data
 * left-aligned and separated by spaces.
 */
void SpacedFormatter::insert(const ResultSet * rs, ostream & out) const {
  if (!rs) return;  // Sanity check.

  vector<size_t> widths = get_widths(rs, do_show_headings);

  // Print the headings, if not suppressed.
  if (do_show_headings) {
    size_t c = 0, cols = widths.size();
    ResultSet::HeadersType::const_iterator i, e;
    for (i = (rs -> headers).begin(), e = (rs -> headers).end(); i != e; ++i) {
      if (c < cols - 1) out << left << setw(widths[c++]);
      out << *i;
    }
    out << endl;
  }

  // Iterate over the data once more, printing.
  for (size_t r = 0; r < rs -> rows; ++r) {
    for (size_t c = 0; c < rs -> columns; ++c) {
      if (c < rs -> columns - 1) out << left << setw(widths[c]);
      out << (rs -> data)[r][c];
    }
    out << endl;
  }
}


/**
 * Inserts a ResultSet with all values delimited by a common delimiter.
 */
void insert_delimited(const ResultSet * rs, ostream & out, const string & d,
                      const bool do_show_headings)
{
  if (!rs) return;

  const ResultSet::HeadersType & headers = rs -> headers;
  ResultSet::HeadersType::const_iterator i, e;

  if (do_show_headings) {
    size_t c = 0;
    for (i = headers.begin(), e = headers.end(); i != e; ++i, ++c)
      // Don't add a delimiter after the last column.
      out << *i << (c + 1 < rs -> columns ? d : "");
    out << endl;
  }

  // Loop ofer the rs.data inserting delimited text.
  for (size_t r = 0; r < rs -> rows; ++r) {
    for (size_t c = 0; c < rs -> columns; ++c) {
      out << rs -> data[r][c] << (c + 1 < rs -> columns ? d : "");
    }
    out << endl;
  }
}


/**
 * Makes this Formatter avaiable for use within the program.
 */
void CsvFormatter::init() {
  static CsvFormatter instance("csv",
    "Columns are comma separated with strings quoted.");
}


/**
 * Inserts data separated by commas.
 */
void CsvFormatter::insert(const ResultSet * rs, ostream & out) const {
  insert_delimited(rs, out, ",", do_show_headings);
}


/**
 * Makes this Formatter avaiable for use within the program.
 */
void NullFormatter::init() {
  static NullFormatter instance("null",
    "Columns are null separated with strings quoted.");
}


/**
 * Inserts data separated by \0 characters.
 */
void NullFormatter::insert(const ResultSet * rs, ostream & out) const {
  insert_delimited(rs, out, string("\0", 1), do_show_headings);
}


/**
 * Makes this Formatter avaiable for use within the program.
 */
void GroupedFormatter::init() {
  static GroupedFormatter instance("auto",
    "Automatically group redundant values.");
}


/**
 * Determines how many levels should be auto-grouped.
 */
int get_grouped_level_count(const ResultSet * rs, const vector<size_t> & widths)
{
  if (!rs) return 0;  // Sanity check.

  size_t width = 0, length = rs -> rows, XX = 4;
  for (size_t i = 0, e = widths.size(); i != e; ++i) width += widths[i];
  size_t min_area = length * width;
  
  // examine the columns from left to right simulating how much screen space
  // would be saved by grouping that column.  If there is a net reduction in
  // screen 'area', then the column will be grouped.  Otherwise it will not.

  // store area of output after simulating grouping at each level successively.
  // the rightmost minimum area will be chosen.
  //
  // For example, consider the following areas after simulating grouping:
  //   areas = [100, 90, 92, 90, 140, 281]
  //
  // With 1 level of grouping and with 3 levels of grouping we get the same
  // screen area, however the rightmost value is chosen, so the return value
  // will be 3.
  vector<size_t> areas(widths.size(), width * length);
  
  string prev;
  for (size_t c = 0, cols = rs -> columns; c < cols; ++c) {
    // test each row in the column to see if it is a duplicate of the previous
    // row.  If so, it will be de-duped in the output.  If not, it means an
    // extra row will be added, so we adjust the new_len variable accordingly.
    prev = "";
    for (size_t r = 0, rows = rs -> rows; r < rows; ++r) {
      if (prev != rs -> data[r][c]) {
        ++length;
        prev = rs -> data[r][c];
      }
    }
    // to calculate the new width, we need to consider both the width of the 
    // grouped column and the width of the remaining columns.  we also need to
    // consider the width of the indent.
    width = max(width - widths[c], widths[c]) + XX * (c + 1);
    min_area = min(length * width, min_area);
    if (c < rs -> columns - 1) areas[c + 1] = width * length;
  }
  // Find the rightmost minimum area from all simulated areas.
  for (size_t c = rs -> columns; c > 0; --c) {
    if (areas[c - 1] == min_area) return c - 1;
  }
  return 0;
}


/**
 * Inserts auto-grouped history, starting with the leftmost columns.
 */
void GroupedFormatter::insert(const ResultSet * rs, ostream & out) const {
  if (!rs) return;  // Sanity check.

  vector<size_t> widths = get_widths(rs, do_show_headings);
  size_t levels = get_grouped_level_count(rs, widths);

  if (do_show_headings) {
    ResultSet::HeadersType::const_iterator h = rs -> headers.begin();
    for (size_t c = 0, cols = rs -> columns; c < cols; ++c) {
      if (c < levels) {
        // if it's a grouped column, print it followed by a newline and an
        // indent for the next line.
        out << *h << "\n";
        for (size_t i = c + 1; i > 0; --i) out << "    ";
      } else {
        // if it's not the last column, we set the alignment left and pad the
        // value with spaces.  Otherwise we just print the value to remove
        // trailing spaces from the last value.
        if (c < cols - 1) {
          out << left << setw(widths[c]) << *h;
        } else {
          out << *h;
        }
      }
      ++h;
    }
    out << endl;
  }
  
  vector<string> prev(levels);
  string value;
  for (size_t r = 0, rows = rs -> rows; r < rows; ++r) {
    for (size_t c = 0, cols = rs -> columns; c < cols; ++c) {
      value = rs -> data[r][c];
      if (c < levels) {
        if (value != prev[c] || r == 0) {
          // The value has not been grouped, 
          out << value;
          if (c < cols - 1) {
            // Since it's not the final column, wrap the line and indent
            // to the next level in preparation for the next value.
            out << "\n";
            for (size_t i = c + 1; i > 0; --i) out << "    ";
            for (size_t i = c; i < levels; ++i) prev[i] = "";
          }
          prev[c] = value;
        } else {
          // The value has been grouped, only print the indent.
          out << "    ";
        }
      } else {
        // Normal (non-grouped) case.
        if (c < cols - 1) {
          out << left << setw(widths[c]) << value;
        } else {
          out << value;
        }
      }
    }
    out << endl;
  }
}

