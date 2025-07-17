#pragma once

#include <sql2++/database.h>
#include <vector>

namespace sql2xx
{
	namespace tests
	{
		struct plural_
		{
			template <typename T>
			std::vector<T> operator +(const T &rhs) const
			{	return std::vector<T>(1, rhs);	}
		} const plural;

		template <typename T>
		inline std::vector<T> operator +(std::vector<T> lhs, const T &rhs)
		{	return lhs.push_back(rhs), lhs;	}

		template <typename T, typename F1, typename F2>
		inline T initialize(const F1 &field1, const F2 &field2)
		{
			T value = {	field1, field2,	};
			return value;
		}

		template <typename T, typename F1, typename F2, typename F3>
		inline T initialize(const F1 &field1, const F2 &field2, const F3 &field3)
		{
			T value = {	field1, field2, field3,	};
			return value;
		}

		template <typename T, typename F1, typename F2, typename F3, typename F4>
		inline T initialize(const F1 &field1, const F2 &field2, const F3 &field3, const F4 &field4)
		{
			T value = {	field1, field2, field3, field4,	};
			return value;
		}

		template <typename T, typename F1, typename F2, typename F3, typename F4, typename F5>
		inline T initialize(const F1 &field1, const F2 &field2, const F3 &field3, const F4 &field4, const F5 &field5)
		{
			T value = {	field1, field2, field3, field4, field5,	};
			return value;
		}

		template <typename T>
		std::vector<T> read_all(reader<T> &&reader)
		{
			T item;
			std::vector<T> result;

			while (reader(item))
				result.push_back(item);
			return result;
		}

		template <typename T>
		std::vector<T> read_all(transaction &tx)
		{	return read_all(tx.select<T>());	}

		template <typename T>
		std::vector<T> read_all(std::string path)
		{
			transaction tx(create_connection(path.c_str()));
			return read_all<T>(tx);
		}

		template <typename T>
		void write_all(transaction &tx, std::vector<T> &records)
		{
			auto w = tx.insert<T>();

			for (auto i = records.begin(); i != records.end(); ++i)
				w(*i);
		}

		template <typename T>
		void write_all(transaction &tx, const std::vector<T> &records, const char *table_name)
		{
			auto w = tx.insert<T>(table_name);

			for (auto i = records.begin(); i != records.end(); ++i)
				w(*i);
		}

		template <typename T>
		void write_all(std::string path, const std::vector<T> &records, const char *table_name)
		{
			transaction tx(create_connection(path.c_str()));
			write_all(tx, records, table_name);
		}
	}

	template <typename T>
	inline bool operator <(const nullable<T> &lhs, const nullable<T> &rhs)
	{
		return !lhs.has_value() && !rhs.has_value() ? false : !lhs.has_value() ? true : !rhs.has_value() ? false
			: *lhs < *rhs;
	}
}
