#include "FileData.hpp"

#include <filesystem>
#include <fstream>


FileData::FileData() : data_size{ 0 }
{
	data = nullptr;
}

FileData::FileData(std::string file_name, size_t data_size, std::string abs_path) :
	file_name{ file_name },
	data_size{ data_size }
{
	abs_filepath = abs_path;
	data = std::shared_ptr<char>(new char[data_size], std::default_delete<char[]>());
}

FileData::FileData(FileData&& other) noexcept
{
	file_name = std::move(other.file_name);
	data = std::move(other.data);
	data_size = other.data_size;
}

FileData::FileData(const FileData& other)
{
	abs_filepath = other.abs_filepath;
	file_name = other.file_name;
	data = other.data;
	data_size = other.data_size;
}

FileData& FileData::operator=(const FileData& other)
{
	if (this != &other)
	{
		abs_filepath = other.abs_filepath;
		file_name = other.file_name;
		data = other.data;
		data_size = other.data_size;
	}
	return *this;
}

FileData& FileData::operator=(FileData&& other) noexcept
{
	if (this != &other)
	{
		abs_filepath = std::move(other.abs_filepath);
		file_name = std::move(other.file_name);
		data = std::move(other.data);
		data_size = other.data_size;
	}
	return *this;
}

FileData::~FileData()
{
}

void FileData::ReadFile()
{
	std::ifstream fileStream(abs_filepath, std::ios::binary);
	data_size = std::filesystem::file_size(abs_filepath);
	// deallocate the memory
	data.reset();
	data = std::shared_ptr<char>(new char[data_size], std::default_delete<char[]>());
	fileStream.read(data.get(), static_cast<std::streamsize>(data_size));
	fileStream.close();
}



