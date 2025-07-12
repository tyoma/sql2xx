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

#include "binding.h"
#include "statement.h"

namespace sql2xx
{
	template <typename T>
	class inserter : statement
	{
	public:
		inserter(sqlite3 &connection, statement_ptr &&statement);

		template <typename T2>
		void operator ()(T2 &item);

	private:
		sqlite3 &_connection;
	};



	template <typename T>
	inline inserter<T>::inserter(sqlite3 &connection, statement_ptr &&statement_)
		: statement(std::move(statement_)), _connection(connection)
	{	}

	template <typename T>
	template <typename T2>
	inline void inserter<T>::operator ()(T2 &item)
	{
		bind_fields<T>(*this, item);
		execute();
		bind_identity<T>(_connection, item);
		reset();
	}
}
