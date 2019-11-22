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
/*
class CSizeInfo
{
public:
int m_nThreadNum_Count;
int m_nThreadNum_Offset;
int m_nThreadNum_Sort;
int m_nThreadNum_AS;
unsigned int m_nHashSize_IdxUnit;
unsigned int m_nHashSize_WordList;
int m_nHashTagSize_Merge;
int m_nMaxAtomSearchRetSize;
int m_nRetInfoMaxSize;
int m_nRetBuffSize;
int m_nMinIdxUnitCount_ForComplexId;
_int64  m_nMaxOffsetFileSize;

_int64  m_nMaxMemCount;
_int64  m_nToFreeMemCount;
int m_nSortCompWordLen;
_int64  m_nMaxUnitCount_ToSort;
_int64  m_nMaxUnitCount_ToSearch;

unsigned int m_nSentInfoHashSize;

float m_fBM_K1;
float m_fBM_K2;
float m_fBM_b;

public:
void SizeInfoInit(cJSON *pJSON);
public:
CSizeInfo() {
m_nMinIdxUnitCount_ForComplexId = 0;
m_nThreadNum_Count = 1;
m_nThreadNum_Offset = 1;
m_nThreadNum_Sort = 1;
m_nThreadNum_AS = 1;
m_nHashSize_IdxUnit = 102400;
m_nHashSize_WordList = 10240000;
m_nHashTagSize_Merge = 1024000;
m_nMaxAtomSearchRetSize = 102400;
m_nRetInfoMaxSize = 102400;
m_nRetBuffSize = 102400;
m_nMaxOffsetFileSize = 1024 * 1024 * 1024 * 2;
m_nMaxMemCount = 1024 * 1024 * 1024 * 2;
m_nToFreeMemCount = 1024 * 1024 * 1024 * 200;
m_nSortCompWordLen = 10;
m_nMaxUnitCount_ToSort = m_nMaxMemCount;
m_nMaxUnitCount_ToSearch = m_nMaxUnitCount_ToSort * 100;
m_fBM_K1 = 0.;
m_fBM_b = 0.;
m_fBM_K2 = 0;
};

};


*/

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

	/*
	bool ReadDirs() {
		char szFile[MAX_PATH];
		char szFind[MAX_PATH];
		strcpy(szFind, m_strIn.c_str());
		strcat(szFind, "\\*");
		WIN32_FIND_DATA FindFileData;
		HANDLE hFind = ::FindFirstFile(szFind, &FindFileData);
		if (INVALID_HANDLE_VALUE == hFind)
		{
			std::cout << "Empty folder!" << std::endl;
			return false;
		}

		do
		{
			if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				if (FindFileData.cFileName[0] != '.')
				{
					if (szFile[0])
					{
						std::string filePath = m_strIn.c_str();
						filePath += "\\";
						filePath += FindFileData.cFileName;
						m_vFileList.push_back(filePath);
					}
					else
					{
						std::string filePath = szFile;
						filePath += FindFileData.cFileName;
						m_vFileList.push_back(filePath);
					}
				}
			}
		} while (::FindNextFile(hFind, &FindFileData));

		::FindClose(hFind);
		return true;
	}
	*/
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

	/*
	s = u8"他来到了网易杭研大厦";
	cout << s << endl;
	cout << "[demo] Cut With HMM" << endl;
	jieba.Cut(s, words, true);
	cout << limonp::Join(words.begin(), words.end(), "/") << endl;

	cout << "[demo] Cut Without HMM " << endl;
	jieba.Cut(s, words, false);
	cout << limonp::Join(words.begin(), words.end(), "/") << endl;

	s = u8"我来到北京清华大学";
	cout << s << endl;
	cout << "[demo] CutAll" << endl;
	jieba.CutAll(s, words);
	cout << limonp::Join(words.begin(), words.end(), "/") << endl;

	s = u8"小明硕士毕业于中国科学院计算所，后在日本京都大学深造";
	cout << s << endl;
	cout << "[demo] CutForSearch" << endl;
	jieba.CutForSearch(s, words);
	cout << limonp::Join(words.begin(), words.end(), "/") << endl;

	cout << "[demo] Insert User Word" << endl;
	jieba.Cut("男默女泪", words);
	cout << limonp::Join(words.begin(), words.end(), "/") << endl;
	jieba.InsertUserWord("男默女泪");
	jieba.Cut("男默女泪", words);
	cout << limonp::Join(words.begin(), words.end(), "/") << endl;

	cout << "[demo] CutForSearch Word With Offset" << endl;
	jieba.CutForSearch(s, jiebawords, true);
	cout << jiebawords << endl;

	cout << "[demo] Lookup Tag for Single Token" << endl;
	const int DemoTokenMaxLen = 32;
	char DemoTokens[][DemoTokenMaxLen] = { "拖拉机", "CEO", "123", "。" };
	vector<pair<string, string> > LookupTagres(sizeof(DemoTokens) / DemoTokenMaxLen);
	vector<pair<string, string> >::iterator it;
	for (it = LookupTagres.begin(); it != LookupTagres.end(); it++) {
	it->first = DemoTokens[it - LookupTagres.begin()];
	it->second = jieba.LookupTag(it->first);
	}
	cout << LookupTagres << endl;

	cout << "[demo] Tagging" << endl;
	vector<pair<string, string> > tagres;
	s = u8"我是拖拉机学院手扶拖拉机专业的。不用多久，我就会升职加薪，当上CEO，走上人生巅峰。";
	jieba.Tag(s, tagres);
	cout << s << endl;
	cout << tagres << endl;

	cout << "[demo] Keyword Extraction" << endl;
	const size_t topk = 5;
	vector<cppjieba::KeywordExtractor::Word> keywordres;
	jieba.extractor.Extract(s, keywordres, topk);
	cout << s << endl;
	cout << keywordres << endl;
	return EXIT_SUCCESS;
	*/
}
