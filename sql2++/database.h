//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#pragma once

#include "insert.h"
#include "remove.h"
#include "select.h"
#include "update.h"
#include "visitor.h"

namespace sql2xx
{
	struct sql_error : std::runtime_error
	{
		sql_error(const std::string &text);

		static void check_step(const connection_ptr &connection, int step_result);
	};

	class transaction
	{
	public:
		enum type {	deferred, immediate, exclusive,	};

	public:
		transaction(connection_ptr connection, type type_ = deferred, int timeout_ms = 500);
		~transaction();

		template <typename T>
		void create_table();

		template <typename T>
		reader<T> select();

		template <typename T, typename T2, typename R, typename... OrderT>
		reader<T> select(const wrapped<T2, R> &where, OrderT&&... order);

		template <typename T>
		std::size_t count();

		template <typename T, typename W>
		std::size_t count(const W &where);

		template <typename T>
		inserter<T> insert();

		template <typename T>
		inserter<T> upsert();

		template <typename T, typename W, typename FieldT, typename U, typename ValueT, typename... RestT>
		updater update(const W& where, FieldT U::*field, const ValueT &value, RestT &&...);

		template <typename T, typename W>
		remover remove(const W &where);

		void commit();

	private:
		void execute(const char *sql_statemet);

		template <typename T>
		std::string create_insert_statement();

	private:
		connection_ptr _connection;
		bool _comitted;
	};



	inline transaction::transaction(connection_ptr connection, type type_, int timeout_ms)
		: _connection(connection), _comitted(false)
	{
		const char *begin_sql[] = {	"BEGIN DEFERRED", "BEGIN IMMEDIATE", "BEGIN EXCLUSIVE",	};

		sqlite3_busy_timeout(_connection.get(), timeout_ms);
		execute(begin_sql[type_]);
	}

	inline transaction::~transaction()
	{
		try
		{
			if (!_comitted)
				execute("ROLLBACK");
		}
		catch (...)
		{  }
	}

	template <typename T>
	inline void transaction::create_table()
	{
		std::string create_table_ddl;

		format_create_table<T>(create_table_ddl, default_table_name<T>().c_str());
		execute(create_table_ddl.c_str());
	}

	template <typename T>
	inline reader<T> transaction::select()
	{	return select_builder<T>().create_reader(*_connection);	}

	template <typename T, typename T2, typename R, typename... OrderT>
	inline reader<T> transaction::select(const wrapped<T2, R> &where, OrderT&&... order)
	{	return select_builder<T>().create_reader(*_connection, where, std::forward<OrderT>(order)...);	}

	template <typename T>
	std::size_t transaction::count()
	{
		std::string expression_text = "SELECT COUNT(*) FROM ";

		format_table_source(expression_text, static_cast<T *>(nullptr));

		statement stmt(create_statement(*_connection, expression_text.c_str()));

		stmt.execute();
		return static_cast<std::size_t>(static_cast<std::uint64_t>(stmt.get(0)));
	}

	template <typename T, typename W>
	std::size_t transaction::count(const W &where)
	{
		std::string expression_text = "SELECT COUNT(*) FROM ";

		format_table_source(expression_text, static_cast<T *>(nullptr));
		expression_text += " WHERE ";
		format_expression(expression_text, where);

		statement stmt(create_statement(*_connection, expression_text.c_str()));

		bind_parameters(stmt, where);
		stmt.execute();
		return static_cast<std::size_t>(static_cast<std::uint64_t>(stmt.get(0)));
	}


	template <typename T>
	inline inserter<T> transaction::insert()
	{	return inserter<T>(*_connection, create_statement(*_connection, create_insert_statement<T>().c_str()));	}

	template <typename T>
	inline inserter<T> transaction::upsert()
	{
		auto request = create_insert_statement<T>() + " ON CONFLICT DO UPDATE SET ";

		describe<T>(collect_regular_field_names([&] (const char *name, bool first) {
			if (!first)
				request += ',';
			request += name;
			request += "=EXCLUDED.";
			request += name;
		}));
		return inserter<T>(*_connection, create_statement(*_connection, request.c_str()));
	}

	template <typename T, typename W, typename FieldT, typename U, typename ValueT, typename... RestT>
	inline updater transaction::update(const W &where, FieldT U:: *field, const ValueT &value, RestT &&... rest)
	{	return update_builder<T>(where, field, value, std::forward<RestT>(rest)...).create(*_connection);	}

	template <typename T, typename W>
	inline remover transaction::remove(const W &where)
	{	return remove_builder(default_table_name<T>().c_str()).create_statement(*_connection, where);	}

	inline void transaction::commit()
	{
		execute("COMMIT");
		_comitted = true;
	}

	inline void transaction::execute(const char *sql_statemet)
	try
	{
		statement stmt(create_statement(*_connection, sql_statemet));

		stmt.execute();
	}
	catch (execution_error &e)
	{
		sql_error::check_step(_connection, e.code);
	}

	template <typename T>
	inline std::string transaction::create_insert_statement()
	{
		auto request = "INSERT INTO " + default_table_name<T>() + " (";

		describe<T>(collect_regular_field_names([&] (const char *name, bool first) {
			if (!first)
				request += ',';
			request += name;
		}));
		request += ") VALUES (";
		describe<T>(collect_regular_field_names([&] (const char *, bool first) {
			if (!first)
				request += ',';
			request += '?';
		}));
		request += ") ";
		return request;
	}


	inline sql_error::sql_error(const std::string &text)
		: std::runtime_error(text)
	{	}

	inline void sql_error::check_step(const connection_ptr &connection, int /*step_result*/)
	{
		std::string text = "SQLite error: ";
			
		text += sqlite3_errmsg(connection.get());
		text += "!";
		throw sql_error(text);
	}
}
