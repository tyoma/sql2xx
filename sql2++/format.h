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

#include "expression.h"
#include "nullable.h"
#include "types.h"

#include <cstdint>
#include <list>
#include <string>
#include <tuple>
#include <vector>

namespace sql2xx
{
	struct table_source_visitor
	{
		template <typename U>
		void operator ()(U &table_name_)
		{	table_name.append(table_name_);	}

		template <typename U, typename V>
		void operator ()(U, V) const
		{	}

		template <typename TagT, typename U, typename V>
		void operator ()(TagT, U, V) const
		{	}

		template <typename TagT>
		nil_stream operator <<(TagT) const
		{	return nil_stream();	}

		std::string &table_name;
	};

	struct select_list_visitor
	{
		template <typename U>
		void operator ()(U)
		{	}

		template <typename F>
		void operator ()(F /*field*/, const char *name)
		{
			if (!first)
				table_name += ",";
			table_name += prefix;
			table_name += name;
			first = false;
		}

		template <typename TagT, typename F>
		void operator ()(TagT /*tag*/, F field, const char *name)
		{	(*this)(field, name);	}

		const char *prefix;
		std::string &table_name;
		bool first;
	};


	template <typename T>
	struct fields_collector
	{
		template <typename U>
		fields_collector<T> operator <<(U T::*field) const;

		std::vector<std::string> &columns;
	};


	template <typename T>
	struct column_definition_format_visitor
	{
		template <typename U>
		void operator ()(U)
		{	}

		void operator ()(int T::*, const char *column_name)
		{	append_integer(column_name); not_null();	}

		void operator ()(unsigned int T::*, const char *column_name)
		{	append_integer(column_name); not_null();	}

		void operator ()(std::int64_t T::*, const char *column_name)
		{	append_integer(column_name); not_null();	}

		void operator ()(std::uint64_t T::*, const char *column_name)
		{	append_integer(column_name); not_null();	}

		void operator ()(std::string T::*, const char *column_name)
		{	append_text(column_name); not_null();	}

		void operator ()(double T::*, const char *column_name)
		{	append_real(column_name); not_null();	}

		void operator ()(nullable<int> T::*, const char *column_name)
		{	append_integer(column_name);	}

		void operator ()(nullable<unsigned int> T::*, const char *column_name)
		{	append_integer(column_name);	}

		void operator ()(nullable<std::int64_t> T::*, const char *column_name)
		{	append_integer(column_name);	}

		void operator ()(nullable<std::uint64_t> T::*, const char *column_name)
		{	append_integer(column_name);	}

		void operator ()(nullable<std::string> T::*, const char *column_name)
		{	append_text(column_name);	}

		void operator ()(nullable<double> T::*, const char *column_name)
		{	append_real(column_name);	}

		template <typename U>
		void operator ()(identity_tag, U T::* field, const char *column_name)
		{
			(*this)(field, column_name);
			column_definitions += " PRIMARY KEY ASC";
		}

		fields_collector<T> operator <<(unique_tag)
		{
			fields_collector<T> collector = {
				std::get<1>(*constraints.insert(constraints.end(), std::make_tuple("UNIQUE", std::vector<std::string>())))
			};
			return collector;
		}

		fields_collector<T> operator <<(primary_key_tag)
		{
			fields_collector<T> collector = {
				std::get<1>(*constraints.insert(constraints.end(), std::make_tuple("PRIMARY KEY", std::vector<std::string>())))
			};
			return collector;
		}

		std::string &column_definitions;
		std::list< std::tuple< std::string, std::vector<std::string> > > &constraints;
		bool first;
		bool is_nullable;

	private:
		std::string &append_column(const char *column_name)
		{
			if (!first)
				column_definitions += ',';
			first = false;
			column_definitions.append(column_name);
			return column_definitions;
		}

		void not_null()
		{	column_definitions += " NOT NULL";	}

		void append_integer(const char *column_name)
		{	append_column(column_name) += " INTEGER";	}

		void append_text(const char *column_name)
		{	append_column(column_name) += " TEXT";	}

		void append_real(const char *column_name)
		{	append_column(column_name) += " REAL";	}
	};


	template <typename T, typename F>
	struct format_column_visitor
	{
		template <typename U>
		void operator ()(F U::*field_, const char *column_name_) const
		{
			if (field_ == field)
				column_name->append(column_name_);
		}

		template <typename TagT, typename U>
		void operator ()(TagT, U T::*field, const char *column_name_)
		{	(*this)(field, column_name_);	}

		template <typename U>
		void operator ()(U)
		{	}

		template <typename U> 
		void operator ()(U, const char *) const
		{	}

		template <typename TagT>
		nil_stream operator <<(TagT) const
		{	return nil_stream();	}

		F T::*field;
		std::string *column_name;
	};



	template <typename T>
	inline std::string default_table_name()
	{
		std::string r;
		table_source_visitor v = {	r	};

		describe<T>(v);
		return r;
	}

	template <unsigned int table_index> inline void table_alias(std::string &output);
	template <> inline void table_alias<0>(std::string &output) {	output += "t0";	}
	template <> inline void table_alias<1>(std::string &output) {	output += "t1";	}
	template <> inline void table_alias<2>(std::string &output) {	output += "t2";	}
	template <> inline void table_alias<3>(std::string &output) {	output += "t3";	}

	template <typename T>
	inline void format_table_source(std::string &output, T *)
	{
		table_source_visitor v = {	output	};

		describe<T>(v);
	}

	template <typename T1, typename T2>
	inline void format_table_source(std::string &output, std::tuple<T1, T2> *)
	{
		table_source_visitor v = {	output	};

		describe<T1>(v), output += " AS ", table_alias<0>(output), output += ",";
		describe<T2>(v), output += " AS ", table_alias<1>(output);
	}

	template <typename T1, typename T2, typename T3>
	inline void format_table_source(std::string &output, std::tuple<T1, T2, T3> *)
	{
		table_source_visitor v = {	output	};

		describe<T1>(v), output += " AS ", table_alias<0>(output), output += ",";
		describe<T2>(v), output += " AS ", table_alias<1>(output), output += ",";
		describe<T3>(v), output += " AS ", table_alias<2>(output);
	}


	template <typename T>
	inline void format_select_list(std::string &output, T *)
	{
		select_list_visitor v = {	"", output, true	};

		describe<T>(v);
	}

	template <typename T1, typename T2>
	inline void format_select_list(std::string &output, std::tuple<T1, T2> *)
	{
		select_list_visitor v0 = {	"t0.", output, true	}, v1 = {	"t1.", output, false	};

		describe<T1>(v0);
		describe<T2>(v1);
	}

	template <typename T1, typename T2, typename T3>
	inline void format_select_list(std::string &output, std::tuple<T1, T2, T3> *)
	{
		select_list_visitor v0 = {	"t0.", output, true	}, v1 = {	"t1.", output, false	},
			v2 = {	"t2.", output, false	};

		describe<T1>(v0);
		describe<T2>(v1);
		describe<T3>(v2);
	}


	template <typename T, typename F>
	inline void format_expression(std::string &output, const column<T, F> &e, unsigned int &/*index*/)
	{
		format_column_visitor<T, F> v = {	e.field, &output	};
		describe<T>(v);
	}

	template <unsigned int table_index, typename T, typename F>
	inline void format_expression(std::string &output, const prefixed_column<table_index, T, F> &e, unsigned int &/*index*/)
	{
		format_column_visitor<T, F> v = {	e.field, &output	};
		table_alias<table_index>(output), output += ".", describe<T>(v);
	}

	template <typename T>
	inline void format_expression(std::string &output, const parameter<T> &/*e*/, unsigned int &index)
	{
		output += ':';
		output += std::to_string((unsigned long long)index++);
	}

	template <typename L, typename R>
	inline void format_expression(std::string &output, const operator_<L, R> &e, unsigned int &index)
	{
		format_expression(output, e.lhs, index);
		output += e.literal;
		format_expression(output, e.rhs, index);
	}

	template <typename E>
	inline void format_expression(std::string &output, const E &e)
	{
		auto index = 1u;

		format_expression(output, e, index);
	}

	template <typename T>
	inline void format_create_table(std::string &output, const char *name)
	{
		std::list< std::tuple< std::string, std::vector<std::string> > > constraints;
		column_definition_format_visitor<T> v = {	output, constraints, true, false	};

		output += "CREATE TABLE ";
		output += name;
		output += " (";
		describe<T>(v);
		std::for_each(std::begin(constraints), std::end(constraints), [&] (const auto &c) {
			auto &columns = std::get<1>(c);
			auto first_column = true;

			output += ",";
			output += std::get<0>(c);
			output += "(";
			std::for_each(std::begin(columns), std::end(columns), [&] (const auto &col) {
				if (!first_column)
					output += ",";
				output += col;
				first_column = false;
			});
			output += ")";
		});
		output += ")";
	}

	template <typename T>
	template <typename U>
	inline fields_collector<T> fields_collector<T>::operator <<(U T::*field) const
	{
		std::string name;
		format_column_visitor<T, U> v = { field, &name };

		describe<T>(v);
		columns.emplace_back(name);
		return *this;
	}
}
