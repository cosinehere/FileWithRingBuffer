#pragma once

#include "RingBuffer.h"

namespace FileMap {

constexpr DWORD BUFFER_SIZE = 0x00100000UL;
constexpr DWORD MAP_SIZE = 0x00100000UL;
constexpr DWORDLONG  OFFSET_MASK_HIGH = 0xfffffffffff00000ULL;
constexpr DWORDLONG  OFFSET_MASK_LOW = 0x00000000000fffffULL;

class FileMap
{
private:
	RingBuffer::RingBuffer<unsigned char, BUFFER_SIZE> m_buffer;

	bool m_bOpen;
	HANDLE m_hFile;
	HANDLE m_hMap;
	LPVOID m_pMap;

	union {
		DWORDLONG quad;
		struct {
			DWORD low;
			DWORD high;
		};
	}m_filesize, m_fileoffset;
	DWORD m_mapsize;
	DWORD m_mapoffset;

	HANDLE m_hReadFromFile;
	bool m_bReadFromFile;

public:
	FileMap();
	~FileMap();

	bool Open(LPCTSTR strFile, DWORDLONG offset);
	void Close();
	bool Read(unsigned char* poutput, DWORD& readnum);

private:
	bool ReadFromFile();
	bool Remap();

	friend UINT __stdcall readfromfilethread(LPVOID lpParam);
};

UINT __stdcall readfromfilethread(LPVOID lpParam);

} //namespace FileMap