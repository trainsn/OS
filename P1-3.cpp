#include <chrono>
#include <iomanip>
#include <string>
#include <string.h>
#include <fstream>
#include <ctime>
#include <sstream>
#include <iostream>
using namespace std::chrono;
using namespace std;

int main()
{
	system_clock::time_point now = system_clock::now();
	time_t time = system_clock::to_time_t(now);
	stringstream stime;
	ifstream in;
	char str[1024], strtime[13];
	int lentime;

	//获取系统时间并将其转换为字符串
	stime << put_time(localtime(&time), "%b %d %H:%M");
	

	sprintf(strtime, stime.str().c_str(), strlen(stime.str().c_str()));
	//cout << strtime << endl;
	int index = 0;
	for (int i = 0; i < strlen(strtime); i++)
		if (strtime[i] == ' ')
		{
		index++;
		if (strtime[i - 2] == '0'&&index == 2)
		{
			strtime[i - 2] = ' ';
			break;
		}		
		}
	//cout << strtime << endl;
	lentime = strlen(strtime);

	//打开日志 
	in.open("/var/log/kern.log");
	if (!in)
	{
		cout << "open failed!" << endl;
		return 0;
	}

	while (!in.eof())
	{
		in.getline(str, 1023); //按行读取
		if (!strncmp(str, strtime, lentime)) //定位到当前时间的日志
		{
			do{ 
				//定位到目标日志				
				if (str[strlen(str) - 2] == '*') //输出开始标记
				{
					cout << str <<endl;
					while (!in.eof())
					{
						in.getline(str, 1023);
						cout << str <<endl;
						if (str[strlen(str) - 2] == '*') //输出结束标记
						{
							in.close();
							return 0;
						}
					}
				}
				in.getline(str, 1023);
			} while (!in.eof());
		}
	}
	//关闭日志
	in.close();

	return 0;
}
