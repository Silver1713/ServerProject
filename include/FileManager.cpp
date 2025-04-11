#include "FileManager.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>

std::map<std::string, FileData> FileManager::files{};
std::string FileManager::file_dir{};
std::mutex FileManager::file_mutex;

void FileManager::SetDirectory(std::string dir)
{
	std::lock_guard<std::mutex> lock(file_mutex);
	file_dir = dir;
}

void FileManager::UpdateFiles()
{
	//Iterate through all files in the directory and add them to the map
	std::lock_guard<std::mutex> lock(file_mutex);
	for (const auto& entry : std::filesystem::directory_iterator(file_dir))
	{
		std::string file_name = entry.path().filename().string();
		std::string abs_filepath = entry.path().string();
		std::ifstream fileStream(abs_filepath, std::ios::binary | std::ios::ate);
		size_t file_size = fileStream.tellg();
		fileStream.close();
		FileData file_data{ file_name, file_size, abs_filepath };
		files[file_name] = std::move(file_data);
		ReadFile(file_name);
	}
}

void FileManager::AddFile(std::string file_name, std::shared_ptr<char> data, size_t data_size)
{
	std::lock_guard<std::mutex> lock(file_mutex);
	FileData file_data{ file_name, data_size };
	memcpy(file_data.data.get(), data.get(), data_size);
	files[file_name] = file_data;
}

void FileManager::RemoveFile(std::string file_name)
{
	std::lock_guard<std::mutex> lock(file_mutex);
	files.erase(file_name);
}

void FileManager::ReadFile(std::string name)
{
	//Read file content and store in FileData
	FileData& file = files[name];

	file.ReadFile();
	
}



