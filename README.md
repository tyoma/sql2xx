
# sql2++
A type-safe C++ ORM wrapper for SQLite3 (and possibly other SQL engines) 
## Quick Reference
### Type definition
sql2++ operates with the database by binding C++ user types with the table definitions in the sqlite database. It makes so by facilitating template function describe(). Let's consider a user type User:

    struct user
    {
	    int id;
	    std::string first_name;
	    std::string last_name;
	    std::string email;
    };
    
Let's describe this type, so that sql2++ would know how to write, update, read and remove users:

	template <typename VisitorT>
	void describe(VisitorT &visitor, user *)
	{
		visitor("Users"); // Table name: Users
		visitor(sql2xx::identity, &user::id, "ID"); // Idenitity column 'ID'
		visitor(&user::first_name, "FirstName"); // First name will be stored in column 'FirstName'
		visitor(&user::last_name, "LastName"); // etc.
		visitor(&user::email, "email");
	}
### Table creation
This will bind objects of type 'user' with the records in table 'Users' , with an autogenerated id. To make sql2xx create this table we only need to write:

    sql2xx::transaction tx(connection);
	
	tx.create_table<user>();
	tx.commit();

### Insertion (INSERT statement)
Now to insert a new record we can use an inserter. Please note, the inserter can be reused -- this way you'll save time by avoiding the creation of an underlying statement:

	user users[] = {
		{	0, "John", "Doe", "jdoe@gmail.com"	},
		{	0, "Bob", "Dylan", "dylan@hotmail.com"	},
		{	0, "Ian", "Burr", "iburr@yahoo.com"	},
		{	0, "Bill", "Burr", "william.burr@gmail.com"	},
	};
	auto inserter = tx.insert<user>();
	
	for (auto u = std::begin(users); u != std::end(users); ++u)
		inserter(*u);
	tx.commit();
		
Upon execution of this code you'll end up with a Users table populated with the four user records. Please note, that every entry in the 'users' array will be assigned an autogenerated id equal to that in the ID column of the Users table. Hence, the inserter needs a non-constant reference to the object being inserted.
	
### Reading (SELECT statement)
The simplest form of reading is unconditional (no WHERE clause is used). To do this we first need to create a reader:

	auto users_reader = tx.select<user>();

The 'users_reader' object is a functor object, that populates an object passed in with fields read from the database, if there're any and moves the read pointer to the next record. If no read has been done, the functor object returns 'false':

	vector<user> records;
	
	for (user record; users_reader(record); )
		records.push_back(record);

And as simple as that you'll have records vector populated with all the records in the 'Users' table.
A more complex form of reading is when a condition is involved. A condition is a C++ expression that allows you to bind C++ variables with the parameters in the query. Like this:

	std::string last_name_filter = "Burr";
	vector<user> records;
	auto filtered_users_reader = tx.select<user>(sql2xx::c(&user::last_name) == sql2xx::p(last_name_filter));
	
	for (user record; filtered_users_reader(record); )
		records.push_back(record);

Now, if no other writes to Users occurred since the insertion operation above, the records will be populated with two users with the last name "Burr".
You can build any where expression by using logical operators, for instance:

	auto filtered_users_reader = tx.select<user>(
 		sql2xx::c(&user::last_name) == sql2xx::p(last_name_filter) && sql2xx::c(&user::first_name) == sql2xx::p(first_name_filter));

### Partial updates (UPDATE ... SET ... WHERE statement)
sql2++ allows you to partially update records in the table without involving whole structure writes. You can do this by binding parameters and update values into an executable updater. Here's how:

	std::string last_name_filter = "Burr"; // update every user with last name 'Burr'
 	std::string new_email = ""; // wipe the value out
	auto update = tx.update<user>(
 		sql2xx::c(&user::last_name) == sql2xx::p(last_name_filter), // A WHERE clause to update only the records matching the condition.
   		&user::email, new_email // A pair: a field (column) and a reference to the value to set. You can specify as many pairs, as you need.
	)

	last_name_filter = "Dylan";
	new_email = "dylan@gmail.com";
	update.reset(); // Tell sql2xx to update bindings to any parameter (via sql2xx::p) and value to set.
	update.execute();

In this way you can reuse the update statement.
Please note, that if you plan to use <statement>.reset() function you must supply references to the objects whose lifetime spans at least to the point where you call reset().

### Reading (advanced)
You can read records from joined tables as well, as you can read single tables. To do this you use tuples and WHERE clauses that bind them into an inner join.
Let's create two tables:

	struct employee
	{
		int id;
		std::string first_name;
		std::string last_name;
		std::string email;
     		int company_id; // Foreign key to a record in company.
	};

	struct company
	{
 		int id;
   		std::string name;
	}

	template <typename VisitorT>
	void describe(VisitorT &visitor, employee *)
	{
		visitor("Employees");
		visitor(sql2xx::identity, &employee::id, "ID");
		visitor(&employee::first_name, "FirstName");
		visitor(&employee::last_name, "LastName");
		visitor(&employee::email, "Email");
  		visitor(&employee::company_id, "CompanyID");

		// Here we specify the nature of the relationship between an employee and their company. Once a company gets deleted,
  		// all its employees will be deleted as well.
		visitor << sql2xx::foreign_key_cascade<company> << &employee::company_id << &company::id;
	}

	template <typename VisitorT>
	void describe(VisitorT &visitor, company *)
	{
		visitor("Companies");
		visitor(sql2xx::identity, &company::id, "ID");
		visitor(&company::name, "CompanyName");
	}

	...
	// Now, somewhere in the code:
 	typedef std::tuple<employee, company> employee_full;

	auto reader = t->select<employee_full>(
		sql2xx::c<1 /*second element of the tuple*/>(&company::id)
			== sql2xx::c<0 /*first element of the tuple*/>(&employee::company_id)
   		&& sql2xx::c<1>(&company::name) == sql2xx::p<const string /*as we have parameter in-place*/>("Microsoft")
	);
 	...

The snippet above will read all the employees from the company named 'Microsoft'.


