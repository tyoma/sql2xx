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
#include "format.h"
#include "visitor.h"

namespace sql2xx
{
	class updater : statement
	{
	public:
		updater(statement_ptr &&statement_, const std::function<void (statement &statement_)> &update_bindings);

		using statement::execute;
		void reset();

	private:
		const std::function<void (statement &statement_)> _update_bindings;
	};


	template <typename T>
	class update_builder
	{
	public:
		template <typename W, typename FieldT, typename U, typename ValueT, typename... RestT>
		update_builder(const W &where, FieldT U::* field, const ValueT &value, RestT &&... rest);

		updater create(sqlite3 &database) const;

	private:
		template <typename FieldT, typename U, typename ValueT, typename... RestT>
		void initialize(FieldT U::* field, const ValueT &value, RestT &&... rest);

		template <typename FieldT, typename U, typename ValueT>
		void initialize(FieldT U::* field, const ValueT &value);

		template <typename FieldT, typename U, typename ValueT>
		void add_set_expression(int index, FieldT U::* field, const ValueT &value);

	private:
		std::string _expression;
		std::function<void (statement &statement_)> _update_bindings;
		unsigned int _binding_index;
	};



	inline updater::updater(statement_ptr &&statement_,
			const std::function<void (statement &statement_)> &update_bindings)
		: statement(std::move(statement_)), _update_bindings(update_bindings)
	{	_update_bindings(*this);	}

	inline void updater::reset()
	{
		statement::reset();
		_update_bindings(*this);
	}


	template <typename T>
	template <typename W, typename FieldT, typename U, typename ValueT, typename... RestT>
	inline update_builder<T>::update_builder(const W &where, FieldT U::* field, const ValueT &value, RestT &&... rest)
		: _expression("UPDATE " + default_table_name<T>() + " SET "), _update_bindings([] (statement &) {}),
			_binding_index(1)
	{
		initialize(field, value, std::forward<RestT>(rest)...);
		_expression += " WHERE ";

		const auto where_index = _binding_index;
		auto previous = _update_bindings;

		format_expression(_expression, where, _binding_index);
		_update_bindings = [where, where_index, previous] (statement &statement_) {
			auto index = where_index;

			previous(statement_);
			sql2xx::bind_parameters(statement_, where, index);
		};
	}

	template <typename T>
	inline updater update_builder<T>::create(sqlite3 &database) const
	{	return updater(sql2xx::create_statement(database, _expression.c_str()), _update_bindings);	}

	template <typename T>
	template <typename FieldT, typename U, typename ValueT, typename... RestT>
	inline void update_builder<T>::initialize(FieldT U::* field, const ValueT &value, RestT &&... rest)
	{
		add_set_expression(_binding_index++, field, value);
		_expression += ',';
		initialize(std::forward<RestT>(rest)...);
	}

	template <typename T>
	template <typename FieldT, typename U, typename ValueT>
	inline void update_builder<T>::initialize(FieldT U::* field, const ValueT &value)
	{	add_set_expression(_binding_index++, field, value);	}

	template <typename T>
	template <typename FieldT, typename U, typename ValueT>
	inline void update_builder<T>::add_set_expression(int index, FieldT U::* field, const ValueT &value)
	{
		auto p = _update_bindings;

		_update_bindings = [&value, p, index] (statement &statement_) {
			p(statement_);
			statement_.bind(index, value);
		};
		format_column(_expression, c(field));
		_expression += "=:";
		_expression += std::to_string(index);
	}
}
