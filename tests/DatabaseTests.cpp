#include <sql2++/database.h>

#include "file_helpers.h"
#include "helpers.h"

#include <sql2++/types.h>
#include <sqlite3.h>
#include <tuple>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace sql2xx
{
	namespace tests
	{
		namespace
		{
			const auto c_create_sample_1 =
				"BEGIN;"

				"CREATE TABLE 'lorem_ipsums' ('name' TEXT, 'age' INTEGER);"
				"INSERT INTO 'lorem_ipsums' VALUES ('lorem', 3141);"
				"INSERT INTO 'lorem_ipsums' VALUES ('Ipsum', 314159);"
				"INSERT INTO 'lorem_ipsums' VALUES ('Lorem Ipsum Amet Dolor', 314);"
				"INSERT INTO 'lorem_ipsums' VALUES ('lorem', 31415926);"

				"CREATE TABLE 'sample_items_2' ('age' INTEGER, 'nickname' TEXT, 'name' TEXT);"
				"INSERT INTO 'sample_items_2' VALUES (3141, 'Bob', 'lorem');"
				"INSERT INTO 'sample_items_2' VALUES (314159, 'AJ', 'Ipsum');"
				"INSERT INTO 'sample_items_2' VALUES (314, 'Liz', 'Lorem Ipsum Amet Dolor');"
				"INSERT INTO 'sample_items_2' VALUES (314, 'K', 'lorem');"
				"INSERT INTO 'sample_items_2' VALUES (31415926, 'K', 'lorem');"

				"CREATE TABLE 'sample_items_1' ('a' INTEGER, 'b' TEXT);"

				"CREATE TABLE 'sample_items_3' ('MyID' INTEGER PRIMARY KEY ASC, 'a' INTEGER, 'b' TEXT, 'c' INTEGER, 'd' REAL, 'e' INTEGER, 'f' INTEGER);"

				"COMMIT;";

			template <int>
			struct test_a
			{
				string name;
				int age;

				bool operator <(const test_a &rhs) const
				{	return make_pair(name, age) < make_pair(rhs.name, rhs.age);	}
			};

			struct test_b
			{
				string nickname;
				int suspect_age;
				string suspect_name;

				bool operator <(const test_b& rhs) const
				{
					return make_tuple(nickname, suspect_age, suspect_name)
						< make_tuple(rhs.nickname, rhs.suspect_age, rhs.suspect_name);
				}
			};

			struct test_c
			{
				int a, b;

				bool operator <(const test_c &rhs) const
				{	return make_tuple(a, b) < make_tuple(rhs.a, rhs.b);	}
			};

			struct test_d
			{
				int bb;
				string bbb;
				int bbbb;

				bool operator <(const test_d &rhs) const
				{	return make_tuple(bb, bbb, bbbb) < make_tuple(rhs.bb, rhs.bbb, rhs.bbbb);	}
			};

			struct sample_item_1
			{
				int a;
				string b;

				bool operator <(const sample_item_1& rhs) const
				{	return make_tuple(a, b) < make_tuple(rhs.a, rhs.b);	}
			};

			template <int>
			struct sample_item_3
			{
				int id;
				int a;
				string b;
				int64_t c;
				double d;
				uint64_t e;
				unsigned int f;

				bool operator <(const sample_item_3& rhs) const
				{
					auto approx_less = [] (double lhs, double rhs) {
						return lhs < rhs - 0.000001 * (rhs - lhs);
					};
					auto tlhs = make_tuple(id, a, b, c, e, f);
					auto trhs = make_tuple(rhs.id, rhs.a, rhs.b, rhs.c, rhs.e, rhs.f);

					if (tlhs < trhs)
						return true;
					if (trhs < tlhs)
						return false;
					return approx_less(d, rhs.d);
				}
			};

			struct sample_base
			{
				int id;
				string comment;
			};

			struct sample_inherited : sample_base
			{
				int b;
				int64_t c;

				static sample_inherited create(int id_, string comment_, int b_, int64_t c_)
				{
					sample_inherited v;

					v.id = id_;
					v.comment = comment_;
					v.b = b_;
					v.c = c_;
					return v;
				}

				bool operator <(const sample_inherited &rhs) const
				{	return make_tuple(id, b, c, comment) < make_tuple(rhs.id, rhs.b, rhs.c, rhs.comment);	}
			};

			template <typename VisitorT>
			void describe(VisitorT &visitor, test_a<0> *)
			{
				visitor("lorem_ipsums");
				visitor(&test_a<0>::name, "name");
				visitor(&test_a<0>::age, "age");
			}

			template <typename VisitorT>
			void describe(VisitorT &visitor, test_a<1> *)
			{
				visitor("other_lorem_ipsums");
				visitor(&test_a<1>::name, "name");
				visitor(&test_a<1>::age, "age");
			}

			template <typename VisitorT>
			void describe(VisitorT &visitor, sample_item_1 *)
			{
				visitor("sample_items_1");
				visitor(&sample_item_1::a, "a");
				visitor(&sample_item_1::b, "b");
			}

			template <typename VisitorT>
			void describe(VisitorT& visitor, test_b *)
			{
				visitor("sample_items_2");
				visitor(&test_b::suspect_name, "name");
				visitor(&test_b::suspect_age, "age");
				visitor(&test_b::nickname, "nickname");
			}

			template <typename VisitorT>
			void describe(VisitorT &visitor, sample_item_3<0> *)
			{
				visitor("sample_items_3");
				visitor(identity, &sample_item_3<0>::id, "MyID");
				visitor(&sample_item_3<0>::a, "a");
				visitor(&sample_item_3<0>::b, "b");
				visitor(&sample_item_3<0>::c, "c");
				visitor(&sample_item_3<0>::d, "d");
				visitor(&sample_item_3<0>::e, "e");
				visitor(&sample_item_3<0>::f, "f");
			}

			template <typename VisitorT>
			void describe(VisitorT &visitor, sample_item_3<1> *)
			{
				visitor("other_sample_items_3");
				visitor(identity, &sample_item_3<1>::id, "MyID");
				visitor(&sample_item_3<1>::a, "a");
				visitor(&sample_item_3<1>::b, "b");
				visitor(&sample_item_3<1>::c, "c");
				visitor(&sample_item_3<1>::d, "d");
				visitor(&sample_item_3<1>::e, "e");
				visitor(&sample_item_3<1>::f, "f");
			}

			template <typename VisitorT>
			void describe(VisitorT &visitor, sample_inherited *)
			{
				visitor("Test");
				visitor(identity, &sample_inherited::id, "myid");
				visitor(&sample_inherited::b, "bb");
				visitor(&sample_inherited::c, "cc");
				visitor(&sample_inherited::comment, "comment");
			}

			template <typename VisitorT>
			void describe(VisitorT &visitor, test_c *)
			{
				visitor("test_c_table");
				visitor(&test_c::a, "aa");
				visitor(&test_c::b, "aaa");
			}

			template <typename VisitorT>
			void describe(VisitorT &visitor, test_d *)
			{
				visitor("TestTableD");
				visitor(&test_d::bb, "bb");
				visitor(&test_d::bbb, "bbb");
				visitor(&test_d::bbbb, "bbbb");
			}
		}

		begin_test_suite( DatabaseTests )
			temporary_directory dir;
			sqlite3 *database;
			string path;

			init( CreateSamples )
			{
				path = dir.track_file("sample-db.db");
				sqlite3_open_v2(path.c_str(), &database,
					SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
				sqlite3_exec(database, c_create_sample_1, nullptr, nullptr, nullptr);
			}

			teardown( Release )
			{
				sqlite3_close(database);
			}


			test( ValuesInTablesCanBeRead )
			{
				// INIT / ACT
				test_a<0> a;
				vector<test_a<0>> results_a;
				transaction t(create_connection(path.c_str()));

				auto r = t.select<test_a<0>>();

				// ACT
				while (r(a))
					results_a.push_back(a);

				// ASSERT
				assert_equivalent(plural
					+ initialize<test_a<0>>("lorem", 3141)
					+ initialize<test_a<0>>("Ipsum", 314159)
					+ initialize<test_a<0>>("Lorem Ipsum Amet Dolor", 314)
					+ initialize<test_a<0>>("lorem", 31415926), results_a);

				// INIT
				test_b b;
				vector<test_b> results_b;

				// INIT / ACT
				auto rb = t.select<test_b>();

				// ACT
				while (rb(b))
					results_b.push_back(b);

				// ASSERT
				assert_equivalent(plural
					+ initialize<test_b>("Bob", 3141, "lorem")
					+ initialize<test_b>("AJ", 314159, "Ipsum")
					+ initialize<test_b>("Liz", 314, "Lorem Ipsum Amet Dolor")
					+ initialize<test_b>("K", 314, "lorem")
					+ initialize<test_b>("K", 31415926, "lorem"), results_b);
			}


			test( ValuesInTableCanBeReadWithAQuery )
			{
				// INIT / ACT
				test_b b;
				vector<test_b> results;
				transaction t(create_connection(path.c_str()));

				// INIT / ACT
				auto r = t.select<test_b>(c(&test_b::suspect_age) == p<const int>(314));

				// ACT
				while (r(b))
					results.push_back(b);

				// ASSERT
				assert_equivalent(plural
					+ initialize<test_b>("K", 314, "lorem")
					+ initialize<test_b>("Liz", 314, "Lorem Ipsum Amet Dolor"), results);

				// INIT
				vector<test_b> results2;

				results.clear();

				// INIT / ACT
				auto r2 = t.select<test_b>(c(&test_b::suspect_age) == p<const int>(314)
					|| c(&test_b::suspect_name) == p<const string>("Ipsum"));
				auto r3 = t.select<test_b>(c(&test_b::suspect_name) == p<const string>("Ipsum")
					|| c(&test_b::suspect_age) == p<const int>(314));

				// ACT
				while (r2(b))
					results.push_back(b);

				while (r3(b))
					results2.push_back(b);

				// ASSERT
				assert_equivalent(plural
					+ initialize<test_b>("AJ", 314159, "Ipsum")
					+ initialize<test_b>("K", 314, "lorem")
					+ initialize<test_b>("Liz", 314, "Lorem Ipsum Amet Dolor"), results);
				assert_equivalent(results, results2);
			}


			test( InsertedRecordsAreNotVisibleBeforeCommit )
			{
				// INIT
				sample_item_1 item = {	314, "Bob Marley"	};
				transaction t(create_connection(path.c_str()));

				// INIT / ACT
				auto w1 = t.insert<sample_item_1>();

				// ACT
				w1(item);

				// ASSERT
				assert_is_empty(read_all<sample_item_1>(path));
			}


			test( InsertedRecordsCanBeRead )
			{
				// INIT
				sample_item_1 items1[] = {
					{	314, "Bob Marley"	},
					{	141, "Peter Tosh"	},
					{	3141, "John Zorn"	},
				};
				transaction t(create_connection(path.c_str()));

				// INIT / ACT
				auto w1 = t.insert<sample_item_1>();

				// ACT
				for (auto i = begin(items1); i != end(items1); ++i)
					w1(static_cast<const sample_item_1 &>(*i));
				t.commit();

				// ASSERT
				assert_equivalent(items1, read_all<sample_item_1>(path));

				// INIT
				test_b items2[] = {
					{	"Bob", 3141, "lorem"	},
					{	"AJ", 314159, "Ipsum"	},
					{	"Liz", 314, "Lorem Ipsum Amet Dolor"	},
					{	"K", 314, "lorem"	},
					{	"K", 31415926, "lorem"	},
				};

				// INIT / ACT
				transaction t2(create_connection(path.c_str()));
				auto w2 = t2.insert<test_b>();

				// ACT
				for (auto i = begin(items2); i != end(items2); ++i)
					w2(*i);
				t2.commit();

				// ASSERT
				auto ref = plural
					+ initialize<test_b>("Bob", 3141, "lorem")
					+ initialize<test_b>("AJ", 314159, "Ipsum")
					+ initialize<test_b>("Liz", 314, "Lorem Ipsum Amet Dolor")
					+ initialize<test_b>("K", 314, "lorem")
					+ initialize<test_b>("K", 31415926, "lorem");
				ref.insert(ref.end(), begin(items2), end(items2));
				assert_equivalent(ref, read_all<test_b>(path));
			}


			test( InnerTransactionsAreProhibited )
			{
				// INIT
				auto conn = create_connection(path.c_str());

				// INIT / ACT
				auto locker = make_shared<transaction>(conn);

				// ACT / ASSERT
				assert_throws(transaction contender(conn), sql_error);

				// ACT
				locker.reset();

				// ACT / ASSERT
				transaction noncontender(conn);
			}


			test( TransactionsCombineAccordinglyToType )
			{
				// INIT
				auto conn1 = create_connection(path.c_str());
				auto conn2 = create_connection(path.c_str());
				auto conn3 = create_connection(path.c_str());
				auto conn4 = create_connection(path.c_str());

				// ACT
				{
					transaction locker(conn1, transaction::immediate);

				// ACT / ASSERT
					assert_throws(transaction contender(conn2, transaction::immediate, 400), sql_error);
					assert_throws(transaction contender(conn2, transaction::exclusive, 400), sql_error);

				// ACT / ASSERT (does not throw)
					transaction non_contended(conn2, transaction::deferred, 2000);
				}

				// ACT
				{
					transaction nonlocker(conn1, transaction::deferred);

					// ACT / ASSERT (does not throw)
					transaction non_contender1(conn2, transaction::immediate, 2000);
					transaction non_contended2(conn3, transaction::deferred, 2000);
				}

				// ACT
				{
					transaction locker(conn1, transaction::exclusive);

				// ACT / ASSERT
					assert_throws(transaction contender(conn3, transaction::immediate, 200), sql_error);
					assert_throws(transaction contender(conn4, transaction::exclusive, 400), sql_error);
				}
			}


			test( PrimaryKeyIsSetUponInsertion )
			{
				// INIT
				transaction t(create_connection(path.c_str()));
				auto w = t.insert<sample_item_3<0>>();

				// ACT
				for (int n = 1000, previous = 0; n--; )
				{
					sample_item_3<0> item = {};

					w(item);

				// ASSERT
					if (previous)
						assert_equal(previous + 1, item.id);

					previous = item.id;
				}
			}


			test( AllSupportedTypesCanBeSelected )
			{
				// INIT
				vector<sample_item_3<0>> items_read;
				sample_item_3<0> items[] = {
					{	100, 0, "Bod Dylan", 10000000001, 1.5391, 0x8912323200000001, 0xB0000000	},
					{	100, 110, "Nick Cave", 10000000002, 1e-8, 0xF912323200000001, 0x00000000	},
					{	100, 13, "Robert Fripp", 10000000001, 1.5e12, 0x1912323200000001, 0x10000000	},
				};
				transaction t(create_connection(path.c_str()));
				auto w = t.insert<sample_item_3<0>>();

				for (auto i = begin(items); i != end(items); ++i)
					w(*i);

				// ACT
				auto r = t.select<sample_item_3<0>>();

				for (sample_item_3<0> item; r(item); )
					items_read.push_back(item);

				// ASSERT
				sample_item_3<0> reference1[] = {
					{	1, 0, "Bod Dylan", 10000000001, 1.5391, 0x8912323200000001, 0xB0000000	},
					{	2, 110, "Nick Cave", 10000000002, 1e-8, 0xF912323200000001, 0x00000000	},
					{	3, 13, "Robert Fripp", 10000000001, 1.5e12, 0x1912323200000001, 0x10000000	},
				};

				assert_equivalent(reference1, items_read);

				// INIT
				sample_item_3<0> items2[] = {
					{	100, 0, "Jimi Hendrix", 30000000001, 1.5391, 0, 0	},
					{	100, 0, "Tom Waits", 70000000002, 3.141e-8, 0, 0	},
				};

				for (auto i = begin(items2); i != end(items2); ++i)
					w(*i);
				items_read.clear();

				// ACT
				auto r2 = t.select<sample_item_3<0>>();

				for (sample_item_3<0> item; r2(item); )
					items_read.push_back(item);

				// ASSERT
				sample_item_3<0> reference2[] = {
					{	1, 0, "Bod Dylan", 10000000001, 1.5391, 0x8912323200000001, 0xB0000000	},
					{	2, 110, "Nick Cave", 10000000002, 1e-8, 0xF912323200000001, 0x00000000	},
					{	3, 13, "Robert Fripp", 10000000001, 1.5e12, 0x1912323200000001, 0x10000000	},
					{	4, 0, "Jimi Hendrix", 30000000001, 1.5391, 0, 0	},
					{	5, 0, "Tom Waits", 70000000002, 3.141e-8, 0, 0	},
				};

				assert_equivalent(reference2, items_read);
			}


			test( CreatedTablesCanBeUsed )
			{
				// INIT
				transaction t(create_connection(path.c_str()));

				// ACT
				t.create_table< sample_item_3<1> >();

				// ASSERT
				vector< sample_item_3<1> > items_read;
				sample_item_3<1> items[] = {
					{	100, 0, "Bod Dylan", 10000000001, 1.5391, 0x8912323200000001, 0xB0000000	},
					{	100, 110, "Nick Cave", 10000000002, 1e-8, 0xF912323200000001, 0x00000000	},
					{	100, 13, "Robert Fripp", 10000000001, 1.5e12, 0x1912323200000001, 0x10000000	},
				};
				auto w = t.insert< sample_item_3<1> >();

				for (auto i = begin(items); i != end(items); ++i)
					w(*i);
				auto r = t.select< sample_item_3<1> >();

				for (sample_item_3<1> item; r(item); )
					items_read.push_back(item);

				assert_equivalent(items, items_read);

				// ACT
				t.create_table< test_a<1> >();

				// ASSERT
				vector< test_a<1> > items_read2;
				test_a<1> items2[] = {
					{	"Bod Dylan", -9001	},
					{	"Nick Cave", 11123	},
				};
				auto w2 = t.insert< test_a<1> >();

				for (auto i = begin(items2); i != end(items2); ++i)
					w2(*i);
				auto r2 = t.select< test_a<1> >();

				for (test_a<1> item; r2(item); )
					items_read2.push_back(item);

				assert_equivalent(items2, items_read2);
			}


			test( InheritanceIsSupportedWhenInsertingAndReading )
			{
				// INIT
				transaction t(create_connection(path.c_str()));
				auto items = plural
					+ sample_inherited::create(10, "lorem ipsum", 12, 23)
					+ sample_inherited::create(10, "ipsum", 121, 213)
					+ sample_inherited::create(10, "amet dolor", 1, 2223);

				// INIT / ACT
				t.create_table<sample_inherited>();

				auto w = t.insert<sample_inherited>();

				// ACT
				for (auto i = begin(items); i != end(items); ++i)
					w(*i);

				// ASSERT
				auto reference = plural
					+ sample_inherited::create(1, "lorem ipsum", 12, 23)
					+ sample_inherited::create(2, "ipsum", 121, 213)
					+ sample_inherited::create(3, "amet dolor", 1, 2223);

				assert_equivalent(reference, items);

				// INIT / ACT
				vector<sample_inherited> items_read;
				auto r = t.select<sample_inherited>();

				// ACT
				for (sample_inherited item; r(item); )
					items_read.push_back(item);

				// ASSERT
				assert_equivalent(reference, items_read);
			}


			test( DefaultTableNameIsUsedWhenNotProvided )
			{
				// INIT
				transaction t(create_connection(path.c_str()));
				auto items1 = plural
					+ initialize<test_c>(1, 11)
					+ initialize<test_c>(1, 13)
					+ initialize<test_c>(7, 11);
				auto items2 = plural
					+ initialize<test_d>(1, "lost", 11)
					+ initialize<test_d>(2, "lost", 13)
					+ initialize<test_d>(7, "lost", 17)
					+ initialize<test_d>(3, "found", 19);

				// ACT
				t.create_table<test_c>();
				t.create_table<test_d>();

				// INIT / ACT
				auto w1 = t.insert<test_c>();
				auto w2 = t.insert<test_d>();

				for (auto i = begin(items1); i != end(items1); ++i)
					w1(*i);
				for (auto i = begin(items2); i != end(items2); ++i)
					w2(*i);

				// ACT
				auto r1 = read_all(t.select<test_c>());
				auto r2 = read_all(t.select<test_d>());

				// ASSERT
				assert_equivalent(items1, r1);
				assert_equivalent(items2, r2);

				// ACT
				auto r3 = read_all(t.select<test_c>(c(&test_c::b) == p<const int>(11)));
				auto r4 = read_all(t.select<test_d>(c(&test_d::bbb) == p<const string>("lost")));

				// ASSERT
				assert_equivalent(plural
					+ initialize<test_c>(1, 11)
					+ initialize<test_c>(7, 11), r3);
				assert_equivalent(plural
					+ initialize<test_d>(1, "lost", 11)
					+ initialize<test_d>(2, "lost", 13)
					+ initialize<test_d>(7, "lost", 17), r4);

				// INIT
				auto item1 = initialize<test_c>(1, 130);
				auto item2 = initialize<test_d>(1, "abc", 100);

				// ACT
				auto w3 = t.insert<test_c>();
				auto w4 = t.insert<test_d>();

				// ACT / ASSERT
				w3(item1);
				w4(item2);

				// ASSERT
				assert_equivalent(plural
					+ initialize<test_c>(1, 11)
					+ initialize<test_c>(1, 13)
					+ initialize<test_c>(7, 11)
					+ initialize<test_c>(1, 130), read_all(t.select<test_c>()));
				assert_equivalent(plural
					+ initialize<test_d>(1, "lost", 11)
					+ initialize<test_d>(2, "lost", 13)
					+ initialize<test_d>(7, "lost", 17)
					+ initialize<test_d>(3, "found", 19)
					+ initialize<test_d>(1, "abc", 100), read_all(t.select<test_d>()));

				// INIT / ACT
				auto d1 = t.remove<test_d>(c(&test_d::bb) == p<const int>(1));

				// ACT
				d1.execute();

				// ASSERT
				assert_equivalent(plural
					+ initialize<test_d>(2, "lost", 13)
					+ initialize<test_d>(7, "lost", 17)
					+ initialize<test_d>(3, "found", 19), read_all(t.select<test_d>()));

				// INIT / ACT
				auto d2 = t.remove<test_c>(c(&test_c::b) == p<const int>(11));

				// ACT
				d2.execute();

				// ASSERT
				assert_equivalent(plural
					+ initialize<test_c>(1, 13)
					+ initialize<test_c>(1, 130), read_all(t.select<test_c>()));
			}


			test( RecordsAreDeletedAccordinglyToTheCriteria )
			{
				// INIT
				transaction t(create_connection(path.c_str()));
				auto items1 = plural
					+ initialize<test_b>("foo", 1, "lorem")
					+ initialize<test_b>("foo", 17, "ipsum")
					+ initialize<test_b>("bar", 31, "amet")
					+ initialize<test_b>("bar", 23, "dolor")
					+ initialize<test_b>("bar", 29, "lorem")
					+ initialize<test_b>("baz", 7, "ipsum");
				auto items2 = plural
					+ initialize<sample_item_1>(1, "test")
					+ initialize<sample_item_1>(2, "testtest")
					+ initialize<sample_item_1>(3, "sample")
					+ initialize<sample_item_1>(2, "feature")
					+ initialize<sample_item_1>(2, "goal");

				// INIT / ACT
				auto stmt01 = t.remove<test_b>(p<const int>(1) == p<const int>(1));
				auto stmt02 = t.remove<test_b>(p<const int>(1) == p<const int>(1));

				// ACT
				stmt01.execute();
				stmt02.execute();

				// ASSERT
				assert_is_empty(read_all<test_b>(t));
				assert_is_empty(read_all<sample_item_1>(t));

				// INIT
				write_all(t, items1);
				write_all(t, items2);

				// INIT / ACT
				auto stmt1 = t.remove<test_b>(p<const string>("qqq") == c(&test_b::nickname));
				auto stmt2 = t.remove<test_b>(p<const string>("foo") == c(&test_b::nickname) && p<const int>(17) == c(&test_b::suspect_age));

				// ACT
				stmt1.execute();
				stmt2.execute();

				// ASSERT
				assert_equivalent(plural
					+ initialize<test_b>("foo", 1, "lorem")
					+ initialize<test_b>("bar", 31, "amet")
					+ initialize<test_b>("bar", 23, "dolor")
					+ initialize<test_b>("bar", 29, "lorem")
					+ initialize<test_b>("baz", 7, "ipsum"), read_all<test_b>(t));

				// INIT / ACT
				auto stmt3 = t.remove<test_b>(p<const string>("lorem") == c(&test_b::suspect_name));

				// ACT
				stmt3.execute();

				// ASSERT
				assert_equivalent(plural
					+ initialize<test_b>("bar", 31, "amet")
					+ initialize<test_b>("bar", 23, "dolor")
					+ initialize<test_b>("baz", 7, "ipsum"), read_all<test_b>(t));

				// INIT / ACT
				auto stmt4 = t.remove<sample_item_1>(p<const int>(2) == c(&sample_item_1::a));

				// ACT
				stmt4.execute();

				// ASSERT
				assert_equivalent(plural
					+ initialize<sample_item_1>(1, "test")
					+ initialize<sample_item_1>(3, "sample"), read_all<sample_item_1>(t));
			}


			test( ResetStatementIsExecutedWithNewParameters )
			{
				// INIT
				transaction t(create_connection(path.c_str()));
				auto items1 = plural
					+ initialize<test_b>("foo", 1, "lorem")
					+ initialize<test_b>("foo", 17, "ipsum")
					+ initialize<test_b>("bar", 31, "lorem")
					+ initialize<test_b>("bar", 29, "dolor")
					+ initialize<test_b>("bar", 29, "lorem")
					+ initialize<test_b>("baz", 7, "ipsum");
				string arg1, arg2;
				int arg3 = 0;

				t.remove<test_b>(p<const int>(1) == p<const int>(1)).execute();
				write_all(t, items1);

				// INIT / ACT
				auto stmt1 = t.remove<test_b>(p(arg1) == c(&test_b::nickname));
				auto stmt2 = t.remove<test_b>(p(arg2) == c(&test_b::suspect_name) && p(arg3) == c(&test_b::suspect_age));

				stmt1.execute();
				stmt2.execute();

				// ACT
				arg1 = "foo";
				stmt1.reset();
				stmt1.execute();

				// ASSERT
				assert_equivalent(plural
					+ initialize<test_b>("bar", 31, "lorem")
					+ initialize<test_b>("bar", 29, "dolor")
					+ initialize<test_b>("bar", 29, "lorem")
					+ initialize<test_b>("baz", 7, "ipsum"), read_all<test_b>(t));

				// ACT
				arg2 = "lorem";
				arg3 = 29;
				stmt2.reset();
				stmt2.execute();

				// ASSERT
				assert_equivalent(plural
					+ initialize<test_b>("bar", 31, "lorem")
					+ initialize<test_b>("bar", 29, "dolor")
					+ initialize<test_b>("baz", 7, "ipsum"), read_all<test_b>(t));
			}
		end_test_suite
	}
}
