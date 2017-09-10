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
//#include <xmmintrin.h>


const std::string s_outFile = "statistics.txt";
const auto s_minRunning = std::chrono::seconds(1);

std::chrono::nanoseconds timeFunction(const std::function<void()>& f)
{
	auto now = std::chrono::high_resolution_clock::now();
	f();
	return std::chrono::high_resolution_clock::now() - now;
}

void removeAnomalies(std::chrono::nanoseconds& t, uint64_t& sampling, std::vector<std::chrono::nanoseconds>& times)
{
	uint64_t m = sampling / times.size();
	size_t anomalies = static_cast<size_t>(times.size() * 0.05); // 5% of anomalies.
	std::partial_sort(times.begin(), times.begin() + anomalies, times.end(), [](auto& a, auto& b) { return a > b; });
	sampling -= (anomalies * m);
	t -= std::accumulate(times.begin(), times.begin() + anomalies, std::chrono::nanoseconds{}, [](auto a, auto b) { return a + b; });
}

template <typename K> void generateLookups(const std::vector<K>& keys, std::vector<std::vector<K>>& lookups)
{
	// Prepare random number generator
	std::random_device randomDevice;
	std::mt19937 engine{ randomDevice() };
	auto random = [&engine](int lb, int ub)
	{
		std::uniform_int_distribution<int> dist(lb, ub);
		return dist(engine);
	};

	int N = keys.size() / lookups.size();

	// Fill keys to lookups in the order of appearance.
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
	}

	// Shuffle the order
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

template <typename K> std::vector<K> loadKeys(const std::string& keyFile, int count)
{
	std::ifstream ifs{ keyFile };

	std::vector<K> keys;
	keys.reserve(count);

	while (static_cast<int>(keys.size()) < count)
	{
		std::string s;
		ifs >> s;
		if (s.empty())
			break;
		keys.emplace_back(convert<K>(s));
	}

	return keys;
}

template <typename T> T convert(const std::string& s)
{
	if constexpr (std::is_same_v<T, int>)
		return std::atoi(s.c_str());
	else
		return s;
}

template <typename T> int getSize(const T& s)
{
	if constexpr (std::is_same_v<T, int>)
		return sizeof(int);
	else
		return s.length();
}

template <typename K, typename V, typename T> void reserve(T& data, int r)
{
	if constexpr (std::is_same_v<T, std::vector<std::pair<K, T>>>)
		data.reserve(r);
	else if constexpr (std::is_same_v<T, boost::container::flat_map<K, V>>)
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
		return it->second.front() + it->second.back();
	}
	else
	{
		auto it = data.find(key);
		return it->second.front() + it->second.back();
	}
}

template <typename V, typename K, typename T> std::string containerName(T& data)
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

template <typename KeyType, typename Container, int valueSize>
void run(const std::string& keyFile, int M, int N, int constructType, int lookupType)
{
	using ValType = std::array<unsigned char, valueSize>;

	constexpr int MAX_N = 100000;
	int keySize = 0;
	int paddingSize = 0;

	std::vector<KeyType> keys = loadKeys<KeyType>(keyFile, M * N);
	if (keys.empty())
		return;

	keySize = getSize(keys.at(0));
	paddingSize = (valueSize + keySize) * (MAX_N - N) / N;

	std::list<std::unique_ptr<char[]>> padding;

	uint64_t sampling = 0;
	std::chrono::nanoseconds time1{}; // Time spent for things we care.
	std::chrono::nanoseconds time2{}; // Time spent for things we don't care.
	std::vector<std::chrono::nanoseconds> times; // I don't want to use anomalies. So, remember all and remove anomalies.

	// Construction
	std::vector<Container> data(M);
	for (;;)
	{
		padding.clear();

		int count = 0;
		auto timeOld = time1;
		if (constructType == 0)
		{
			time1 += timeFunction([&data, N]()
			{
				for (auto& d : data)
				{
					d.clear();
					reserve<KeyType, ValType>(d, N);
				}
			});

			for (int i = 0; i < M; i++)
			{
				time1 += timeFunction([&data, &keys, &count, N, i]()
				{
					auto& d = data[i];
					for (int j = 0; j < N; j++)
					{
						emplace<ValType>(d, keys.at(count));
						count++;
					}
				});

				time2 += timeFunction([paddingSize, &padding, N]
				{
					// Padding
					for (int j = 0; j < N; j++)
					{
						padding.emplace_back(std::make_unique<char[]>(paddingSize));
					}
				});
			}
		}
		else
		{
			time1 += timeFunction([&data]()
			{
				for (auto& d : data)
				{
					d.clear();
					// When elements are being added sporadically, we won't typically know how many we should reserve in advance.
				}
			});

			for (int j = 0; j < N; j++)
			{
				for (int i = 0; i < M; i++)
				{
					time1 += timeFunction([&data, &keys, &count, N, i]()
					{
						auto& d = data[i];
						emplace<ValType>(d, keys.at(count));
						count++;
					});

					// Padding
					time2 += timeFunction([paddingSize, &padding]()
					{
						padding.emplace_back(std::make_unique<char[]>(paddingSize));
					});
				}
			}
		}

		times.emplace_back(time1 - timeOld);
		sampling += M;
		if (time1 + time2 > s_minRunning)
		{
			break;
		}
	}

	removeAnomalies(time1, sampling, times);
	auto micro = std::chrono::duration_cast<std::chrono::microseconds>(time1);

	{
		std::ofstream ofs{ s_outFile, std::ios::out | std::ios::app };
		ofs << keyFile << "," << M << "," << N << "," << valueSize << "," << keySize << ","
			<< containerName<ValType, KeyType>(data[0]) << "," << constructType << "," << (double)micro.count() / sampling << ",";
	}

	// Some data to clear cache (16 MB)
	auto clearCache = std::vector<int64_t>(2 * 1024 * 1024);

	// Create look up data
	std::vector<std::vector<KeyType>> lookups(M);
	generateLookups(keys, lookups);

	// Look up
	time1 = {};
	time2 = {};
	times.clear();
	sampling = 0;

	int64_t ret = 0; // Use this variable to avoid codes get optimized away...
	for (;;)
	{
		if (lookupType == 0)
		{
			auto oldTime = time1;
			for (int i = 0; i < M; i++)
			{
				time1 += timeFunction([&data, &lookups, N, i, &ret]()
				{
					auto& dd = data[i];
					auto& ll = lookups[i];
					for (int j = 0; j < N; j++)
					{
						ret += lookup<ValType>(dd, ll[j]);
					}
				});

				// Load a lot of data to cache.
				time2 += timeFunction([&clearCache, &ret]()
				{
					// I tried _mm_prefetch. It's too slow (note 1 invocation is only for 1 line which is 32B/64B)
					// I tried accumulate. It's just slow. I go with simple one.
					for (auto x : clearCache)
						ret += x;
				});
			}

			times.push_back(time1 - oldTime);
			sampling += (M * N);
			if (time1 + time2 > s_minRunning)
			{
				break;
			}
		}
		else
		{
			for (int j = 0; j < N; j++)
			{
				auto oldTime = time1;

				time1 += timeFunction([&data, &lookups, M, j, &ret]()
				{
					for (int i = 0; i < M; i++)
					{
						auto& dd = data[i];
						auto& ll = lookups[i];
						ret += lookup<ValType>(dd, ll[j]);
					}
				});

				// Load a lot of data to cache.
				time2 += timeFunction([&clearCache, &ret]()
				{
					// I tried _mm_prefetch. It's too slow (note 1 invocation is only for 1 line which is 32B/64B)
					// I tried accumulate. It's just slow. I go with simple one.
					for (auto x : clearCache)
						ret += x;
				});

				times.push_back(time1 - oldTime);
				sampling += M;
				if (time1 + time2 > s_minRunning)
				{
					break;
				}
			}
			if (time1 + time2 > s_minRunning)
			{
				break;
			}
		}
	}

	std::cout << ret << std::endl;

	removeAnomalies(time1, sampling, times);
	micro = std::chrono::duration_cast<std::chrono::microseconds>(time1);

	{
		std::ofstream ofs{ s_outFile, std::ios::out | std::ios::app };
		ofs << lookupType << "," << (double)micro.count() / sampling << std::endl;
	}
}

template<typename KeyType, int valueSize> void runForContainer(
	const std::string& container, const std::string& keyFile,
	int M, int N, int constructType, int lookupType)
{
	using ValType = std::array<unsigned char, valueSize>;

	if (container == "map")
	{
		using Container = std::map<KeyType, ValType>;
		run<KeyType, Container, valueSize>(keyFile, M, N, constructType, lookupType);
	}
	if (container == "vector")
	{
		using Container = std::vector<std::pair<KeyType, ValType>>;
		run<KeyType, Container, valueSize>(keyFile, M, N, constructType, lookupType);
	}
	if (container == "unordered_map")
	{
		using Container = std::unordered_map<KeyType, ValType>;
		run<KeyType, Container, valueSize>(keyFile, M, N, constructType, lookupType);
	}
	if (container == "flat_map")
	{
		using Container = boost::container::flat_map<KeyType, ValType>;
		run<KeyType, Container, valueSize>(keyFile, M, N, constructType, lookupType);
	}
}

template<typename KeyType> void runForValueSize(int valueSize,
	const std::string& container, const std::string& keyFile, int M, int N,
	int constructType, int lookupType)
{
	if (valueSize == 4)
	{
		runForContainer<KeyType, 4>(container, keyFile, M, N, constructType, lookupType);
	}
	if (valueSize == 16)
	{
		runForContainer<KeyType, 16>(container, keyFile, M, N, constructType, lookupType);
	}
	if (valueSize == 64)
	{
		runForContainer<KeyType, 64>(container, keyFile, M, N, constructType, lookupType);
	}
	if (valueSize == 256)
	{
		runForContainer<KeyType, 256>(container, keyFile, M, N, constructType, lookupType);
	}
}

int main(int argc, char** argv)
{
	if (argc < 9)
		return 0;

	const std::string keyFile = argv[1];   // keys/keys_integer.txt, string10.txt, or string32.txt
	const int M = atoi(argv[2]);           // # of container instances (I'm always passing 10 though...)
	const int N = atoi(argv[3]);           // # of elements of each container - 10, 100, 1000, or 10000
	const int valueSize = atoi(argv[4]);  // 4, 16, 64, or 256
	const std::string keyType = argv[5];   // string or integer
	const std::string container = argv[6]; // vector, map, unordered_map, or flat_map
	const int constructType = strcmp(argv[7], "construct-sporadic") == 0 ? 1 : 0;
	const int lookupType = strcmp(argv[8], "lookup-sporadic") == 0 ? 1 : 0;

	if (keyType == "string")
	{
		using KeyType = std::string;
		runForValueSize<KeyType>(valueSize, container, keyFile, M, N, constructType, lookupType);
	}
	else
	{
		using KeyType = int;
		runForValueSize<KeyType>(valueSize, container, keyFile, M, N, constructType, lookupType);
	}
}
