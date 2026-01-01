/*
Copyright 2014 Stas'M Corp.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#pragma once

#include "stdafx.h"
#include <Windows.h>
#include <string>
#include <vector>
#include <map>

#define MAX_STRING_LEN 255

typedef struct _INI_VAR_STRING
{
	char Name[MAX_STRING_LEN];
	char Value[MAX_STRING_LEN];
} INI_VAR_STRING, *PINI_VAR_STRING;

typedef struct _INI_VAR_DWORD
{
	char Name[MAX_STRING_LEN];
#ifndef _WIN64
	DWORD ValueDec;
	DWORD ValueHex;
#else
	DWORD64 ValueDec;
	DWORD64 ValueHex;
#endif
} INI_VAR_DWORD, *PINI_VAR_DWORD;

typedef struct _INI_VAR_BYTEARRAY
{
	char Name[MAX_STRING_LEN];
	BYTE ArraySize;
	char Value[MAX_STRING_LEN];
} INI_VAR_BYTEARRAY, *PINI_VAR_BYTEARRAY;

typedef struct _INI_SECTION_VARLIST_ENTRY
{
	char String[MAX_STRING_LEN];
} INI_SECTION_VARLIST_ENTRY, *PINI_SECTION_VARLIST_ENTRY;

typedef struct _INI_SECTION_VARLIST
{
	DWORD EntriesCount;
	INI_SECTION_VARLIST_ENTRY *NamesEntries;
	INI_SECTION_VARLIST_ENTRY *ValuesEntries;
} INI_SECTION_VARLIST, *PINI_SECTION_VARLIST;

class INI_FILE
{
private:
	std::map<std::string, std::map<std::string, std::string>> data;

	bool parseLine(const std::string& line, std::string& currentSection);
	void trimString(std::string& str);
	bool hexToBytes(const std::string& hexStr, char* buffer, BYTE& arraySize);
	void wcharToChar(const wchar_t* wstr, char* buffer, size_t bufferSize);

public:
	INI_FILE(wchar_t* filePath);
	~INI_FILE();

	bool SectionExists(char* sectionName);
	bool VariableExists(char* sectionName, char* variableName);
	bool GetVariableInSection(char* sectionName, char* variableName, INI_VAR_STRING* variable);
	bool GetVariableInSection(char* sectionName, char* variableName, INI_VAR_DWORD* variable);
	bool GetVariableInSection(char* sectionName, char* variableName, bool* variable);
	bool GetVariableInSection(char* sectionName, char* variableName, INI_VAR_BYTEARRAY* variable);
	bool GetSectionVariablesList(char* sectionName, INI_SECTION_VARLIST* variablesList);

	bool SectionExists(wchar_t* sectionName);
	bool VariableExists(wchar_t* sectionName, wchar_t* variableName);
	bool GetVariableInSection(wchar_t* sectionName, wchar_t* variableName, INI_VAR_STRING* variable);
	bool GetVariableInSection(wchar_t* sectionName, wchar_t* variableName, INI_VAR_DWORD* variable);
	bool GetVariableInSection(wchar_t* sectionName, wchar_t* variableName, bool* variable);
	bool GetVariableInSection(wchar_t* sectionName, wchar_t* variableName, INI_VAR_BYTEARRAY* variable);
	bool GetSectionVariablesList(wchar_t* sectionName, INI_SECTION_VARLIST* variablesList);

	void FreeSectionVariablesList(INI_SECTION_VARLIST* variablesList);
};
