// pick.a.container.cpp : Defines the entry point for the console application.
//

#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <set>
#include <unordered_map>
#include <vector>
#include <boost/format.hpp>
#include <boost/container/flat_map.hpp>


template <typename T> T convert(const std::string& s)
{
	if constexpr (std::is_same_v<T, int>)
		return std::atoi(s.c_str());
	else
		return s;
}

template <typename T> int getSize(const std::string& s)
{
	if constexpr (std::is_same_v<T, int>)
		return sizeof(int);
	else
		return s.length();
}

template <typename Pair, typename T> void reserve(T& data, int r)
{
	if constexpr (std::is_same_v<T, std::vector<Pair>>)
		data.reserve(r);
}

template <typename V, typename K, typename T> void emplace(T& data, const K& key)
{
	if constexpr (std::is_same_v<T, std::vector<std::pair<K, V>>>)
		data.emplace_back(std::make_pair(key, V{}));
	else
		data.emplace(key, V{});
}

template <typename V, typename K, typename T> char lookup(T& data, const K& key)
{
	if constexpr (std::is_same_v<T, std::vector<std::pair<K, V>>>)
	{
		auto it = std::find_if(data.begin(), data.end(), [&key](const auto& x) { return x.first == key; });
		return it->second[0];
	}
	else
	{
		auto it = data.find(key);
		return it->second[0];
	}
}

template <typename V, typename K, typename T> std::string container_name(T& data)
{
	if constexpr (std::is_same_v<T, std::vector<std::pair<K, V>>>)
		return "vector";
	else if constexpr (std::is_same_v<T, std::map<K, V>>)
		return "map";
	else if constexpr (std::is_same_v<T, std::unordered_map<K, V>>)
		return "unordered_map";
	else if constexpr (std::is_same_v<T, boost::container::flat_map<K, V>>)
		return "flat_map";
	else
		return "unknown";
}

template <typename KeyType, typename Container, int value_size>
void run(const std::string& keyFile, int M, int N, int constructType, int lookupType)
{
	using ValType = std::array<unsigned char, value_size>;

	constexpr int MAX_N = 100000;
	int key_size = 0;
	int padding_size = 0;

	const std::string outFile = "statistics.txt";
	std::ofstream ofs{ outFile, std::ios::out | std::ios::app };

	std::ifstream ifs{ keyFile };
	std::vector<KeyType> keys;
	keys.reserve(M * N);
	while (static_cast<int>(keys.size()) < M * N)
	{
		std::string s;
		ifs >> s;
		keys.emplace_back(convert<KeyType>(s));
		if (key_size == 0)
			key_size = getSize<KeyType>(s);
	}
	padding_size = (value_size + key_size) * (MAX_N - N) / N;

	std::list<std::unique_ptr<char[]>> padding;

	std::chrono::nanoseconds paddingTime{};

	const auto min_running = std::chrono::seconds(3);
	uint64_t sampling = 0;
	auto now = std::chrono::high_resolution_clock::now();

	// Construction
	std::vector<Container> data(M);
	for (int s = 0; ; s++)
	{
		padding.clear();
		for (auto& d : data)
		{
			d.clear();
			reserve<std::pair<KeyType, ValType>>(d, N);
		}

		int count = 0;
		if (constructType == 0)
		{
			for (int i = 0; i < M; i++)
			{
				auto& d = data[i];
				for (int j = 0; j < N; j++)
				{
					emplace<ValType>(d, keys.at(count));
					count++;
				}

				// Padding
				auto beforePadding = std::chrono::high_resolution_clock::now();
				for (int j = 0; j < N; j++)
				{
					padding.emplace_back(std::make_unique<char[]>(padding_size));
				}
				paddingTime += (std::chrono::high_resolution_clock::now() - beforePadding);
			}
		}
		else
		{
			for (int j = 0; j < N; j++)
			{
				for (int i = 0; i < M; i++)
				{
					auto& d = data[i];
					emplace<ValType>(d, keys.at(count));
					count++;

					// Padding
					auto beforePadding = std::chrono::high_resolution_clock::now();
					padding.emplace_back(std::make_unique<char[]>(padding_size));
					paddingTime += (std::chrono::high_resolution_clock::now() - beforePadding);
				}
			}
		}

		if (std::chrono::high_resolution_clock::now() - now > min_running)
		{
			sampling = (s + 1) * M;
			break;
		}
	}

	auto duration = std::chrono::high_resolution_clock::now() - now;
	duration -= paddingTime; // Compensate the time taken for padding allocation.
	auto micro = std::chrono::duration_cast<std::chrono::microseconds>(duration);

	ofs << keyFile << "," << M << "," << N << "," << value_size << "," << key_size << ","
		<< container_name<ValType, KeyType>(data[0]) << "," << constructType << "," << (double)micro.count() / sampling << ",";


	// Some data to load on cache (16 MB)
	const int dummySize = 16 * 1024 * 1024;
	auto dummySrc = std::make_unique<char[]>(dummySize);
	auto dummyDst = std::make_unique<char[]>(dummySize);
	std::chrono::nanoseconds dummyTime{};

	// Prepare random number generator
	std::random_device random_device;
	std::mt19937 engine{ random_device() };
	auto random = [&engine](int lb, int ub)
	{
		std::uniform_int_distribution<int> dist(lb, ub);
		return dist(engine);
	};


	// Create look up data
	std::vector<std::vector<KeyType>> lookups(M);

	{
		int count = 0;
		for (auto& l : lookups)
		{
			for (int j = 0; j < N; j++)
			{
				l.push_back(keys.at(count));
				count++;
			}
		}
		for (auto& l : lookups)
		{
			for (int j = 0; j < N; j++)
			{
				int n = random(0, N - 1);
				int m = random(0, N - 1);
				std::swap(l[n], l[m]);
			}
		}
	}


	// Look up
	now = std::chrono::high_resolution_clock::now();
	sampling = 0;

	int ret = 0;
	for (int s = 0; ; s++)
	{
		if (lookupType == 0)
		{
			for (int i = 0; i < M; i++)
			{
				auto& dd = data[i];
				auto& ll = lookups[i];
				for (int j = 0; j < N; j++)
				{
					ret += lookup<ValType>(dd, ll[j]);
				}

				// Load a lot of data to cache.
				auto beforeDummy = std::chrono::high_resolution_clock::now();
				dummySrc[0] = ret;
				dummySrc[dummySize - 1] = ret;
				memcpy(&dummyDst[0], &dummySrc[0], dummySize);
				dummyTime += (std::chrono::high_resolution_clock::now() - beforeDummy);
			}
			if (std::chrono::high_resolution_clock::now() - now > min_running)
			{
				sampling = M * N * (s + 1);
				break;
			}
		}
		else
		{
			for (int j = 0; j < N; j++)
			{
				for (int i = 0; i < M; i++)
				{
					auto& dd = data[i];
					auto& ll = lookups[i];
					ret += lookup<ValType>(dd, ll[j]);
				}

				// Load a lot of data to cache.
				auto beforeDummy = std::chrono::high_resolution_clock::now();
				dummySrc[0] = ret;
				dummySrc[dummySize - 1] = ret;
				memcpy(&dummyDst[0], &dummySrc[0], dummySize);
				dummyTime += (std::chrono::high_resolution_clock::now() - beforeDummy);

				if (std::chrono::high_resolution_clock::now() - now > min_running)
				{
					sampling = (j + 1) * M * (s + 1);
					break;
				}
			}
			if (sampling > 0)
			{
				break;
			}
		}
	}

	std::cout << ret << std::endl;
	duration = std::chrono::high_resolution_clock::now() - now;
	duration -= dummyTime; // Compensate the time taken for dummy access.
	micro = std::chrono::duration_cast<std::chrono::microseconds>(duration);

	ofs << lookupType << "," << (double)micro.count() / sampling << std::endl;
}

template<typename KeyType, int value_size> void runForContainer(const char* container, const std::string& keyFile, int M, int N, int constructType, int lookupType)
{
	using ValType = std::array<unsigned char, value_size>;

	if (strcmp(container, "map") == 0)
	{
		using Container = std::map<KeyType, ValType>;
		run<KeyType, Container, value_size>(keyFile, M, N, constructType, lookupType);
	}
	if (strcmp(container, "vector") == 0)
	{
		using Container = std::vector<std::pair<KeyType, ValType>>;
		run<KeyType, Container, value_size>(keyFile, M, N, constructType, lookupType);
	}
	if (strcmp(container, "unordered_map") == 0)
	{
		using Container = std::unordered_map<KeyType, ValType>;
		run<KeyType, Container, value_size>(keyFile, M, N, constructType, lookupType);
	}
	if (strcmp(container, "flat_map") == 0)
	{
		using Container = boost::container::flat_map<KeyType, ValType>;
		run<KeyType, Container, value_size>(keyFile, M, N, constructType, lookupType);
	}
}

template<typename KeyType> void runForValueSize(const char* container, const std::string& keyFile, int M, int N, int value_size, int constructType, int lookupType)
{
	if (value_size == 4)
	{
		runForContainer<KeyType, 4>(container, keyFile, M, N, constructType, lookupType);
	}
	if (value_size == 16)
	{
		runForContainer<KeyType, 16>(container, keyFile, M, N, constructType, lookupType);
	}
	if (value_size == 64)
	{
		runForContainer<KeyType, 64>(container, keyFile, M, N, constructType, lookupType);
	}
	if (value_size == 256)
	{
		runForContainer<KeyType, 256>(container, keyFile, M, N, constructType, lookupType);
	}
}

int main(int argc, char** argv)
{
	if (argc < 9)
		return 0;

	const std::string keyFile = argv[1]; // "keys/keys_string32.txt"; // integer,8,16,32
	const int M = atoi(argv[2]); // 10; // # of container instances
	const int N = atoi(argv[3]);// 1000; // # of elements of each container 10,100,1000,10000,100000
	const int value_size = atoi(argv[4]); // 4,16,64,256

	int constructType = 0;
	if (strcmp(argv[7], "construct-sporadic") == 0)
	{
		constructType = 1;
	}

	int lookupType = 0;
	if (strcmp(argv[8], "lookup-sporadic") == 0)
	{
		lookupType = 1;
	}

	if (strcmp(argv[5], "string") == 0)
	{
		using KeyType = std::string;
		runForValueSize<KeyType>(argv[6], keyFile, M, N, value_size, constructType, lookupType);
	}
	else
	{
		using KeyType = int;
		runForValueSize<KeyType>(argv[6], keyFile, M, N, value_size, constructType, lookupType);
	}
}
