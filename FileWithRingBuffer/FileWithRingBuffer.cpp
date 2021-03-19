// FileWithRingBuffer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "RingBuffer.h"

RingBuffer::RingBuffer<char,1024> ringbuf;

int main()
{
	char str[256];
	memset(str, 0, sizeof(str));
	for (int i = 0; i < 100; i++)
	{
		str[i] = i+65;
	}

	ringbuf.write(str, 100);

	ringbuf.read(&str[100], 100);

	std::cout << str;
}
