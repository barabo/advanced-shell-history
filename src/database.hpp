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

#ifndef __ASH_DATABASE__
#define __ASH_DATABASE__


#include <list>
#include <map>
#include <string>
#include <vector>

using std::list;
using std::map;
using std::string;
using std::vector;

class sqlite3;  // Forward declaration.
class sqlite3_stmt;  // Forward declaration.

namespace ash {

class Database;  // Forward declaration.
class DBObject;  // Forward declaration.


/**
 * This is the result of a query that selects multiple rows.
 */
class ResultSet {
  public:
    typedef list<string> HeadersType;
    typedef vector<string> RowType;
    typedef vector<RowType> DataType;

  public:
    ~ResultSet() {}

  private:
    ResultSet(const HeadersType & headers, const DataType & data);

  public:
    const HeadersType headers;
    const DataType data;
    const size_t rows, columns;

  // DISALLOWED:
  private:
    ResultSet(const ResultSet & other);  // disallowed.
    ResultSet & operator = (const ResultSet & other);  // disallowed.

  friend class Database;
};


/**
 * This class abstracts a backing sqlite3 database.
 */
class Database {
  public:
    Database(const string & filename);
    virtual ~Database();

    ResultSet * exec(const string & query, const int limit=0) const;

    long int insert(DBObject * object) const;

    void init_db();

  private:
    sqlite3_stmt * prepare_stmt(const string & query) const;

  private:
    const string db_filename;
    sqlite3 * db;
};


/* abstract */
class DBObject {
  // STATIC:
  public:
    static const string quote(const char * value);
    static const string quote(const string & value);
    static const string get_create_tables();

  protected:
    static void register_table(const string & name, const string & sql);

  protected:
    static list<string> create_tables;
    static vector<string> table_names;

  // NON-STATIC:
  protected:
    DBObject();
    virtual ~DBObject();

    virtual const string get_name() const = 0;  // abstract
    virtual const string get_sql() const;

    map<string, string> values;

  // DISALLOWED:
  private:
    DBObject(const DBObject & other);  // disabled
    DBObject & operator =(const DBObject & other);  // disabled

  friend class Database;
};


}  // namespace ash

#endif  /* __ASH_DATABASE__ */

