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

#include "types.h"

namespace sql2xx
{
	template <typename F>
	class identity_field_names_visitor
	{
	public:
		identity_field_names_visitor(F &callback)
			: _first(true), _callback(callback)
		{	}

		void operator ()(const char * /*table_name*/)
		{	}

		template <typename FieldT, typename BaseT>
		void operator ()(FieldT BaseT:: *field, const char *name)
		{	}

		template <typename FieldT, typename BaseT>
		void operator ()(identity_tag, FieldT BaseT:: *field, const char *name)
		{
			_callback(name, _first);
			_first = false;
		}

		template <typename T>
		identity_field_names_visitor operator <<(T) const
		{	return *this;	}

	private:
		bool _first;
		F &_callback;
	};


	template <typename F>
	class regular_field_names_visitor
	{
	public:
		regular_field_names_visitor(F &callback)
			: _first(true), _callback(callback)
		{	}

		void operator ()(const char * /*table_name*/)
		{	}

		template <typename FieldT, typename BaseT>
		void operator ()(FieldT BaseT:: *field, const char *name)
		{
			_callback(name, _first);
			_first = false;
		}

		template <typename FieldT, typename BaseT>
		void operator ()(identity_tag, FieldT BaseT:: *field, const char *name)
		{	}

		template <typename T>
		regular_field_names_visitor operator <<(T) const
		{	return *this;	}

	private:
		bool _first;
		F &_callback;
	};


	template <typename F>
	class all_field_names_visitor
	{
	public:
		all_field_names_visitor(F &callback)
			: _first(true), _callback(callback)
		{	}

		void operator ()(const char * /*table_name*/)
		{	}

		template <typename FieldT, typename BaseT>
		void operator ()(FieldT BaseT:: *field, const char *name)
		{
			_callback(name, _first);
			_first = false;
		}

		template <typename FieldT, typename BaseT>
		void operator ()(identity_tag, FieldT BaseT:: *field, const char *name)
		{
			_callback(name, _first);
			_first = false;
		}

		template <typename T>
		all_field_names_visitor operator <<(T) const
		{	return *this;	}

	private:
		bool _first;
		F &_callback;
	};



	template <typename F>
	inline identity_field_names_visitor<F> collect_identity_field_names(F &callback)
	{	return identity_field_names_visitor<F>(callback);	}

	template <typename F>
	inline regular_field_names_visitor<F> collect_regular_field_names(F &callback)
	{	return regular_field_names_visitor<F>(callback);	}

	template <typename F>
	inline all_field_names_visitor<F> collect_all_field_names(F &callback)
	{	return all_field_names_visitor<F>(callback);	}
}
