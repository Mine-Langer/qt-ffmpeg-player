#pragma once
#include <mutex>
#include <queue>

template<class T>
class SafeQueue
{
public:
	SafeQueue() {}
	~SafeQueue() {}

	void Push(T& value)
	{
		std::lock_guard<std::mutex> lock(_mut);
		_data.push(value);
	}

	void MaxSizePush(T& value, volatile bool* bFlag, int maxSize = 256)
	{
		int nSize = 0;
		while (*bFlag)
		{
			_mut.lock();
			nSize = _data.size();
			_mut.unlock();
			
			if (nSize > maxSize)
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			else
				break;;
		}

		_mut.lock();
		_data.push(value);
		_mut.unlock();
	}

	bool Pop(T& value)
	{
		std::lock_guard<std::mutex> lock(_mut);
		if (_data.empty())
			return false;

		value = _data.front();
		_data.pop();

		return true;
	}

	void Clear()
	{
		std::lock_guard<std::mutex> lock(_mut);
		while (_data.size() > 0)
		{
			_data.pop();
		}
	}

private:
	std::mutex _mut;
	std::queue<T> _data;
};