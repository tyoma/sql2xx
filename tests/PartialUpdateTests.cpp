#include <sql2++/database.h>

#include "file_helpers.h"
#include "helpers.h"

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace sql2xx
{
	namespace tests
	{
		namespace
		{
			const auto c_create_sample =
				"BEGIN;"
				"CREATE TABLE 'Seller' ('id' INTEGER PRIMARY KEY ASC, 'name' TEXT NOT NULL, 'dob' TEXT NOT NULL, 'employer' TEXT NULL, 'license_issued' TEXT NULL, 'deals_closed' INTEGER);"
				"INSERT INTO 'Seller' ('name', 'dob', 'deals_closed') VALUES ('John Doe', '1978-09-29', 24);"
				"INSERT INTO 'Seller' ('name', 'dob', 'employer') VALUES ('Helen ?', '1980-01-30', 'Qualcomm');"
				"INSERT INTO 'Seller' ('name', 'dob', 'employer', 'license_issued') VALUES ('Kat Smilings', '1992-12-07', 'BrandUp', '2019-07-19');"
				"COMMIT";
		
			struct Seller
			{
				string name;
				string dob;
				nullable<string> employer;
				nullable<string> license_issued;
				nullable<int> deals_closed;
				int id;
				
				bool operator <(const Seller& rhs) const
				{
					return make_tuple(name, dob, employer, license_issued, deals_closed)
						< make_tuple(rhs.name, rhs.dob, rhs.employer, rhs.license_issued, rhs.deals_closed);
				}
			};
		
			template <typename VisitorT>
			void describe(VisitorT &&visitor, Seller *)
			{
				visitor("Seller");
				visitor(&Seller::name, "name");
				visitor(&Seller::dob, "dob");
				visitor(&Seller::employer, "employer");
				visitor(&Seller::license_issued, "license_issued");
				visitor(&Seller::deals_closed, "deals_closed");
				visitor(&Seller::id, "id");
			}
		}

		begin_test_suite( PartialUpdateTests )
			temporary_directory dir;
			string path;

			init( CreateSamples )
			{
				path = dir.track_file("sample-db.db");
				auto conn = create_connection(path.c_str());

				sqlite3_exec(conn.get(), c_create_sample, nullptr, nullptr, nullptr);
			}


			test( NewValuesAreReadAfterTheUpdate2 )
			{
				// INIT
				transaction t(create_connection(path.c_str()));
				string where_dob = "1980-01-01";
				nullable<int> deals_closed;

				// INIT / ACT
				auto update = t.update<Seller>(c(&Seller::dob) > p(where_dob), &Seller::deals_closed, deals_closed);

				// ACT
				update.execute();

				// ASSERT
				assert_equivalent(plural
					+ initialize<Seller>("John Doe", "1978-09-29", nullable<string>(), nullable<string>(), nullable<int>(24))
					+ initialize<Seller>("Helen ?", "1980-01-30", nullable<string>("Qualcomm"), nullable<string>(), nullable<int>())
					+ initialize<Seller>("Kat Smilings", "1992-12-07", nullable<string>("BrandUp"), nullable<string>("2019-07-19"), nullable<int>()),
					read_all<Seller>(t));

				// INIT
				deals_closed = 10;

				// ACT
				update.reset();
				update.execute();

				// ASSERT
				assert_equivalent(plural
					+ initialize<Seller>("John Doe", "1978-09-29", nullable<string>(), nullable<string>(), nullable<int>(24))
					+ initialize<Seller>("Helen ?", "1980-01-30", nullable<string>("Qualcomm"), nullable<string>(), nullable<int>(10))
					+ initialize<Seller>("Kat Smilings", "1992-12-07", nullable<string>("BrandUp"), nullable<string>("2019-07-19"), nullable<int>(10)),
					read_all<Seller>(t));

				// INIT
				where_dob = "1972-01-01";
				deals_closed = 11;

				// ACT
				update.reset();
				update.execute();

				// ASSERT
				assert_equivalent(plural
					+ initialize<Seller>("John Doe", "1978-09-29", nullable<string>(), nullable<string>(), nullable<int>(11))
					+ initialize<Seller>("Helen ?", "1980-01-30", nullable<string>("Qualcomm"), nullable<string>(), nullable<int>(11))
					+ initialize<Seller>("Kat Smilings", "1992-12-07", nullable<string>("BrandUp"), nullable<string>("2019-07-19"), nullable<int>(11)),
					read_all<Seller>(t));

				// INIT
				deals_closed = nullable<int>();

				// ACT
				update.reset();
				update.execute();

				// ASSERT
				assert_equivalent(plural
					+ initialize<Seller>("John Doe", "1978-09-29", nullable<string>(), nullable<string>(), nullable<int>())
					+ initialize<Seller>("Helen ?", "1980-01-30", nullable<string>("Qualcomm"), nullable<string>(), nullable<int>())
					+ initialize<Seller>("Kat Smilings", "1992-12-07", nullable<string>("BrandUp"), nullable<string>("2019-07-19"), nullable<int>()),
					read_all<Seller>(t));
			}


			test( NewValuesAreReadAfterTheUpdate3 )
			{
				// INIT
				transaction t(create_connection(path.c_str()));
				int id = 1;
				nullable<string> license_issued, employer = nullable<string>("TruU");

				// INIT / ACT
				auto update = t.update<Seller>(c(&Seller::id) == p(id), &Seller::employer, employer, &Seller::license_issued, license_issued);

				// ACT
				update.execute();

				// ASSERT
				assert_equivalent(plural
					+ initialize<Seller>("John Doe", "1978-09-29", nullable<string>("TruU"), nullable<string>(), nullable<int>(24))
					+ initialize<Seller>("Helen ?", "1980-01-30", nullable<string>("Qualcomm"), nullable<string>(), nullable<int>())
					+ initialize<Seller>("Kat Smilings", "1992-12-07", nullable<string>("BrandUp"), nullable<string>("2019-07-19"), nullable<int>()),
					read_all<Seller>(t));

				// INIT
				id = 3;
				license_issued = "2020-01-01";
				employer = nullable<string>("Salesforce");

				// ACT
				update.reset();
				update.execute();

				// ASSERT
				assert_equivalent(plural
					+ initialize<Seller>("John Doe", "1978-09-29", nullable<string>("TruU"), nullable<string>(), nullable<int>(24))
					+ initialize<Seller>("Helen ?", "1980-01-30", nullable<string>("Qualcomm"), nullable<string>(), nullable<int>())
					+ initialize<Seller>("Kat Smilings", "1992-12-07", nullable<string>("Salesforce"), nullable<string>("2020-01-01"), nullable<int>()),
					read_all<Seller>(t));
			}
		end_test_suite
	}
}
