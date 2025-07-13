#include <sql2++/database.h>

#include "file_helpers.h"
#include "helpers.h"

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace sql2xx
{
	namespace
	{
		struct Company
		{
			int id;
			string name;
			string incorporated;
		};

		struct Campaign
		{
			string name;
		};

		struct Contact
		{
			string name;
			int company;
			nullable<string> campaign;
			string position;
		};

		template <typename VisitorT>
		void describe(VisitorT &&visitor, Company *)
		{
			visitor("Company");
			visitor(identity, &Company::id, "id");
			visitor(&Company::name, "name");
			visitor(&Company::incorporated, "incorporated");

			visitor << unique << &Company::name;
		}

		template <typename VisitorT>
		void describe(VisitorT &&visitor, Campaign *)
		{
			visitor("Campaign");
			visitor(&Campaign::name, "name");

			visitor << unique << &Campaign::name;
		}

		template <typename VisitorT>
		void describe(VisitorT &&visitor, Contact *)
		{
			visitor("Contact");
			visitor(&Contact::name, "name");
			visitor(&Contact::company, "company_id");
			visitor(&Contact::campaign, "campaign_id");
			visitor(&Contact::position, "position");

			visitor << foreign_key_cascade<Company> << &Contact::company << &Company::id;
			visitor << foreign_key_cascade<Campaign> << &Contact::campaign << &Campaign::name;
		}
	}

	namespace tests
	{
		begin_test_suite( ConstrainedTablesTests )
			temporary_directory dir;
			string path;

			init( CreateSamples )
			{
				path = dir.track_file("sample-db.db");
			}

			teardown( Release )
			{
			}


			test( InsertersReadersAndRemoversAreCompilable )
			{
				// INIT
				transaction t(create_connection(path.c_str()));
				Company company = {	0, "Apple", "1975-01-02"	};
				Campaign campaign = { "vote-or-lose" };
				Contact contact = { "John Doe", 0, nullable<string>("vote-or-lose") };

				// ACT
				t.create_table<Company>();
				t.create_table<Campaign>();
				t.create_table<Contact>();

				// INIT / ACT
				auto ins_company = t.insert<Company>();
				auto ins_campaign = t.insert<Campaign>();
				auto ins_contact = t.insert<Contact>();

				// ACT / ASSERT (compilable)
				ins_company(company);
				ins_campaign(campaign);

				contact.company = company.id;
				ins_contact(contact);

				// INIT / ACT
				auto read_company = t.select<Company>();
				auto read_campaign = t.select<Campaign>();
				auto read_contact = t.select<Contact>();

				// ACT
				read_company(company);
				read_campaign(campaign);
				read_contact(contact);

				// INIT / ACT
				auto read_company2 = t.select<Company>(c(&Company::incorporated) == p<const string>("1973-01-01"));
				auto read_campaign2 = t.select<Campaign>(c(&Campaign::name) == p<const string>("lorem ipsum"));
				auto read_contact2 = t.select<Contact>(c(&Contact::position) == p<const string>("CTO"));

				// ACT
				read_company2(company);
				read_campaign2(campaign);
				read_contact2(contact);

				// ACT
				t.remove<Company>(c(&Company::id) == p<const int>(1)).execute();
				t.remove<Campaign>(c(&Campaign::name) == p<const string>("company.id")).execute();
				t.remove<Contact>(c(&Contact::name) == p<const string>("Rick Berg")).execute();
			}

		end_test_suite
	}
}
