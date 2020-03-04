%{
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
#include "queries.hpp"

#include "logger.hpp"

#include <iostream>
#include <sstream>


// This is implemented at the end of this file, after the syntax definition.
extern int yylex();
extern "C" int yywrap();


namespace ash {
using namespace std;

namespace query {

// Temp variables for parsing.
string * name, * desc, * sql;
int braces = 0;
stringstream * ss;

// The config files to parse for queries.
list<string> files;

// The file currently being parsed.
string parsing;

}  // namespace query


map<string, string> Queries::descriptions;
map<string, string> Queries::queries;


/**
 * Add a query (and description) to the collection of saved queries.
 */
void Queries::add(string & name, string & desc, string & sql) {
  Queries::descriptions[name] = desc;
  Queries::queries[name] = sql;
}


/**
 * Get the set of names and descriptions of all queries.
 */
map<string, string> Queries::get_desc() {
  Queries::lazy_load();
  return Queries::descriptions;
}


/**
 * Return true if the argument query is already defined.
 */
bool Queries::has(const string & name) {
  Queries::lazy_load();
  return
    Queries::queries.find(name) != Queries::queries.end() &&
    Queries::descriptions.find(name) != Queries::descriptions.end();
}


/**
 * Return the description of an argument query.
 */
string Queries::get_desc(const string & name) {
  Queries::lazy_load();
  return Queries::has(name) ? Queries::descriptions[name] : "";
}


/**
 * Return the set of names and SQL for all saved queries.
 */
map<string, string> Queries::get_sql() {
  Queries::lazy_load();
  return Queries::queries;
}


/**
 * Return the query without substituting environment variables.
 */
string Queries::get_raw_sql(const string & name) {
  Queries::lazy_load();
  return Queries::has(name) ? Queries::queries[name] : "";
}


/**
 * Returns the stored query, substituting variables with current values within
 * the query string.
 */
string Queries::get_sql(const string & name) {
  Queries::lazy_load();

  // Sanity check, do nothing if the requested query is not found.
  if (!Queries::has(name)) return "";

  LOG(DEBUG) << "Fetching query: '" << Queries::queries[name] << "'";

  // Eval the query, to expand any and all referenced variables.
  stringstream ss;
  ss << "cat <<EOF_ASH_SQL\n" << Queries::queries[name] << "\nEOF_ASH_SQL";

  FILE * p = popen(ss.str().c_str(), "r");
  if (!p) LOG(FATAL) << "Failed to popen(\"" << ss.str() << "\", \"r\")";

  // Read the query, with all variables substituted.
  ss.str("");
  for (char buffer[1000]; fgets(buffer, sizeof(buffer), p) != 0; ) ss << buffer;

  LOG(DEBUG) << "Query evaluated to: '" << ss.str() << "'";

  // Clean up.
  if (pclose(p) == -1) {
    LOG(FATAL) << "Failed to pclose open stream after reading: " << ss.str();
  }

  // Remove the trailing newline, which was added by the here-document.
  string sql = ss.str();
  return sql.substr(0, sql.size() - 1);
}


/**
 * Loads the configured files looking for saved queries.
 */
void Queries::lazy_load() {
  // Prevent multiple loads, but allow loads for on-demand use.
  static bool loaded = false;
  if (loaded) return;
  loaded = true;

  LOG(DEBUG) << "Loading query files for saved queries.";

  // Load these files, in this order.
  query::files.push_back("/etc/ash/queries");
  query::files.push_back(string(getenv("ASH_CFG_SYSTEM_QUERY_FILE")));
  query::files.push_back(string(getenv("HOME")) + "/.ash/queries");

  // Initialize the input file.
  yyin = 0;
  if (yywrap()) {
    cout << "FAILED TO FIND A FILE TO PARSE!" << endl;
    exit(1);
  }

  // Parse the input file.
  if (yylex()) {
    cout << "FAILED TO PARSE QUERIES!" << endl;
    exit(1);
  }
}


/**
 * Returns an ostream loaded with the filename and line number where a parse
 * error was found.
 */
ostream & fail() {
  cerr << "\nError parsing Advanced Shell History config file:\n"
       << "  " << query::parsing << "\n"
       << "ERROR: Line " << yylineno;
  return cerr;
}


/**
 * Aborts parsing with an error pointing to the erroneous line in the file.
 */
void fail(const char * message) {
  fail() << " - " << message << ".\n" << endl;
  exit(1);
}


/**
 * Aborts parsing with an error pointing to the erroneous line in the file.
 */
void expected(const char * message) {
  fail() << " - Expected " << message << "\n" << endl;
  exit(1);
}

}  // namespace ash
%}

%option nomain
%option nounput
%option yylineno

/**
 * This parser relies on setting states of a finite-state-automata to control
 * parsing and provide (hopefully) useful error messages that point to the
 * exact line of code where a problem is found.
 *
 * States:
 *   INITIAL := Looking for a query definition.
 *   Q1      := Found a query definition, looking for a COLON.
 *   Q2      := Found a query definition and COLON, looking for a LEFT_BRACE.
 *   QUERY   := Found a query definition, colon and left brace, looking for
 *              a description field and a sql field.
 *   D1      := Found a 'definition' keyword, looking for a COLON.
 *   DESC    := Found a 'definition' and COLON, looking for a quoted string.
 *   STR     := Found a double-quote, looking for a closing double quote.
 *   SQL     := Found a 'sql' keyword, looking for a COLON.
 *   SQL1    := Found 'sql:', looking for a LEFT_BRACE.
 *   SQL2    := Found 'sql: {', reading a query and looking for closing '}'.
 *
 * Example Input:
 *   MY_QUERY: {
 *     description: "This is what this query does."
 *     sql: {
 *       select * from foo;
 *     }
 *   }
 */
%x Q1 Q2 QUERY D1 DESC STR SQL SQL1 SQL2

%%
<INITIAL>[ \t\n]+	;  // WHITESPACE
<INITIAL>#.*\n		;  // # LINE COMMENT.
<INITIAL>[a-zA-Z_0-9-]+	{
			  ash::query::desc = ash::query::sql = 0;
			  ash::query::name = new std::string(yytext);
			  BEGIN(Q1);
			}

  /* State Q1 - Read a queary name, expecting a COLON. */
<Q1>#.*			;  // LINE COMMENT.
<Q1>[ \t\n]+		;  // WHITESPACE
<Q1>":"			BEGIN(Q2);
<Q1>[^#: \t\n]+		ash::expected(":");


  /* State Q2 - Read a query name and COLON, expecting an LBRACE. */
<Q2>#.*			;  // LINE COMMENT.
<Q2>[ \t\n]+		;  // WHITESPACE
<Q2>"{"			BEGIN(QUERY);
<Q2>[^#: \t\n]+		ash::expected("{");


  /* State QUERY - Expecting a description and sql definition. */
<QUERY>#.*		;  // LINE COMMENT.
<QUERY>[ \t\n]+		;  // WHITESPACE
<QUERY>"description"	{
			  if (ash::query::desc)
			    ash::fail("multiple descriptions defined");
			  BEGIN(D1);
			}
<QUERY>"sql"		{
			  if (ash::query::sql)
			    ash::fail("multiple sql sections defined");
			  BEGIN(SQL);
			}
<QUERY>"}"		{
			  using namespace ash;
			  using namespace ash::query;

			  // These checks should never be needed.
			  if (!name) expected("a query name for the query.");
			  if (!desc) expected("a description in the query.");
			  if (!sql) expected("a sql field in the query.");

			  Queries::add(*name, *desc, *sql);

			  // Clean up for the next query to be parsed.
			  delete name; delete desc; delete sql;
			  name = desc = sql = 0;
			  BEGIN(INITIAL);
			}

  /* State D1 - Read keyword 'description', expecting a COLON. */
<D1>#.*			;  // LINE COMMENT.
<D1>[ \t\n]+		;  // WHITESPACE
<D1>":"			BEGIN(DESC);
<D1>[^#: \t\n]+		ash::expected(":");

  /* State DESC - Read 'description:' - expecting a quoted string. */
<DESC>#.*		;  // LINE COMMENT.
<DESC>[ \t\n]+		;  // WHITESPACE
<DESC>\"		BEGIN(STR);
<DESC>[^# \t\n\"]	ash::expected("\"");

  /* State STR - Read a quoted string. */
<STR>[^"\n]*\"		{
			  ash::query::desc = new std::string(yytext, yyleng-1);
			  BEGIN(QUERY);
			}
<STR>\n			ash::expected("\" - Multi-line strings are illegal.");

  /* State SQL - read 'sql' token, expecting a COLON. */
<SQL>#.*		;  // LINE COMMENT.
<SQL>[ \t\n]+		;  // WHITESPACE
<SQL>":"		BEGIN(SQL1);

  /* State SQL1 - read 'sql:' token, expecting a LEFT_BRACE. */
<SQL1>#.*		;  // LINE COMMENT.
<SQL1>[ \t\n]+		;  // WHITESPACE
<SQL1>\{[ \t]*	{
			  ash::query::ss = new std::stringstream();
			  BEGIN(SQL2);
			}
<SQL1>.			ash::expected("{");

  /* State SQL2 - read 'sql: {' token, expecting a closing RBRACE */
<SQL2>[^{}]+		*ash::query::ss << yytext;
<SQL2>"{"		{
			  ++ash::query::braces;
			  *ash::query::ss << "{";
			}
<SQL2>"}"		{
			  using namespace ash::query;
			  if (braces) {
			    --braces;
			    *ss << "}";
			  } else {
			    sql = new std::string(ss -> str());
			    delete ss;
			    ss = 0;
			    BEGIN(QUERY);
			  }
			}

  /* FAIL BUCKET - this matches any character that is not covered above. */
.			{
			  ash::fail() << ": Unexpected character." << std::endl;
			  exit(1);
			}
%%


/**
 * This method is called by flex whenever the yyin stream is closed.  This
 * gives us a chance to change the source file and resume parsing.
 */
int yywrap() {
  using namespace ash;
  using namespace std;

  // Reset line number, since we're switching to a new file.
  yyset_lineno(1);

  // Close the old file, if it's actually open.
  if (yyin != 0) fclose(yyin);

  while (!query::files.empty()) {
    query::parsing = query::files.front();
    query::files.pop_front();
    yyin = fopen(query::parsing.c_str(), "r");
    if (yyin) return 0;
    LOG(DEBUG) << "File could not be opened: " << query::parsing;
  }
  LOG(DEBUG) << "Done parsing config files.";
  return 1;
}

