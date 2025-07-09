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

#include "nullable.h"

#include <type_traits>

namespace sql2xx
{
	template <typename T, typename R = typename T::result_type>
	struct wrapped : T
	{
		wrapped(const T &inner)
			: T(inner)
		{	}
	};

	template <typename T>
	struct parameter
	{
		typedef typename std::remove_cv<T>::type result_type;

		T &object;
	};

	template <typename T, typename F>
	struct column
	{
		typedef F result_type;

		F T::*field;
	};

	template <typename T, typename F>
	struct column< T, nullable<F> >
	{
		typedef F result_type;

		nullable<F> T::* field;
	};

	template <unsigned int table_index, typename T, typename F>
	struct prefixed_column
	{
		typedef F result_type;

		F T::*field;
	};

	template <typename L, typename R>
	struct operator_
	{
		typedef bool result_type;

		L lhs;
		R rhs;
		const char *literal;
	};



	template <typename T>
	inline wrapped<T> wrap(const T &inner)
	{	return wrapped<T>(inner);	}

	template <typename T, typename F>
	inline wrapped< column<T, F> > c(F T::*field)
	{
		column<T, F> c = {	field	};
		return wrap(c);
	}

	template <unsigned int table_index, typename T, typename F>
	inline wrapped< prefixed_column<table_index, T, F> > c(F T::*field)
	{
		prefixed_column<table_index, T, F> pc = {	field	};
		return wrap(pc);
	}

	template <typename T>
	inline wrapped< parameter<T> > p(T &object)
	{
		parameter<T> param = {	object	};
		return wrap(param);
	}

	template <typename L, typename R, typename T>
	inline wrapped< operator_<L, R> > operator ==(const wrapped<L, T> &lhs, const wrapped<R, T> &rhs)
	{
		operator_<L, R> o = {	lhs, rhs, "="	};
		return wrap(o);
	}

	template <typename L, typename R, typename T>
	inline wrapped< operator_<L, R> > operator !=(const wrapped<L, T> &lhs, const wrapped<R, T> &rhs)
	{
		operator_<L, R> o = {	lhs, rhs, "<>"	};
		return wrap(o);
	}

	template <typename L, typename R>
	inline wrapped< operator_<L, R> > operator &&(const wrapped<L, bool> &lhs, const wrapped<R, bool> &rhs)
	{
		operator_<L, R> o = {	lhs, rhs, " AND "	};
		return wrap(o);
	}

	template <typename L, typename R>
	inline wrapped< operator_<L, R> > operator ||(const wrapped<L, bool> &lhs, const wrapped<R, bool> &rhs)
	{
		operator_<L, R> o = {	lhs, rhs, " OR "	};
		return wrap(o);
	}

	template <typename T, typename VisitorT>
	inline void describe(VisitorT &&visitor)
	{	describe(visitor, static_cast<T *>(nullptr));	}
}
