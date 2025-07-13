cmake_minimum_required(VERSION 3.13)

project(sql2++)

enable_testing()

if (NOT WIN32)
	set(CMAKE_CXX_STANDARD 11)
endif()

option(SQL2PP_NO_TESTS "Do not build test modules." OFF)

find_package(SQLite3 REQUIRED)

add_library(sql2++ INTERFACE)
target_include_directories(sql2++ INTERFACE .)
target_link_libraries(sql2++ INTERFACE SQLite::SQLite3)

if (NOT SQL2PP_NO_TESTS)
	if (NOT TARGET utee)
		set(UTEE_NO_TESTS ON)
		add_subdirectory(libraries/utee)
	endif()

	add_library(sql2++.tests SHARED
		tests/ConstrainedTablesTests.cpp
		tests/DatabaseDDLTests.cpp
		tests/DatabaseExpressionTests.cpp
		tests/DatabaseTests.cpp
		tests/file_helpers.cpp
		tests/JoiningTests.cpp
		tests/NullableTests.cpp
	)
	target_link_libraries(sql2++.tests sql2++)
	
	add_utee_test(sql2++.tests)
endif()
