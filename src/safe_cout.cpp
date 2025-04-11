#include "safe_cout.hpp"

SafeCout& SafeCout::operator<<(std::ostream& (*func)(std::ostream&))
{
	std::lock_guard<std::mutex> lock(_stdoutMutex);
	std::cout << func;
	return *this;
}

