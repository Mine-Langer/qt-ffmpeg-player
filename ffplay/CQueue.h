#pragma once
#include <queue>
#include <mutex>

template<class T>
class CQueue
{
public:
	CQueue() {}
	
	~CQueue() { Clear(); }

	void Push(T val) 
	{
		std::lock_guard<std::mutex> lock(mut);
		dataQueue.push(val);
	}

	void MaxSizePush(T val, volatile bool* flag, unsigned maxSize = 256)
	{
		int size = 0;

		while (*flag)
		{
			mut.lock();
			size = dataQueue.size();
			mut.unlock();

			if (size >= maxSize)
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			else
				break;
		}

		mut.lock();
		dataQueue.push(val);
		mut.unlock();
	}

	size_t Size()
	{
		std::lock_guard<std::mutex> lock(mut);
		return dataQueue.size();
	}

	bool TryPop(T& val)
	{
		std::lock_guard<std::mutex> lock(mut);
		if (dataQueue.empty())
			return false;
		val = dataQueue.front();
		dataQueue.pop();

		return true;
	}

	void Clear()
	{
		std::lock_guard<std::mutex> lock(mut);
		while (dataQueue.size()>0)
		{
			dataQueue.pop();
		}
	}

private:
	std::queue<T> dataQueue;
	std::mutex mut;
};