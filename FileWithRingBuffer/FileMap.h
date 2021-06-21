#pragma once

#include "RingBuffer.h"

namespace FileMap {

#ifdef _FILEMAP_USE_RINGBUFFER_
constexpr DWORD BUFFER_SIZE = 0x00100000UL;
#endif
constexpr DWORD MAP_SIZE = 0x10000000UL;
constexpr DWORDLONG  OFFSET_MASK_FILE = 0xfffffffff0000000ULL;
constexpr DWORDLONG  OFFSET_MASK_MAP = 0x000000000fffffffULL;

class FileMap
{
private:
#ifdef _FILEMAP_USE_RINGBUFFER_
	RingBuffer::RingBuffer<unsigned char, BUFFER_SIZE> m_buffer;
#endif

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

#ifdef _FILEMAP_USE_RINGBUFFER_
	HANDLE m_hReadFromFile;
	bool m_bReadFromFile;
#endif

public:
	FileMap();
	~FileMap();

	bool Open(LPCTSTR strFile, DWORDLONG offset);
	void Close();
	bool Read(unsigned char* poutput, DWORD& readnum);

private:
#ifdef _FILEMAP_USE_RINGBUFFER_
	bool ReadFromFile();
#endif
	bool Remap();

#ifdef _FILEMAP_USE_RINGBUFFER_
	friend UINT __stdcall readfromfilethread(LPVOID lpParam);
#endif
};

#ifdef _FILEMAP_USE_RINGBUFFER_
UINT __stdcall readfromfilethread(LPVOID lpParam);
#endif

} //namespace FileMap