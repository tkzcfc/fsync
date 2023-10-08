#pragma once

#include <vector>
#include <string>

class Config
{
public:

	static Config& instance();

	Config();

	std::vector<std::string> srcIgnores;
	std::vector<std::string> dstIgnores;

	// �����ļ�ɾ��
	bool disableFileDeletion;
	// �̳߳��߳�����
	int threadNum;
};

