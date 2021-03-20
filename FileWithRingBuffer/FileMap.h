#pragma once

#include "RingBuffer.h"

#define BUFFER_SIZE 1024
#define MAP_SIZE 1024

namespace FileMap
{
	class FileMap
	{
	private:
		RingBuffer::RingBuffer<unsigned char, BUFFER_SIZE> m_buffer;

		bool m_bOpen;
		HANDLE m_hFile;
		HANDLE m_hMap;
		LPVOID m_pMap;

		DWORD m_filesize;
		DWORD m_mapsize;
		DWORD m_fileoffset;
		DWORD m_mapoffset;

	public:
		FileMap()
		{
			m_bOpen = false;
			m_hFile = NULL;
			m_hMap = NULL;
			m_pMap = NULL;

			m_filesize = 0;
			m_mapsize = 0;
			m_fileoffset = 0;
			m_mapoffset = 0;
		}

		~FileMap()
		{
			Close();
		}

		bool Open(LPCTSTR strFile, DWORD offset)
		{
			if (m_bOpen == true)
			{
				return false;
			}

			m_hFile = CreateFile(strFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (m_hFile == INVALID_HANDLE_VALUE)
			{
				return false;
			}

			m_filesize = GetFileSize(m_hFile, NULL);
			if (m_filesize == 0 || m_filesize == INVALID_FILE_SIZE || offset >= m_filesize)
			{
				CloseHandle(m_hFile);
				return false;
			}

			m_hMap = CreateFileMapping(m_hFile, NULL, PAGE_READONLY, 0, 0, NULL);
			if (m_hMap == NULL)
			{
				CloseHandle(m_hFile);
				return false;
			}

			m_mapsize = ((m_filesize - offset) > MAP_SIZE) ? MAP_SIZE : (m_filesize - offset);
			m_fileoffset = offset & 0xffff0000;
			m_mapoffset = offset & 0x0000ffff;

			m_pMap = MapViewOfFile(m_hMap, FILE_MAP_READ, 0, m_fileoffset, MAP_SIZE);
			if (m_pMap == NULL)
			{
				CloseHandle(m_hMap);
				CloseHandle(m_hFile);
				return false;
			}

			m_bOpen = true;
		}

		void Close()
		{
			if (m_bOpen == false)
			{
				return;
			}
			UnmapViewOfFile(m_pMap);
			CloseHandle(m_hMap);
			CloseHandle(m_hFile);

			m_bOpen = false;
		}

		bool Read(unsigned char* poutput, DWORD& readnum)
		{
			if (m_bOpen == false)
			{
				return false;
			}

			readnum = m_buffer.Read(poutput, readnum);

			return true;
		}

		void ReadFromFile()
		{
			if (m_bOpen == false)
			{
				return;
			}
		}

		void Remap()
		{
			if (m_bOpen == false)
			{
				return;
			}

			DWORD offset = m_fileoffset + m_mapoffset;
			m_fileoffset = offset & 0xffff0000;
			m_mapoffset = offset & 0x0000ffff;

			UnmapViewOfFile(m_pMap);
			m_mapsize = ((m_filesize - offset) > MAP_SIZE) ? MAP_SIZE : (m_filesize - offset);
			m_pMap = MapViewOfFile(m_hMap, FILE_MAP_READ, 0, m_fileoffset, m_mapsize);
		}
	};
}