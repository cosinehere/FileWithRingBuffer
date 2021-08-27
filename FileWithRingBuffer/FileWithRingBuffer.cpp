// FileWithRingBuffer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "RingBuffer.h"
#include "FileMap.h"

#include <io.h>
#include <set>
#include <string>

RingBuffer::RingBuffer<char,1024> ringbuf;
FileMap::FileMap filemap;

void FindFiles(const char* path, std::set<std::string>& files)
{
	std::string format = path;
	format.append("\\*");
	struct _finddata_t info;
	intptr_t hfile;
	hfile = _findfirst(format.c_str(), &info);
	if (hfile != -1)
	{
		do {
			if (info.attrib&_A_SUBDIR)
			{
				if (strcmp(info.name, ".") && strcmp(info.name, ".."))
				{
					std::string folder = path;
					folder.append("\\");
					folder.append(info.name);
					FindFiles(folder.c_str(), files);
				}
			}
			else
			{
				std::string fullpath = path;
				fullpath.append("\\");
				fullpath.append(info.name);
				files.insert(fullpath);
			}
		} while (_findnext(hfile, &info) == 0);

		_findclose(hfile);
	}
}

int main()
{
// 	char str[256];
// 	memset(str, 0, sizeof(str));
// 	for (int i = 0; i < 100; i++)
// 	{
// 		str[i] = i+65;
// 	}
// 
// 	ringbuf.Write(str, 100);
// 
// 	ringbuf.Read(&str[101], 100);
// 
// 	std::cout << str << std::endl;
// 	std::cout << &str[101] << std::endl;

// 	filemap.Open("E:\\VBox\\VBox_Win10_1909_x64\\VBox_Win10_1909_x64.vdi", 0);
// 	//filemap.ReadFromFile();
// 	uint8_t buf[1024];
// 	while (1)
// 	{
// 		DWORD dwrd = 1024;
// 		filemap.Read(buf, dwrd);
// 		if (dwrd == 0)
// 		{
// 			if (!filemap.ReadFromFile())
// 			{
// 				break;
// 			}
// 		}
// 	}
	//std::cout << filemap.Read(buf, dwrd) << std::endl;
// 	for (int i = 0; i < 1024; ++i)
// 	{
// 		std::cout << buf[i];
// 	}

// 	filemap.Close();

// 	std::set<std::string> files;
// 	files.clear();
// 
// 	files.insert(std::string("I:\\迅雷下载\\cn_windows_10_consumer_editions_version_1909_updated_jan_2020_x64_dvd_47161f17.iso"));
// 	//FindFiles("E:\\src_3.5_win7_2021_V1.07a\\_Release", files);
// 	DWORDLONG a = GetTickCount64();
// 	int cnt = 0;
// 	for (auto it = files.begin(); it != files.end(); ++it)
// 	{
// 		if (filemap.Open(it->c_str(), 0))
// 		{
// 			FILE* f = nullptr;
// 			fopen_s(&f, "output.txt", "wb");
// 			DWORDLONG size = 0;
// 			++cnt;
// 			//printf("%s\n", it->c_str());
// 			uint8_t* buf = new uint8_t[1024 * 1024 * 16];
// 			while (1)
// 			{
// 				DWORD dwrd = 1024 * 1024 * 16;
// 				if (!filemap.Read(buf, dwrd))
// 				{
// 					break;
// 				}
// 				size += dwrd;
// // 				for (int i = 0; i < dwrd; ++i)
// // 				{
// // 					printf("%c", buf[i]);
// // 				}
// 				fwrite(buf, sizeof(uint8_t), dwrd, f);
// 				//printf("read %lu\n", dwrd);
// 			}
// 			filemap.Close();
// 			fclose(f);
// 			printf("size = %llu\n", size);
// 		}
// 		else
// 		{
// 			//printf("%s\n", it->c_str());
// 		}
// 	}
// 
// 	DWORDLONG b = GetTickCount64();
// 	printf("size %d, opened %d cost %llu\n", files.size(), cnt, b - a);

	filemap.Open("output.txt", FileMap::FileMap::enum_mode_write);
	filemap.Remap();
	unsigned char buf[] = "12345";
	DWORD write = 5;
	filemap.Write(buf, write);
	filemap.Close();
}
