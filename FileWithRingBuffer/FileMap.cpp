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

	m_hReadFromFile = nullptr;
	m_bReadFromFile = false;
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
	m_fileoffset.quad = offset & OFFSET_MASK_HIGH;
	m_mapoffset = offset & OFFSET_MASK_LOW;

	m_pMap = MapViewOfFile(m_hMap, FILE_MAP_READ, m_fileoffset.high, m_fileoffset.low, m_mapsize);
	if (m_pMap == nullptr)
	{
		CloseHandle(m_hMap);
		CloseHandle(m_hFile);
		return false;
	}

	m_bOpen = true;

	m_bReadFromFile = true;
	m_hReadFromFile = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, readfromfilethread, this, 0, NULL));

	return true;
}

void FileMap::Close()
{
	if (m_hReadFromFile != nullptr)
	{
		m_bReadFromFile = false;
		WaitForSingleObject(m_hReadFromFile, 1000);
		CloseHandle(m_hReadFromFile);
	}

	m_buffer.Clean();

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

	readnum = m_buffer.Read(poutput, readnum);

	return true;
}


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

bool FileMap::Remap()
{
	if (m_bOpen == false)
	{
		return false;
	}

	DWORDLONG offset = m_fileoffset.quad + m_mapoffset;
	m_fileoffset.quad = offset & OFFSET_MASK_HIGH;
	m_mapoffset = offset & OFFSET_MASK_LOW;

	UnmapViewOfFile(m_pMap);
	m_mapsize = ((m_filesize.quad - offset) > MAP_SIZE) ? MAP_SIZE : static_cast<DWORD>(m_filesize.quad - offset);
	m_pMap = MapViewOfFile(m_hMap, FILE_MAP_READ, m_fileoffset.high, m_fileoffset.low, m_mapsize);
	if (m_pMap == nullptr)
	{
		return false;
	}

	return true;
}

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

};
