// pickacontainerbat.cpp : Defines the entry point for the console application.
//

#include <fstream>
#include <string>

int main()
{
	const std::string outFile = "run_call.bat";
	std::ofstream ofs{ outFile, std::ios::out };

	const int maxRepeat = 1;
	const int countContainer = 10;

	for (int i = 0; i < maxRepeat; i++)
	{
		for (auto& constructType : { "construct-once", "construct-sporadic" })
		{
			for (auto& lookupType : { "lookup-continuously", "lookup-sporadic" })
			{
				for (const std::string& keyFile : { "keys/keys_integer.txt", "keys/keys_string10.txt", "keys/keys_string32.txt" })
				{
					for (auto& valSize : { 4, 16, 64, 256 })
					{
						for (auto& N : { 10, 100, 1000, 10000 })
						{
							for (auto& container : { "vector", "map", "unordered_map", "flat_map" })
							{
								ofs << "call ..\\Release\\pick.a.container.exe ";
								ofs << keyFile << " ";
								ofs << countContainer << " ";
								ofs << N << " ";
								ofs << valSize << " ";
								if (keyFile.find("integer") != std::string::npos)
									ofs << "integer ";
								else
									ofs << "string ";
								ofs << container << " ";
								ofs << constructType << " ";
								ofs << lookupType << std::endl;
							}
						}
					}
				}
			}
		}
	}
}
