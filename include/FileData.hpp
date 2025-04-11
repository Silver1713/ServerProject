#ifndef FILE_DATA_HPP
#define FILE_DATA_HPP
#include <memory>
#include <string>

struct FileData {
	std::string abs_filepath;
	std::string file_name;
	std::shared_ptr<char> data;
	size_t data_size;


	FileData();
	FileData(std::string file_name, size_t data_size, std::string abs_path="");
	FileData(const FileData& other);
	FileData(FileData&& other) noexcept;

	FileData& operator=(const FileData& other);
	FileData& operator=(FileData&& other) noexcept;

	void ReadFile();
	~FileData();

};
#endif
