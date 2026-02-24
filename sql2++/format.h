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
#include "visitor.h"

#include <algorithm>
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


	template <typename T>
	struct fields_collector
	{
		template <typename U>
		fields_collector<T> operator <<(U T::*field) const;

		std::vector<std::string> &columns;
	};


	struct foreign_key_constraint
	{
		std::string referred_table;
		std::vector< std::tuple<std::string, std::string> > column_pairs; // (fk-column-1, pk-column-1), ...
	};


	template <typename T, typename ReferredT>
	struct fk_pk_fields_collector;

	template <typename T, typename ReferredT>
	struct fk_fields_collector
	{
		template <typename U>
		fk_pk_fields_collector<T, ReferredT> operator <<(U T::* field) const;

		foreign_key_constraint &constraint;
	};

	template <typename T, typename ReferredT>
	struct fk_pk_fields_collector
	{
		template <typename U, typename ReferredT2>
		fk_fields_collector<T, ReferredT> operator <<(U ReferredT2::* field) const;

		foreign_key_constraint &constraint;
	};


	template <typename T>
	inline std::string default_table_name();


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

		template <typename F>
		void operator ()(identity_tag, F field, const char *column_name)
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

		template <typename ReferredT>
		fk_fields_collector<T, ReferredT> operator <<(void (*)(ReferredT))
		{
			auto &constraint = *foreign_key_constraints.insert(foreign_key_constraints.end(), foreign_key_constraint());

			constraint.referred_table = default_table_name<ReferredT>();

			fk_fields_collector<T, ReferredT> v = { constraint };

			return v;
		}

		std::string &column_definitions;
		std::list< std::tuple< std::string, std::vector<std::string> > > &constraints;
		std::list<foreign_key_constraint> &foreign_key_constraints;
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

		template <typename TagT, typename F2>
		void operator ()(TagT, F2 field, const char *column_name_)
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
		describe<T>(collect_all_field_names([&] (const char *name, bool first) {
			if (!first)
				output += ',';
			output += name;
		}));
	}

	template <typename T1, typename T2>
	inline void format_select_list(std::string &output, std::tuple<T1, T2> *)
	{
		describe<T1>(collect_all_field_names([&] (const char *name, bool first) {
			if (!first)
				output += ',';
			output += "t0."; output += name;
		}));
		describe<T2>(collect_all_field_names([&] (const char *name, bool /*first*/) {
			output += ",t1."; output += name;
		}));
	}

	template <typename T1, typename T2, typename T3>
	inline void format_select_list(std::string &output, std::tuple<T1, T2, T3> *)
	{
		describe<T1>(collect_all_field_names([&] (const char *name, bool first) {
			if (!first)
				output += ',';
			output += "t0."; output += name;
		}));
		describe<T2>(collect_all_field_names([&] (const char *name, bool /*first*/) {
			output += ",t1."; output += name;
		}));
		describe<T3>(collect_all_field_names([&] (const char *name, bool /*first*/) {
			output += ",t2."; output += name;
		}));
	}


	template <typename T, typename F>
	inline void format_column(std::string &output, const column<T, F> &e)
	{
		format_column_visitor<T, F> v = {	e.field, &output	};
		describe<T>(v);
	}

	template <unsigned int table_index, typename T, typename F>
	inline void format_column(std::string &output, const prefixed_column<table_index, T, F> &e)
	{
		format_column_visitor<T, F> v = {	e.field, &output	};
		table_alias<table_index>(output), output += ".", describe<T>(v);
	}

	template <typename T, typename F>
	inline void format_expression(std::string &output, const column<T, F> &e, unsigned int &/*index*/)
	{	format_column(output, e);	}

	template <unsigned int table_index, typename T, typename F>
	inline void format_expression(std::string &output, const prefixed_column<table_index, T, F> &e, unsigned int &/*index*/)
	{	format_column(output, e);	}

	template <typename T>
	inline void format_expression(std::string &output, const parameter<T> &/*e*/, unsigned int &index)
	{
		output += ':';
		output += std::to_string((unsigned long long)index++);
	}

	template <typename L, typename R>
	inline void format_expression(std::string &output, const binary_operator<L, R> &e, unsigned int &index)
	{
		output += '(';
		format_expression(output, e.lhs, index);
		output += e.literal;
		format_expression(output, e.rhs, index);
		output += ')';
	}

	template <typename U>
	inline void format_expression(std::string &output, const unary_operator<U> &e, unsigned int &index)
	{
		output += e.literal_prefix;
		format_expression(output, e.operand, index);
		output += e.literal_postfix;
	}

	template <typename T, typename R>
	inline void format_expression(std::string &output, const wrapped<T, R> &e)
	{
		auto index = 1u;

		format_expression(output, e, index);
	}

	template <typename ColT>
	inline void format_order_one(std::string &output, const ColT &col, bool ascending)
	{
		format_column(output, col);
		output += ascending ? " ASC" : " DESC";
	}

	inline void format_order_next(std::string &/*output*/)
	{	}

	template <typename ColT, typename... RestT>
	inline void format_order_next(std::string &output, const ColT &col, bool ascending, RestT&&... args)
	{
		output += ",";
		format_order_one(output, col, ascending);
		format_order_next(output, std::forward<RestT>(args)...);
	}

	inline void format_order(std::string &/*output*/)
	{	}

	template <typename ColT, typename... RestT>
	inline void format_order(std::string &output, const ColT &col, bool ascending, RestT&&... args)
	{
		output += " ORDER BY ";
		format_order_one(output, col, ascending);
		format_order_next(output, std::forward<RestT>(args)...);
	}


	template <typename T>
	inline void format_create_table(std::string &output, const char *name)
	{
		typedef std::tuple< std::string, std::vector<std::string> > constraint_t;

		std::list<constraint_t> constraints;
		std::list<foreign_key_constraint> fk_constraints;
		column_definition_format_visitor<T> v = {	output, constraints, fk_constraints, true, false	};

		output += "CREATE TABLE ";
		output += name;
		output += " (";
		describe<T>(v);
		std::for_each(std::begin(constraints), std::end(constraints), [&] (const constraint_t &c) {
			auto &columns = std::get<1>(c);
			auto first_column = true;

			output += ",";
			output += std::get<0>(c);
			output += "(";
			for (auto i = std::begin(columns); i != std::end(columns); ++i, first_column = false)
			{
				if (!first_column)
					output += ",";
				output += *i;
			}
			output += ")";
		});
		std::for_each(std::begin(fk_constraints), std::end(fk_constraints), [&] (const foreign_key_constraint &c) {
			auto first_column = true;

			output += ",FOREIGN KEY(";
			for (auto i = std::begin(c.column_pairs); i != std::end(c.column_pairs); ++i, first_column = false)
			{
				if (!first_column)
					output += ",";
				output += std::get<0>(*i);
			}
			output += ") REFERENCES ";
			output += c.referred_table;
			output += "(";
			first_column = true;
			for (auto i = std::begin(c.column_pairs); i != std::end(c.column_pairs); ++i, first_column = false)
			{
				if (!first_column)
					output += ",";
				output += std::get<1>(*i);
			}
			output += ") ON DELETE CASCADE";
		});
		output += ")";
	}

	template <typename T>
	template <typename U>
	inline fields_collector<T> fields_collector<T>::operator <<(U T::*field) const
	{
		std::string name;

		format_column(name, c(field));
		columns.emplace_back(name);
		return *this;
	}

	template <typename T, typename ReferredT>
	template <typename U>
	inline fk_pk_fields_collector<T, ReferredT> fk_fields_collector<T, ReferredT>::operator <<(U T::* field) const
	{
		std::string name;

		format_column(name, c(field));
		constraint.column_pairs.emplace_back(std::make_tuple(name, std::string()));

		fk_pk_fields_collector<T, ReferredT> next = {	constraint	};

		return next;
	}

	template <typename T, typename ReferredT>
	template <typename U, typename ReferredT2>
	inline fk_fields_collector<T, ReferredT> fk_pk_fields_collector<T, ReferredT>::operator <<(U ReferredT2::* field) const
	{
		ReferredT2 *check = static_cast<ReferredT *>(nullptr);

		std::string name;
		format_column_visitor<ReferredT, U> v = {	field, &name	};

		describe<ReferredT>(v);
		std::get<1>(constraint.column_pairs.back()) = name;

		fk_fields_collector<T, ReferredT> next = {	constraint	};

		return next;
	}
}
