// cppjiebaapp.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "./include/cppjieba/Jieba.hpp"
#include "./include/inifile2/inifile.h"
#include <Windows.h>
#include <direct.h>
#include <time.h>

#define MAX_SENT_LEN 1024 * 100

using namespace inifile;
using namespace std;

class CServer;

CServer* g_pServer = NULL;

void PrintUsage(char* psCmd)
{
	printf("Usage:\n");
	printf("%s --batch config.ini inpath outpath\n", psCmd);
	/*
	printf("%s -as psConfig,psAction,psPath\n", psCmd);
	printf("%s -org psOrgDat, psOrgTxt\n", psCmd);
	printf("%s -http psPort, psPath\n", psCmd);
	printf("%s -idxinfo psOffset2ID, psIDInfo, psIdx\n", psCmd);
	*/
}

string basename(string path) {
	if (path.rfind('\\') != string::npos) {
		return path.substr(path.rfind('\\') + 1);
	}
	else if (path.rfind('/') != string::npos) {
		return path.substr(path.rfind('/') + 1);
	}
	else {
		return path;
	}
}

class CConfig
{
public:
	string m_strDICT_PATH;
	string m_strHMM_PATH;
	string m_strUSER_DICT_PATH;
	string m_strIDF_PATH;
	string m_strSTOP_WORD_PATH;
public:
	bool ConfigInit(char* psConfig) {
		IniFile ini;
		if (ini.Load(psConfig) == ERR_OPEN_FILE_FAILED) return false;

		if (ini.GetStringValue("COMMON", "DICT_PATH", &m_strDICT_PATH))           return false;
		if (ini.GetStringValue("COMMON", "HMM_PATH", &m_strHMM_PATH))             return false;
		if (ini.GetStringValue("COMMON", "USER_DICT_PATH", &m_strUSER_DICT_PATH)) return false;
		if (ini.GetStringValue("COMMON", "IDF_PATH", &m_strIDF_PATH))             return false;
		if (ini.GetStringValue("COMMON", "STOP_WORD_PATH", &m_strSTOP_WORD_PATH)) return false;

		return true;
	}
	void ConfigExit() {

	};
public:
	CConfig() {
		m_strDICT_PATH;
		m_strHMM_PATH;
		m_strUSER_DICT_PATH;
		m_strIDF_PATH;
		m_strSTOP_WORD_PATH;
	};

};


class CThreadInfo
{
public:
	unsigned m_nID;
	vector<string> m_vFileList;
	string  m_strOutPath;

public:
	CThreadInfo() {
		m_nID = 0;
	};

};

class CServer
{
public:
	CConfig m_pConfig;

	vector< vector<string> > m_vFileList;

	cppjieba::Jieba* m_jieba;

	vector<string> words;
	vector<cppjieba::Word> jiebawords;
	string s;
	string result;

	bool Initialize(char* psConfig) {
		m_pConfig.ConfigInit(psConfig);


		m_jieba = new cppjieba::Jieba(
			m_pConfig.m_strDICT_PATH,
			m_pConfig.m_strHMM_PATH,
			m_pConfig.m_strUSER_DICT_PATH,
			m_pConfig.m_strIDF_PATH,
			m_pConfig.m_strSTOP_WORD_PATH);


		return true;
	}
	string Seg(string line) {
		return "";
	}

	string Segpos(string line) {
		return "";
	}

	bool Exit() {
		if (m_jieba != NULL) delete m_jieba;
		return true;
	}
public:
	CServer() {
		m_jieba = NULL;
	};
	~CServer() {
	};
};


void ThreadWorker(void *objInp)
{
	CThreadInfo* pThreadInfo;
	pThreadInfo = (CThreadInfo*)objInp;
	int nLine = 0;
	for (unsigned int i = 0; i<pThreadInfo->m_vFileList.size(); i++) {
		FILE *fpInp = NULL, *fpOut = NULL;
		char psOut[MAX_PATH];
		strcpy(psOut, pThreadInfo->m_strOutPath.c_str());
		strcat(psOut, strrchr((char*)pThreadInfo->m_vFileList[i].c_str(), '\\'));
		strcat(psOut, ".seged");

		char szLine[MAX_SENT_LEN];
		szLine[0] = 0;
		if ((fpInp = fopen(pThreadInfo->m_vFileList[i].c_str(), "rb")) == NULL) {
			printf("Error open %s\n", pThreadInfo->m_vFileList[i].c_str());
			return;
		}
		if ((fpOut = fopen(psOut, "wb")) == NULL) {
			printf("Error open %s\n", psOut);
			return;
		}
		vector<string> words;
		vector<pair<string, string> > tagres;
		while (fgets(szLine, MAX_SENT_LEN, fpInp) != NULL) {
			if (szLine[0] == '<') {
				fprintf(fpOut, "%s", szLine);
				continue;
			}

			if (szLine[strlen(szLine) - 1] == '\n') {
				szLine[strlen(szLine) - 1] = 0;
			}
			if (szLine[strlen(szLine) - 1] == '\r') {
				szLine[strlen(szLine) - 1] = 0;
			}

			//g_pServer->m_jieba->Cut(psTmp, words, true);
			g_pServer->m_jieba->Tag(szLine, tagres);
			for (size_t i = 0; i < tagres.size(); i++) fprintf(fpOut, "%s/%s ", tagres[i].first.c_str(), tagres[i].second.c_str());
			fprintf(fpOut, "\n");

			nLine++;
			if (nLine % 10000 == 0) {
				printf("Thread %d Line %d\r", pThreadInfo->m_nID, nLine);
			}
			tagres.clear();
		}
		fclose(fpInp);
		fclose(fpOut);
	}
}


class CClient
{
public:
	//CSizeInfo m_sizeinfo;
	int m_nJobs;
	string m_strIn, m_strOut;
	std::vector<std::string> m_vFileList;	//all files 
	vector< vector<string> > m_vvFileList;	//file vector for thread worker

	bool ReadList() {
		FILE *fpInp = NULL;
		char szLine[MAX_PATH];
		szLine[0] = 0;
		if ((fpInp = fopen(m_strIn.c_str(), "rb")) == NULL) {
			printf("Error open %s\n", m_strIn.c_str());
			return false;
		}
		while (fgets(szLine, MAX_PATH, fpInp) != NULL) {
			if (szLine[strlen(szLine) - 1] == '\n') {
				szLine[strlen(szLine) - 1] = 0;
			}
			if (szLine[strlen(szLine) - 1] == '\r') {
				szLine[strlen(szLine) - 1] = 0;
			}
			if (*szLine == ' ' || *szLine == 0 || *szLine == '#' || *szLine == '\\' || *szLine == '-') {
				continue;
			}
			m_vFileList.push_back(szLine);
		}
		fclose(fpInp);
		return true;
	}
	
	bool SliceFileList()
	{
		if (m_vFileList.size() < m_nJobs) {
			m_nJobs = m_vFileList.size();
		}

		int nPerNum = m_vFileList.size() / m_nJobs;
		int nRemainNum = m_vFileList.size() % m_nJobs;
		if (nRemainNum > 0) nPerNum++;
		int nNo = 0;
		vector<string> m_vFileListTmp;
		for (unsigned int i = 0; i<m_nJobs; i++) {
			m_vFileListTmp.clear();
			for (int j = 0; j<nPerNum; j++) {
				m_vFileListTmp.push_back(m_vFileList[nNo]);
				nNo++;
			}
			m_vvFileList.push_back(m_vFileListTmp);
			if (m_vvFileList.size() >= nRemainNum && nPerNum == m_vFileList.size() / m_nJobs + 1) {
				nPerNum--;
			}
		}
		return true;
	}
	bool Run()
	{
		mkdir(m_strOut.c_str());

		HANDLE* hThread = NULL;
		DWORD* ThreadID = NULL;
		CThreadInfo* pThreadInfo;

		hThread = new HANDLE[m_nJobs];
		ThreadID = new DWORD[m_nJobs];
		pThreadInfo = new CThreadInfo[m_nJobs];

		memset(hThread, 0, sizeof(HANDLE)*m_nJobs);
		memset(ThreadID, 0, sizeof(DWORD)*m_nJobs);
		printf("Creating SEGMENTOR pool with [0 - %d] jobs...\n", m_nJobs);
		for (unsigned int i = 0; i < m_nJobs; i++) {
			pThreadInfo[i].m_strOutPath = m_strOut;
			pThreadInfo[i].m_nID = i;
			for (unsigned int j = 0; j < m_vvFileList[i].size(); j++) pThreadInfo[i].m_vFileList.push_back(m_vvFileList[i][j]);
			hThread[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadWorker, LPVOID(&pThreadInfo[i]), 0, &ThreadID[i]);
		}

		for (unsigned int i = 0; i<m_vFileList.size(); i++) {
			WaitForSingleObject(hThread[i], INFINITE);
		}

		delete[] hThread;
		delete[] ThreadID;
		delete[] pThreadInfo;
		return true;
	}
public:
	CClient() {
		m_nJobs = 0;
	}
	~CClient() {

	}
};

bool Segbatch(char* in, char* out, int jobs)
{
	CClient Client;
	Client.m_strIn = in;
	Client.m_strOut = out;
	Client.m_nJobs = jobs;

	if (!Client.ReadList()) {
		return false;
	}
	if (!Client.SliceFileList()) {
		return false;
	}

	int start, stop;
	start = clock();
	Client.Run();
	stop = clock();
	printf("\nRunning Time：%dms\n", (stop - start));

	return true;
}


bool GlobalInit(char* psConfig)
{
	if (!psConfig) return false;

	if (g_pServer == NULL) {
		g_pServer = new CServer();
	}
	return g_pServer->Initialize(psConfig);
}
int main(int argc, char** argv) {

	if (argc < 6) {
		PrintUsage(argv[0]);
		return 0;
	}

	if (argc == 6 && strcmp((char*)argv[1], "--batch") == 0) {
		GlobalInit(argv[2]);
		Segbatch(argv[3], argv[4], atoi(argv[5]));
		return 1;
	}

	return 0;
}
