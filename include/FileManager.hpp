#ifndef FILE_MANAGER_HPP
#define FILE_MANAGER_HPP
#include <memory>
#include <string>
#include <map>
#include <mutex>

#include "FileData.hpp"

//Thread safe file manager
struct FileManager
{
	static std::string file_dir;
	static std::map<std::string, FileData> files;

	static void SetDirectory(std::string dir);

	static void UpdateFiles();
	static void AddFile(std::string file_name, std::shared_ptr<char> data, size_t data_size);

	static void RemoveFile(std::string file_name);

	static void ReadFile(std::string name);

	static std::mutex file_mutex;
};


#endif // !FILE_MANAGER_HPP

