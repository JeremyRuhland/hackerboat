/******************************************************************************
 * Hackerboat Beaglebone storable state objects
 * Kind of a simple ORM
 * 
 * Version 0.1
 *
 ******************************************************************************/

#include "sqliteStorage.hpp"
#include "stateStructTypes.hpp"
#include "logs.hpp"

#include <sstream>

extern "C" {
#include <err.h>
#include <sqlite3.h>
#include "jansson.h"
}

/* Deleter helper-classes for use with std::shared_ptr<> */

class dbh_deleter {
public:
	void operator() (struct sqlite3 *dbh) {
		sqlite3_close(dbh);
	}
};

class hackerboatStateStorage::sth_deleter {
public:
	void operator() (sqlite3_stmt *sth) {
		sqlite3_finalize(sth);
	}
};

hackerboatStateStorage::hackerboatStateStorage(shared_dbh dbh, const char *tableName, std::initializer_list<column> columns)
	: dbh(dbh), tableName(tableName), columns(columns.begin(), columns.end())
{
}

int hackerboatStateStorage::columnCount() const
{
	return columns.size();
}

void hackerboatStateStorage::appendColumns(std::ostringstream& s, bool comma)
{
	for(auto it = columns.cbegin(); it != columns.cend(); it ++) {
		if (comma) {
			s << ", ";
		} else {
			comma = true;
		}
		s << it->name;
	}
}

void hackerboatStateStorage::prepare(shared_stmt& sth, const std::string& sql)
{
	sqlite3_stmt *prepared_stmt = NULL;
	int err = sqlite3_prepare_v2(dbh.get(), sql.c_str(), sql.length(), &prepared_stmt, NULL);
	if (err != SQLITE_OK) {
		logError();
		abort();
	}
	
	sth.reset(prepared_stmt, sth_deleter());
}

void hackerboatStateStorage::logError(void)
{
#warning Wim - implement me
	warnx("sqlite error (%p, %s): %s",
	      dbh.get(), tableName.c_str(),
	      sqlite3_errmsg(dbh.get()));
	abort();
}

static inline int step(shared_stmt& sth)
{
	return sqlite3_step(sth.get());
}

//******************************************************************************
// Helpers to assemble and prepare SQL statements for various operations.

shared_stmt& hackerboatStateStorage::queryLastRecord()
{
	if (!sth_last) {
		std::ostringstream sql;
		sql << "SELECT oid";
		appendColumns(sql, true);
		sql << " FROM " << tableName << " ORDER BY oid DESC LIMIT 1";
		
		prepare(sth_last, sql.str());
	} else {
		sqlite3_reset(sth_last.get());
	}

	return sth_last;
}

shared_stmt& hackerboatStateStorage::queryRecord()
{
	if (!sth_byOid) {
		std::ostringstream sql;
		sql << "SELECT ";
		appendColumns(sql, false);
		sql << " FROM " << tableName << " WHERE OID = ?";
		
		prepare(sth_byOid, sql.str());
	} else {
		sqlite3_reset(sth_byOid.get());
	}

	return sth_byOid;
}

shared_stmt& hackerboatStateStorage::queryRecordCount()
{
	if (!sth_count) {
		std::ostringstream sql;
		sql << "SELECT COUNT(*) FROM " << tableName;
		prepare(sth_count, sql.str());
	} else {
		sqlite3_reset(sth_count.get());
	}
	
	return sth_count;
}

shared_stmt& hackerboatStateStorage::insertRecord()
{
	if (!sth_insert) {
		std::ostringstream sql;
		sql << "INSERT INTO " << tableName << "(";
		appendColumns(sql, false);
		sql << ") VALUES (";
		int numPlaceholders = columnCount();
		bool comma = false;
		while(numPlaceholders) {
			if (comma)
				sql << ", ";
			sql << "?";
			comma = true;
			numPlaceholders --;
		}
		sql << ")";
		
		prepare(sth_insert, sql.str());
	} else {
		sqlite3_reset(sth_insert.get());
	}

	return sth_insert;
}

shared_stmt& hackerboatStateStorage::updateRecord()
{
	if (!sth_update) {
		std::ostringstream sql;
		sql << "UPDATE " << tableName << " SET ";
		bool comma = false;
		for(auto it = columns.cbegin(); it != columns.cend(); it ++) {
			if (comma) {
				sql << ", ";
			} else {
				comma = true;
			}
			sql << it->name << "=?";
		}
		sql << " WHERE oid = ?";
		
		prepare(sth_update, sql.str());
	} else {
		sqlite3_reset(sth_update.get());
	}

	return sth_update;
}

//******************************************************************************
// Database connection and schema setup

shared_dbh hackerboatStateStorage::databaseConnection(const char *filename)
{
	static shared_dbh dbh;
	if (!dbh) {
		sqlite3 *p;
		if (sqlite3_open("/tmp/foo.sqlite", &p) != SQLITE_OK) {
			errx(1, "Failed to open sqlite database.");
		}
		dbh.reset(p, dbh_deleter());
	}

	return dbh;
}

void hackerboatStateStorage::createTable()
{
	std::ostringstream sql;
	sql << "CREATE TABLE IF NOT EXISTS " << tableName << "(";
	int numColumns = columnCount();
	for(int i = 0; i < numColumns; i++) {
		if (i > 0) {
			sql << ", ";
		}
		sql << columns[i].name << " " << columns[i].type;
	}
	sql << ")";

	int err = sqlite3_exec(dbh.get(), sql.str().c_str(),
			       NULL, NULL,
			       NULL);
	if (err != SQLITE_OK) {
		logError();
		abort();
	}
}

//******************************************************************************
// Methods on storable objects to read and write from the class's hackerboatStateStorage

hackerboatStateClassStorable::sequence hackerboatStateClassStorable::countRecords(void)
{
	hackerboatStateStorage& db = storage();
	shared_stmt sth = db.queryRecordCount();
	int rc = step(sth);
	if (rc != SQLITE_ROW) {
		db.logError();
		return -1;
	} else {
		return sqlite3_column_int64(sth.get(), 0);
	}
}

bool hackerboatStateClassStorable::writeRecord(void)
{
	hackerboatStateStorage& db = storage();
	shared_stmt sth = db.updateRecord();
	int columnCount = db.columnCount();
	assert(sqlite3_bind_parameter_count(sth.get()) == columnCount+1);
	if (!fillRow(sqliteParameterSlice(sth, 1, columnCount)))
		return false;
	sqlite3_bind_int64(sth.get(), columnCount+1, _sequenceNum);
	int rc = step(sth);
	if (rc == SQLITE_DONE) {
		return true;
	} else {
		db.logError();
		return false;
	}
}

bool hackerboatStateClassStorable::appendRecord(void)
{
	hackerboatStateStorage& db = storage();
	shared_stmt sth = db.insertRecord();
	assert(sqlite3_bind_parameter_count(sth.get()) == db.columnCount());
	sqliteParameterSlice parameters(sth, 1, db.columnCount());
	if (!fillRow(parameters))
		return false;
	int rc = step(sth);
	if (rc == SQLITE_DONE) {
		_sequenceNum = sqlite3_last_insert_rowid(db.dbh.get());
		return true;
	} else {
		db.logError();
		return false;
	}
}

bool hackerboatStateClassStorable::getLastRecord(void)
{
	hackerboatStateStorage& db = storage();
	shared_stmt sth = db.queryLastRecord();
	int rc = step(sth);
	if (rc == SQLITE_ROW) {
		return readFromRow(sqliteRowReference(sth, 1, db.columnCount()),
				   sqlite3_column_int64(sth.get(), 0));
	} else if (rc == SQLITE_DONE) {
		/* No rows returned: the table is empty. */
		return false;
	} else {
		db.logError();
		return false;
	}
}

bool hackerboatStateClassStorable::getRecord(sequence oid)
{
	hackerboatStateStorage& db = storage();
	shared_stmt sth = db.queryRecord();
	assert(sqlite3_column_count(sth.get()) == db.columnCount());
	sqlite3_bind_int64(sth.get(), 1, oid);
	int rc = step(sth);
	if (rc == SQLITE_ROW) {
		return readFromRow(sqliteRowReference(sth, 0, db.columnCount()), oid);
	} else if (rc == SQLITE_DONE) {
		/* No rows returned: the table is empty. */
		return false;
	} else {
		db.logError();
		return false;
	}
}

//******************************************************************************
// C++-y helper classes for parameter and row slices

void sqliteParameterSlice::bind_json_new(int column, json_t *j)
{
	assert(column >= 0);
	assert(column < count);
	if (json_is_null(j)) {
		sqlite3_bind_null(sth, column + offset);
	} else {
		char *encoded = json_dumps(j, JSON_COMPACT);
		sqlite3_bind_text(sth, column + offset, encoded, -1, ::free);
	}
	json_decref(j);
}

json_t *sqliteRowReference::json_field(int column) const
{
	assert(column >= 0);
	assert(column < count);
	int coltype = sqlite3_column_type(sth, column + offset);
	if (coltype == SQLITE_NULL) {
		return json_null();
	}
	int length = sqlite3_column_bytes(sth, column + offset);
	const unsigned char *contents = sqlite3_column_text(sth, column + offset);
	return json_loadb(reinterpret_cast<const char *>(contents), length, 0, NULL);
}

std::string sqliteRowReference::string_field(int column) const
{
	assert(column >= 0);
	assert(column < count);
	if (sqlite3_column_type(sth, column + offset) == SQLITE_NULL) {
		// ...;
	}
	int length = sqlite3_column_bytes(sth, column + offset);
	const unsigned char *contents = sqlite3_column_text(sth, column + offset);
	return std::string(reinterpret_cast<const char *>(contents), length);
}

#if 0

bool hackerboatStateClassStorable::openFile()
{
	if (!_db) {
		sqlite3 *dbh = NULL;
		int err = sqlite3_open_v2(_dbPath.c_str(), &dbh,
					  SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
					  NULL);
		if (err != SQLITE_OK) {
			logError::instance()->write("sqlite3", "cannot open db");
			if (dbh) {
				sqlite3_close(dbh);
				dbh = NULL;
			}
			return false;
		} else {
			_db.reset(dbh, dbh_deleter());
		}
	}

	return true;
}

#endif
