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

#include <cstddef>

namespace sql2xx
{
	template <std::size_t... Is>
	struct index_sequence	{	};

	namespace detail
	{
		template <std::size_t N, std::size_t... Is>
		struct make_index_sequence : make_index_sequence<N - 1, N - 1, Is...>	{	};

		template <std::size_t... Is>
		struct make_index_sequence<0, Is...>
		{	typedef index_sequence<Is...> type;	};
	}

	template <std::size_t N>
	using make_index_sequence = typename detail::make_index_sequence<N>::type;

	template <typename... Ts>
	using index_sequence_for = make_index_sequence<sizeof...(Ts)>;
}
