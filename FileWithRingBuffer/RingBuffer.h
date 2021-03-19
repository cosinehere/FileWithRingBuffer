#pragma once

#include <windows.h>

namespace RingBuffer
{
	template<typename T, size_t buffer_size = 32>
	class RingBuffer
	{
	private:
		T m_buffer[buffer_size];
		size_t m_head;
		size_t m_tail;
		size_t m_used;

		class mutex;
		mutex _mtx;
	public:
		RingBuffer()
		{
			m_head = 0;
			m_tail = 0;
			m_used = 0;
		}
		
		size_t read(T* poutput, size_t readnum)
		{
			_mtx.lock();

			size_t readed = (m_used >= readnum) ? readnum : m_used;
			size_t len = (m_head + readed >= buffer_size) ? (buffer_size - m_head) : readed;
			if (len)
			{
				memcpy(poutput, &m_buffer[m_head], len);
				m_head = (m_head + len) % buffer_size;
			}

			size_t left = readed - len;
			if (left)
			{
				memcpy(&poutput[len], &m_buffer[m_head], left);
				m_head += left;
			}

			m_used -= readed;

			_mtx.unlock();

			return readed;
		}

		size_t write(T* pinput, size_t writenum)
		{
			_mtx.lock();

			size_t written = (buffer_size - m_used > writenum) ? writenum : (buffer_size - m_used);
			size_t len = (m_tail + written >= buffer_size) ? (buffer_size - m_tail) : written;
			if (len)
			{
				memcpy(&m_buffer[m_tail], pinput, len);
				m_tail = (m_tail + len) % buffer_size;
			}

			size_t left = written - len;
			if (left)
			{
				memcpy(&m_buffer[m_tail], &pinput[len], left);
				m_tail += left;
			}

			m_used += written;

			_mtx.unlock();

			return written;
		}

	private:
		class mutex
		{
		private:
			CRITICAL_SECTION p_cs;
		public:
			mutex()
			{
				InitializeCriticalSection(&p_cs);
			}

			~mutex()
			{
				DeleteCriticalSection(&p_cs);
			}

			void lock()
			{
				EnterCriticalSection(&p_cs);
			}

			void unlock()
			{
				LeaveCriticalSection(&p_cs);
			}
		};
	};

}