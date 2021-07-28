#include "FileMap.h"

#include <process.h>

namespace FileMap {

FileMap::FileMap()
{
	m_bOpen = false;
	m_hFile = nullptr;
	m_hMap = nullptr;
	m_pMap = nullptr;

	m_filesize.quad = 0;
	m_fileoffset.quad = 0;
	m_mapsize = 0;
	m_mapoffset = 0;

#ifdef _FILEMAP_USE_RINGBUFFER_
	m_hReadFromFile = nullptr;
	m_bReadFromFile = false;
#endif
}

FileMap::~FileMap()
{
	Close();
}

bool FileMap::Open(LPCTSTR strFile, DWORDLONG offset)
{
	if (m_bOpen == true)
	{
		return false;
	}

	m_hFile = CreateFile(strFile, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (m_hFile == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	m_filesize.low = GetFileSize(m_hFile, &m_filesize.high);
	if (m_filesize.quad == 0 || m_filesize.low == INVALID_FILE_SIZE || offset >= m_filesize.quad)
	{
		CloseHandle(m_hFile);
		return false;
	}

	m_hMap = CreateFileMapping(m_hFile, NULL, PAGE_READONLY, 0, 0, nullptr);
	if (m_hMap == nullptr)
	{
		CloseHandle(m_hFile);
		return false;
	}

	m_mapsize = ((m_filesize.quad - offset) > MAP_SIZE) ? MAP_SIZE : static_cast<DWORD>(m_filesize.quad - offset);
	m_fileoffset.quad = offset & OFFSET_MASK_FILE;
	m_mapoffset = offset & OFFSET_MASK_MAP;
	m_pMap = MapViewOfFile(m_hMap, FILE_MAP_READ, m_fileoffset.high, m_fileoffset.low, m_mapsize);
	if (m_pMap == nullptr)
	{
		CloseHandle(m_hMap);
		CloseHandle(m_hFile);
		return false;
	}

	m_bOpen = true;

#ifdef _FILEMAP_USE_RINGBUFFER_
	m_bReadFromFile = true;
	m_hReadFromFile = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, readfromfilethread, this, 0, NULL));
#endif

	return true;
}

bool FileMap::Open(LPCTSTR strFile, enum_mode mode)
{
	if (m_bOpen == true)
	{
		return false;
	}

	m_eMode = mode;

	DWORD dwFlag = 0;
	DWORD dwShare = 0;
	DWORD dwCreation = 0;
	switch (mode)
	{
	case enum_mode_read:
		dwFlag = GENERIC_READ;
		dwShare = FILE_SHARE_READ;
		dwCreation = OPEN_EXISTING;
		break;
	case enum_mode_write:
		dwFlag = GENERIC_READ | GENERIC_WRITE;
		dwShare = 0;
		dwCreation = CREATE_ALWAYS;
		break;
	case enum_mode_readwrite:
		dwFlag = GENERIC_READ | GENERIC_WRITE;
		dwShare = 0;
		dwCreation = CREATE_NEW;
		break;
	default:
		return false;
	}

	m_hFile = CreateFile(strFile, dwFlag, dwShare, NULL, dwCreation, FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_hFile == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	m_filesize.low = GetFileSize(m_hFile, &m_filesize.high);
	if ((m_filesize.quad == 0 && mode == enum_mode_read) || m_filesize.low == INVALID_FILE_SIZE)
	{
		CloseHandle(m_hFile);
		return false;
	}

	DWORD dwProtect = 0;
	switch (mode)
	{
	case enum_mode_read:
		dwProtect = PAGE_READONLY;
		break;
	case enum_mode_write:
		dwProtect = PAGE_READWRITE;
		break;
	case enum_mode_readwrite:
		dwProtect = PAGE_READWRITE;
		break;
	}

	m_fileoffset.quad = 0;
	m_mapoffset = 0;
	m_mapsize = (mode == enum_mode_read) ? 0 : 65536;
	m_pMap = NULL;
	m_hMap = CreateFileMapping(m_hFile, NULL, dwProtect, 0, m_mapsize, NULL);
	if (m_hMap == NULL)
	{
		CloseHandle(m_hFile);
		return false;
	}

	m_bOpen = true;

	return false;
}

void FileMap::Close()
{
#ifdef _FILEMAP_USE_RINGBUFFER_
	if (m_hReadFromFile != nullptr)
	{
		m_bReadFromFile = false;
		WaitForSingleObject(m_hReadFromFile, 1000);
		CloseHandle(m_hReadFromFile);
	}

	m_buffer.Clean();
#endif

	if (m_bOpen == false)
	{
		return;
	}
	UnmapViewOfFile(m_pMap);
	CloseHandle(m_hMap);
	CloseHandle(m_hFile);

	m_bOpen = false;
}

bool FileMap::Read(unsigned char* poutput, DWORD& readnum)
{
	if (m_bOpen == false || m_filesize.quad <= m_fileoffset.quad + m_mapoffset)
	{
		return false;
	}

	if (m_eMode == enum_mode_write)
	{
		return false;
	}

#ifdef _FILEMAP_USE_RINGBUFFER_
	readnum = m_buffer.Read(poutput, readnum);
#else
	DWORD read = (readnum > m_mapsize - m_mapoffset) ? (m_mapsize - m_mapoffset) : readnum;
	memcpy_s(poutput, readnum * sizeof(unsigned char), reinterpret_cast<unsigned char*>(m_pMap) + m_mapoffset, read * sizeof(unsigned char));
	readnum = read;
	m_mapoffset += read;
	if (m_mapoffset >= m_mapsize)
	{
		return Remap();
	}
#endif

	return true;
}

bool FileMap::Write(unsigned char* pinput, DWORD& writenum)
{
	if (m_bOpen == false || m_filesize.quad <= m_fileoffset.quad + m_mapoffset)
	{
		return false;
	}

	if (m_eMode == enum_mode_read)
	{
		return false;
	}

	DWORD written = (writenum > m_mapsize - m_mapoffset) ? (m_mapsize - m_mapoffset) : writenum;
	memcpy_s(reinterpret_cast<unsigned char*>(m_pMap) + m_mapoffset, written * sizeof(unsigned char*), pinput, written * sizeof(unsigned char));
	writenum = written;
	m_mapoffset += written;
	if (m_mapoffset >= m_mapsize)
	{
		return Remap();
	}
	return true;
}

#ifdef _FILEMAP_USE_RINGBUFFER_
bool FileMap::ReadFromFile()
{
	if (m_bOpen == false)
	{
		return false;
	}

	if (m_mapsize <= m_mapoffset)
	{
		return false;
	}

	unsigned char* pfile = reinterpret_cast<unsigned char*>(m_pMap);
	size_t wrote = m_buffer.Write(pfile, m_mapsize - m_mapoffset);
	m_mapoffset += wrote;
	if (m_mapsize == m_mapoffset)
	{
		bool bRet = Remap();
		return bRet;
	}
	else
	{
		return true;
	}

	return false;
}
#endif

bool FileMap::Remap()
{
	if (m_bOpen == false)
	{
		return false;
	}

	DWORDLONG offset = m_fileoffset.quad + m_mapoffset;
	m_fileoffset.quad = offset & OFFSET_MASK_FILE;
	m_mapoffset = offset & OFFSET_MASK_MAP;

	if (m_pMap != NULL)
	{
		UnmapViewOfFile(m_pMap);
	}
	if (m_eMode == enum_mode_read)
	{
		m_mapsize = ((m_filesize.quad - offset) > MAP_SIZE) ? MAP_SIZE : static_cast<DWORD>(m_filesize.quad - offset);
	}
	else
	{
		m_mapsize = 65536;
	}

	DWORD dwAccess = 0;
	switch (m_eMode)
	{
	case enum_mode_read:
		dwAccess = FILE_MAP_READ;
		break;
	case enum_mode_write:
		dwAccess = FILE_MAP_WRITE;
		break;
	case enum_mode_readwrite:
		dwAccess = FILE_MAP_READ | FILE_MAP_WRITE;
		break;
	}
	m_pMap = MapViewOfFile(m_hMap, dwAccess, m_fileoffset.high, m_fileoffset.low, m_mapsize);
	if (m_pMap == NULL)
	{
		return false;
	}

	return true;
}

#ifdef _FILEMAP_USE_RINGBUFFER_
UINT __stdcall readfromfilethread(LPVOID lpParam)
{
	FileMap* pmap = reinterpret_cast<FileMap*>(lpParam);
	while (pmap->m_bReadFromFile)
	{
		if (pmap->m_buffer.Empty())
		{
			pmap->ReadFromFile();
		}
		else
		{
			Sleep(1);
		}
	}

	return 0;
}
#endif
};
