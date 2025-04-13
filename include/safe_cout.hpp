#ifndef SAFETY_COUT_HPP
#define SAFETY_COUT_HPP

#include <iostream>
#include <mutex>

#include "taskqueue.h"

/*!****************************************************************************
 * \brief
 * SafeCout is a class that wraps std::cout with a mutex to synchronize output
 * between threads which prevents interleaving of output.
 ******************************************************************************/
class SafeCout
{
public:
	
	SafeCout() = default;
	~SafeCout() = default;
	template <typename T>
	SafeCout& operator<<(T const& value);
	SafeCout& operator<<(std::ostream& (*func)(std::ostream&));
};

template <typename T>
SafeCout& SafeCout::operator<<(T const& value)
{

	std::lock_guard<std::mutex> lock(_stdoutMutex);
	std::cout << value;
	return *this;
}


static SafeCout safe_cout;

#endif