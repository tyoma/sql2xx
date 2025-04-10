#include <sql2++/format.h>

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace sql2xx
{
	namespace
	{
		struct type_a
		{
			int a;
			string b;
		};

		struct type_b
		{
			string a;
			string b;
			int64_t c;
		};

		struct type_all
		{
			int a;
			unsigned int b;
			int64_t c;
			uint64_t d;
			string e;
			double f;
		};

		struct type_all_nullable
		{
			int z;
			nullable<int> a;
			nullable<unsigned int> b;
			nullable<int64_t> c;
			nullable<uint64_t> d;
			nullable<string> e;
			nullable<double> f;
		};

		struct type_ided
		{
			unsigned int b;
			int id;
		};

		struct type_inherited : type_ided
		{
			int c;
		};

		struct type_child_a
		{
			string name;
			nullable<int> parent; // type_inherited::id
		};

		struct type_with_unique
		{
			int a;
			int b;
			string c;
			nullable<unsigned int> q;
		};

		struct type_child_b
		{
			string name;
			int parent_a; // type_with_unique::a
			string parent_c; // type_with_unique::c
		};

		struct type_with_primary
		{
			int a;
			int b;
			string c;
			string d;
		};

		template <typename VisitorT>
		void describe(VisitorT &&visitor, type_a *)
		{
			visitor(&type_a::a, "Foo");
			visitor(&type_a::b, "Bar");
		}

		template <typename VisitorT>
		void describe(VisitorT &&visitor, type_b *)
		{
			visitor(&type_b::a, "Lorem");
			visitor(&type_b::b, "Ipsum");
			visitor(&type_b::c, "Amet");
		}

		template <typename VisitorT>
		void describe(VisitorT &&visitor, type_all *)
		{
			visitor(&type_all::a, "A");
			visitor(&type_all::b, "b");
			visitor(&type_all::c, "C");
			visitor(&type_all::d, "D");
			visitor(&type_all::e, "E");
			visitor(&type_all::f, "F");
		}

		template <typename VisitorT>
		void describe(VisitorT &&visitor, type_all_nullable *)
		{
			visitor(&type_all_nullable::a, "A");
			visitor(&type_all_nullable::b, "b");
			visitor(&type_all_nullable::c, "C");
			visitor(&type_all_nullable::d, "D");
			visitor(&type_all_nullable::e, "E");
			visitor(&type_all_nullable::f, "F");
			visitor(&type_all_nullable::z, "zz");
		}

		template <typename VisitorT>
		void describe(VisitorT &&visitor, type_ided *)
		{
			visitor(&type_ided::b, "b");
			visitor(identity, &type_ided::id, "Id");
		}

		template <typename VisitorT>
		void describe(VisitorT &&visitor, type_inherited *)
		{
			visitor("TypeInherited");
			visitor(&type_inherited::b, "b");
			visitor(identity, &type_inherited::id, "id");
			visitor(&type_inherited::c, "c");
		}

		template <typename VisitorT>
		void describe(VisitorT &&visitor, type_child_a *)
		{
			visitor(&type_child_a::name, "name");
			visitor(&type_child_a::parent, "parent");

			visitor << foreign_key_cascade<type_inherited> << &type_child_a::parent << &type_inherited::id;
		}

		template <typename VisitorT>
		void describe(VisitorT &&visitor, type_with_unique *)
		{
			visitor("UNIQUE_TABLE");
			visitor(&type_with_unique::a, "a");
			visitor(identity, &type_with_unique::b, "b");
			visitor(&type_with_unique::c, "c");
			visitor(&type_with_unique::q, "q");

			visitor << sql2xx::unique << &type_with_unique::a << &type_with_unique::c;
		}

		template <typename VisitorT>
		void describe(VisitorT &&visitor, type_child_b *)
		{
			visitor(&type_child_b::name, "name");
			visitor(&type_child_b::parent_a, "parent_a");
			visitor(&type_child_b::parent_c, "parent_c");

			visitor << foreign_key_cascade<type_with_unique>
				<< &type_child_b::parent_a << &type_with_unique::a
				<< &type_child_b::parent_c << &type_with_unique::c;
		}

		template <typename VisitorT>
		void describe(VisitorT &&visitor, type_with_primary *)
		{
			visitor(&type_with_primary::a, "a");
			visitor(&type_with_primary::b, "b");
			visitor(&type_with_primary::c, "c");
			visitor(&type_with_primary::d, "d");

			visitor << sql2xx::primary << &type_with_primary::a << &type_with_primary::b << &type_with_primary::c;
			visitor << sql2xx::unique << &type_with_primary::d;
		}

		template <typename T>
		string format_columns()
		{
			string result;
			list< tuple< string, vector<string> > > constraints;
			list<foreign_key_constraint> foreign_key_constraints;
			column_definition_format_visitor<T> v = {	result, constraints, foreign_key_constraints, true	};

			describe<T>(v);
			return result;
		}

		template <typename T>
		string format_create_table(const char *name)
		{
			string result;

			sql2xx::format_create_table<T>(result, name);
			return result;
		}
	}

	begin_test_suite( DatabaseDDLTests )
		test( FormattingColumnsDefinitionProvidesExpectedResults )
		{
			// INIT / ACT / ASSERT
			assert_equal("Foo INTEGER NOT NULL,Bar TEXT NOT NULL", format_columns<type_a>());
			assert_equal("Lorem TEXT NOT NULL,Ipsum TEXT NOT NULL,Amet INTEGER NOT NULL", format_columns<type_b>());
			assert_equal("A INTEGER NOT NULL,b INTEGER NOT NULL,C INTEGER NOT NULL,D INTEGER NOT NULL,E TEXT NOT NULL,F REAL NOT NULL", format_columns<type_all>());
			assert_equal("A INTEGER,b INTEGER,C INTEGER,D INTEGER,E TEXT,F REAL,zz INTEGER NOT NULL", format_columns<type_all_nullable>());
			assert_equal("a INTEGER NOT NULL,b INTEGER NOT NULL PRIMARY KEY ASC,c TEXT NOT NULL,q INTEGER", format_columns<type_with_unique>());
		}


		test( FormattingCreateTableWithoutKeysProvidesExpectedResults )
		{
			// INIT / ACT / ASSERT
			assert_equal("CREATE TABLE Lorem (Foo INTEGER NOT NULL,Bar TEXT NOT NULL)", format_create_table<type_a>("Lorem"));
			assert_equal("CREATE TABLE Zanzibar (Lorem TEXT NOT NULL,Ipsum TEXT NOT NULL,Amet INTEGER NOT NULL)", format_create_table<type_b>("Zanzibar"));
			assert_equal("CREATE TABLE Bar (A INTEGER NOT NULL,b INTEGER NOT NULL,C INTEGER NOT NULL,D INTEGER NOT NULL,E TEXT NOT NULL,F REAL NOT NULL)", format_create_table<type_all>("Bar"));
		}


		test( FormattingCreateTableWithPrimaryKeyProvidesExpectedResult )
		{
			// INIT / ACT / ASSERT
			assert_equal("CREATE TABLE Baz (b INTEGER NOT NULL,Id INTEGER NOT NULL PRIMARY KEY ASC)", format_create_table<type_ided>("Baz"));
			assert_equal("CREATE TABLE Baz (b INTEGER NOT NULL,id INTEGER NOT NULL PRIMARY KEY ASC,c INTEGER NOT NULL)", format_create_table<type_inherited>("Baz"));
		}


		test( FormattingCreateTableWithConstraintsProvidesExpectedResult )
		{
			assert_equal("CREATE TABLE Bar ("
				"a INTEGER NOT NULL,b INTEGER NOT NULL PRIMARY KEY ASC,c TEXT NOT NULL,q INTEGER,"
				"UNIQUE(a,c)"
				")", format_create_table<type_with_unique>("Bar"));

			assert_equal("CREATE TABLE Baz ("
				"a INTEGER NOT NULL,b INTEGER NOT NULL,c TEXT NOT NULL,d TEXT NOT NULL,"
				"PRIMARY KEY(a,b,c),UNIQUE(d)"
				")", format_create_table<type_with_primary>("Baz"));
		}


		test( FormattingCreateTableWithForeignKeyProvidesExpectedResult )
		{
			assert_equal("CREATE TABLE Child ("
				"name TEXT NOT NULL,parent INTEGER,"
				"FOREIGN KEY(parent) REFERENCES TypeInherited(id) ON DELETE CASCADE"
				")", format_create_table<type_child_a>("Child"));

			assert_equal("CREATE TABLE Child ("
				"name TEXT NOT NULL,parent_a INTEGER NOT NULL,parent_c TEXT NOT NULL,"
				"FOREIGN KEY(parent_a,parent_c) REFERENCES UNIQUE_TABLE(a,c) ON DELETE CASCADE"
				")", format_create_table<type_child_b>("Child"));
		}

	end_test_suite
}
