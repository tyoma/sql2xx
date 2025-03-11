#include "file_helpers.h"

//#include <common/formatting.h>
//#include <common/module.h>
//#include <common/path.h>
//#include <test-helpers/constants.h>
#include <memory>
#include <ut/assert.h>

#ifdef _WIN32
	#include <process.h>
	
	#define mkdir _mkdir
	#define rmdir _rmdir
	#define unlink _unlink
	#define getcwd _getcwd

	extern "C" int _mkdir(const char *path, int mode);
	extern "C" int _rmdir(const char *path);
	extern "C" int _unlink(const char *path);
	extern "C" char *_getcwd(char* buffer, int maxlen);

#else
	#include <sys/stat.h>
	#include <unistd.h>

#endif

using namespace std;

namespace sql2xx
{
	namespace tests
	{
		const char *operator *(const string &value)
		{
			const char separators[] = { '\\', '/', '\0' };
			const auto pos = value.find_last_of(separators);

			return value.c_str() + (pos != string::npos ? pos + 1 : 0u);
		}

		string operator &(const string &lhs, const string &rhs)
		{
			if (lhs.empty())
				return rhs;
			if (lhs.back() == '\\' || lhs.back() == '/')
				return lhs + rhs;
			return lhs + '/' + rhs;
		}

		string operator ~(const string &value)
		{
			const char separators[] = { '\\', '/', '\0' };
			const auto pos = value.find_last_of(separators);

			if (pos != string::npos)
				return value.substr(0, pos);
			return string();
		}


		temporary_directory::temporary_directory()
		{
			char buffer[256] = { 0 };

			getcwd(buffer, sizeof buffer);
			const auto base_path = ~(string)getcwd(buffer, sizeof buffer);

			for (unsigned index = 1; index < 1000; ++index)
			{
				char name[32] = { 0 };

				sprintf(name, "test_directory.%03d", index);
				auto temp = base_path & name;
				if (!::mkdir(temp.c_str(), 0700))
				{
					_temp_path = temp;
					return;
				}
				if (EEXIST == errno)
					continue;
				break;
			}
			throw runtime_error("cannot create temporary test directory");
		}

		temporary_directory::~temporary_directory()
		{
			for (auto i = _tracked.begin(); i != _tracked.end(); ++i)
				::unlink((_temp_path & *i).c_str());
			::rmdir(_temp_path.c_str());
		}

		string temporary_directory::path() const
		{	return _temp_path;	}

		string temporary_directory::track_file(const string &filename)
		{
			_tracked.push_back(filename);
			return _temp_path & filename;
		}

		string temporary_directory::copy_file(const string &source)
		{
			const auto destination = track_file(*source);
#ifdef _WIN32
			const auto file_open = [] (const char *path, const char *mode) -> shared_ptr<FILE> {
				const auto f = fopen(path, mode);

				assert_not_null(f);
				return shared_ptr<FILE>(f, &fclose);
			};
			const auto fsource = file_open(source.c_str(), "rb");
			const auto fdestination = file_open(destination.c_str(), "wb");
			char buffer[1000];

			for (auto bytes = sizeof buffer; bytes == sizeof buffer; )
			{
				bytes = fread(buffer, 1, bytes, fsource.get());
				fwrite(buffer, 1, bytes, fdestination.get());
			}
#else
			system(("cp \"" + source + "\" \"" + destination + "\"").c_str());
#endif
			return destination;
		}
	}
}
