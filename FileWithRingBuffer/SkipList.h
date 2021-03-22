#pragma once

#define SL_MAXLVL 32
#define SL_P 4
#define SL_S 0xffff
#define SL_PS (SL_S/SL_P)

#include <cstdlib>
#include <ctime>

#include "Mutex.h"

namespace SkipList
{
	template<typename T>
	struct SkipListNode
	{
		unsigned char level;
		T value;
		SkipListNode** next;

		SkipListNode()
		{
			next = NULL;
		}

		SkipListNode(T val, unsigned char lvl, SkipListNode* nxt = NULL)
		{
			value = val;
			level = lvl;
			next = new SkipListNode*[lvl + 1];
			for(int i=0;i<=lvl;i++)
			{
				next[i] = nxt;
			}
		}

		~SkipListNode()
		{
			if (next != NULL)
			{
				delete[] next;
			}
		}
	};

	template<typename T>
	class SkipList
	{
	private:
		SkipListNode<T>* m_head;
		SkipListNode<T>* m_tail;
		int m_level;
		int m_length;

		bool m_init;

		Mutex::Mutex _mtx;

		int RandLevel()
		{
			int lvl = 1;
			while ((rand()&SL_S) < SL_PS)
			{
				++lvl;
			}
			return min(SL_MAXLVL, lvl);
		}
	public:
		SkipList()
		{
			m_init = false;

			Init();
		}

		~SkipList()
		{
			Clean();
		}

		void Init()
		{
			_mtx.lock();

			if (m_init == true)
			{
				_mtx.unlock();
				return;
			}

			srand(static_cast<unsigned int>(time(0)));
			m_level = 0;
			m_length = 0;
			m_tail = new SkipListNode<T>(0, 0);
			m_head = new SkipListNode<T>(0, SL_MAXLVL, m_tail);
			memset(&m_tail->value, 0xff, sizeof(T));

			m_init = true;

			_mtx.unlock();
		}

		void Clean()
		{
			_mtx.lock();

			if (m_init == false)
			{
				_mtx.unlock();
				return;
			}

			m_init = false;

			SkipListNode<T>* p = NULL;
			while (m_head->next[0] != m_tail)
			{
				p = m_head->next[0];
				m_head->next[0] = p->next[0];
				delete p;
			}

			delete m_head;
			delete m_tail;

			_mtx.unlock();
		}

		T FindFirst()
		{
			_mtx.lock();

			if (m_init == false)
			{
				_mtx.unlock();
				return 0;
			}

			T value = 0;
			if (m_head->next[0] != m_tail)
			{
				value = m_head->next[0]->value;
			}

			_mtx.unlock();

			return value;
		}

		T Find(const T& val)
		{
			_mtx.lock();

			if (m_init == false)
			{
				_mtx.unlock();
				return;
			}

			SkipListNode<T>* p = m_head;
			for (int i = m_level; i >= 0; --i)
			{
				while (p->next[i] != m_tail && p->next[i]->value <= val)
				{
					p = p->next[i];
				}
			}

			_mtx.unlock();

			return p->value;
		}

		T FindNext(const T& val)
		{
			_mtx.lock();

			if (m_init == false)
			{
				_mtx.unlock();
				return;
			}

			SkipListNode<T>* p = m_head;
			for (int i = m_level; i >= 0; --i)
			{
				while (p->next[i] != m_tail && p->next[i]->value <= val)
				{
					p = p->next[i];
				}
			}

			_mtx.unlock();

			return p->next[0]->value;
		}

		void Insert(const T& val)
		{
			_mtx.lock();

			if (m_init==false)
			{
				_mtx.unlock();
				return;
			}

			SkipListNode<T>* update[SL_MAXLVL + 1];
			SkipListNode<T>* p = m_head;
			for (int i = m_level; i >= 0; --i)
			{
				while (p->next[i] != m_tail && p->next[i]->value < val)
				{
					p = p->next[i];
				}
				update[i] = p;
			}

			p = p->next[0];
			if (p->value == val)
			{
				_mtx.unlock();
				return;
			}

			int lvl = RandLevel();
			if (lvl > m_level)
			{
				lvl = ++m_level;
				update[lvl] = m_head;
			}

			SkipListNode<T>* node = new SkipListNode<T>(val, lvl);
			for (int i = lvl; i >= 0; --i)
			{
				p = update[i];
				node->next[i] = p->next[i];
				p->next[i] = node;
			}

			++m_length;

			_mtx.unlock();
		}

		void Delete(const T& val)
		{
			_mtx.lock();

			if (m_init == false)
			{
				_mtx.unlock();
				return;
			}

			SkipListNode<T>* update[SL_MAXLVL + 1];
			SkipListNode<T>* p = m_head;
			for (int i = m_level; i >= 0; --i)
			{
				while (p->next[i] != m_tail && p->next[i]->value < val)
				{
					p = p->next[i];
				}
				update[i] = p;
			}

			p = p->next[0];
			if (p->value != val)
			{
				_mtx.unlock();
				return;
			}

			for (int i = 0; i <= m_level; i++)
			{
				if (update[i]->next[i] != p)
				{
					break;
				}
				update[i]->next[i] = p->next[i];
			}

			delete p;

			while (m_level > 0 && m_head->next[m_level] == m_tail)
			{
				--m_level;
			}

			--m_length;

			_mtx.unlock();
		}
	};
}
