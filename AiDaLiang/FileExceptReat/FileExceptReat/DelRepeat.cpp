#include "stdafx.h"
#include "DelRepeat.h"

DelRepeat::DelRepeat(const std::vector<std::string>& files,const std::string& sRetain)
{
	int index = 0;
	msRetain = sRetain;
	for (auto it = files.begin();it!=files.end();it++,index++)
	{
		mapFiles[index] = *it;

		std::ifstream ifile(*it);
		if(ifile.is_open())
		{
			std::string sLine;
			while (std::getline(ifile,sLine,'\n'))
			{
				if(sLine.length() == 0) continue;

				mapFileData[index].push_back(sLine);
			}

			//std::ofstream* pof = new std::ofstream;
			//fstreams.push_back(pof);
		}

	}
}

DelRepeat::~DelRepeat()
{

}

void DelRepeat::Execute()
{
	for (auto itMap1 = mapFileData.begin();itMap1!=mapFileData.end();itMap1++)
	{
		auto itMap2 = itMap1;
		itMap2 ++;
		for (;itMap2!=mapFileData.end();itMap2++)
		{
			for (auto it1 = itMap1->second.begin();it1!=itMap1->second.end();it1++)
			{
				for (auto it2 = itMap2->second.begin();it2!=itMap2->second.end();)
				{
					if (*it1 == *it2)
					{
						it2 = itMap2->second.erase(it2);
					}
					else
					{
						it2++;
					}
				}
			}
		}
	}

	for (auto itMap = mapFiles.begin();itMap != mapFiles.end();itMap++)
	{
		std::ofstream ofile(itMap->second);
		if (ofile.is_open())
		{
			std::for_each(mapFileData[itMap->first].begin(),mapFileData[itMap->first].end(),[&ofile](std::string sItem){
				ofile<<sItem<<std::endl;
			});
		}
	}
}