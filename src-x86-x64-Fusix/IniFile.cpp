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

#include "stdafx.h"
#include <Windows.h>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <cctype>
#include "IniFile.h"

INI_FILE::INI_FILE(wchar_t* filePath)
{
	HANDLE hFile = CreateFileW(filePath, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		return;
	}

	DWORD fileSize = GetFileSize(hFile, NULL);
	if (fileSize == INVALID_FILE_SIZE || fileSize > 10 * 1024 * 1024)
	{
		CloseHandle(hFile);
		return;
	}

	char* fileRaw = new char[fileSize + 1];
	DWORD bytesRead = 0;
	BOOL status = ReadFile(hFile, fileRaw, fileSize, &bytesRead, NULL);
	CloseHandle(hFile);

	if (!status || bytesRead != fileSize)
	{
		delete[] fileRaw;
		return;
	}

	fileRaw[fileSize] = '\0';

	std::string currentSection = "";
	size_t lineStart = 0;

	for (size_t i = 0; i <= fileSize; i++)
	{
		if (i == fileSize || fileRaw[i] == '\n' || fileRaw[i] == '\r')
		{
			if (i > lineStart)
			{
				std::string line(fileRaw + lineStart, i - lineStart);
				parseLine(line, currentSection);
			}
			
			if (i < fileSize && fileRaw[i] == '\r' && i + 1 < fileSize && fileRaw[i + 1] == '\n')
			{
				i++;
			}
			
			lineStart = i + 1;
		}
	}

	delete[] fileRaw;
}

INI_FILE::~INI_FILE()
{
	data.clear();
}

bool INI_FILE::parseLine(const std::string& line, std::string& currentSection)
{
	std::string trimmedLine = line;
	trimString(trimmedLine);

	if (trimmedLine.empty() || trimmedLine[0] == ';' || trimmedLine[0] == '#')
	{
		return true;
	}

	if (trimmedLine[0] == '[' && trimmedLine.back() == ']')
	{
		currentSection = trimmedLine.substr(1, trimmedLine.length() - 2);
		trimString(currentSection);
		return true;
	}

	size_t equalPos = trimmedLine.find('=');
	if (equalPos != std::string::npos && !currentSection.empty())
	{
		std::string key = trimmedLine.substr(0, equalPos);
		std::string value = trimmedLine.substr(equalPos + 1);
		trimString(key);
		trimString(value);

		if (!key.empty())
		{
			data[currentSection][key] = value;
		}
	}

	return true;
}

void INI_FILE::trimString(std::string& str)
{
	size_t start = str.find_first_not_of(" \t\r\n");
	if (start == std::string::npos)
	{
		str.clear();
		return;
	}

	size_t end = str.find_last_not_of(" \t\r\n");
	str = str.substr(start, end - start + 1);
}

bool INI_FILE::hexToBytes(const std::string& hexStr, char* buffer, BYTE& arraySize)
{
	std::string cleanHex = hexStr;
	trimString(cleanHex);

	if (cleanHex.length() % 2 != 0)
	{
		return false;
	}

	size_t maxBytes = (cleanHex.length() > 32) ? 16 : (cleanHex.length() / 2);
	if (maxBytes > MAX_STRING_LEN)
	{
		maxBytes = MAX_STRING_LEN;
	}

	BYTE byteCount = 0;
	for (size_t i = 0; i < cleanHex.length() && byteCount < maxBytes; i += 2)
	{
		unsigned char highNibble = 0, lowNibble = 0;
		bool valid = true;

		switch (cleanHex[i])
		{
		case '0': highNibble = 0; break;
		case '1': highNibble = 1; break;
		case '2': highNibble = 2; break;
		case '3': highNibble = 3; break;
		case '4': highNibble = 4; break;
		case '5': highNibble = 5; break;
		case '6': highNibble = 6; break;
		case '7': highNibble = 7; break;
		case '8': highNibble = 8; break;
		case '9': highNibble = 9; break;
		case 'A': case 'a': highNibble = 10; break;
		case 'B': case 'b': highNibble = 11; break;
		case 'C': case 'c': highNibble = 12; break;
		case 'D': case 'd': highNibble = 13; break;
		case 'E': case 'e': highNibble = 14; break;
		case 'F': case 'f': highNibble = 15; break;
		default: valid = false; break;
		}

		if (i + 1 < cleanHex.length())
		{
			switch (cleanHex[i + 1])
			{
			case '0': lowNibble = 0; break;
			case '1': lowNibble = 1; break;
			case '2': lowNibble = 2; break;
			case '3': lowNibble = 3; break;
			case '4': lowNibble = 4; break;
			case '5': lowNibble = 5; break;
			case '6': lowNibble = 6; break;
			case '7': lowNibble = 7; break;
			case '8': lowNibble = 8; break;
			case '9': lowNibble = 9; break;
			case 'A': case 'a': lowNibble = 10; break;
			case 'B': case 'b': lowNibble = 11; break;
			case 'C': case 'c': lowNibble = 12; break;
			case 'D': case 'd': lowNibble = 13; break;
			case 'E': case 'e': lowNibble = 14; break;
			case 'F': case 'f': lowNibble = 15; break;
			default: valid = false; break;
			}
		}
		else
		{
			valid = false;
		}

		if (!valid)
		{
			return false;
		}

		unsigned char byteValue = static_cast<unsigned char>((highNibble << 4) | lowNibble);
		buffer[byteCount++] = static_cast<char>(byteValue);
	}

	arraySize = byteCount;
	return true;
}

void INI_FILE::wcharToChar(const wchar_t* wstr, char* buffer, size_t bufferSize)
{
	if (wstr == NULL || buffer == NULL || bufferSize == 0)
	{
		return;
	}

	size_t convertedChars = 0;
	wcstombs_s(&convertedChars, buffer, bufferSize, wstr, _TRUNCATE);
}

bool INI_FILE::SectionExists(char* sectionName)
{
	if (sectionName == NULL)
	{
		return false;
	}

	std::string section(sectionName);
	return data.find(section) != data.end();
}

bool INI_FILE::VariableExists(char* sectionName, char* variableName)
{
	if (sectionName == NULL || variableName == NULL)
	{
		return false;
	}

	std::string section(sectionName);
	std::string variable(variableName);

	auto sectionIt = data.find(section);
	if (sectionIt == data.end())
	{
		return false;
	}

	return sectionIt->second.find(variable) != sectionIt->second.end();
}

bool INI_FILE::GetVariableInSection(char* sectionName, char* variableName, INI_VAR_STRING* variable)
{
	if (sectionName == NULL || variableName == NULL || variable == NULL)
	{
		return false;
	}

	std::string section(sectionName);
	std::string variableStr(variableName);

	auto sectionIt = data.find(section);
	if (sectionIt == data.end())
	{
		return false;
	}

	auto varIt = sectionIt->second.find(variableStr);
	if (varIt == sectionIt->second.end())
	{
		return false;
	}

	strncpy_s(variable->Name, MAX_STRING_LEN, variableName, _TRUNCATE);
	strncpy_s(variable->Value, MAX_STRING_LEN, varIt->second.c_str(), _TRUNCATE);
	return true;
}

bool INI_FILE::GetVariableInSection(char* sectionName, char* variableName, INI_VAR_DWORD* variable)
{
	if (sectionName == NULL || variableName == NULL || variable == NULL)
	{
		return false;
	}

	std::string section(sectionName);
	std::string variableStr(variableName);

	auto sectionIt = data.find(section);
	if (sectionIt == data.end())
	{
		return false;
	}

	auto varIt = sectionIt->second.find(variableStr);
	if (varIt == sectionIt->second.end())
	{
		return false;
	}

	strncpy_s(variable->Name, MAX_STRING_LEN, variableName, _TRUNCATE);

	char* endptr = NULL;
	errno = 0;
#ifndef _WIN64
	variable->ValueDec = static_cast<DWORD64>(strtoul(varIt->second.c_str(), &endptr, 10));
#else
	variable->ValueDec = _strtoui64(varIt->second.c_str(), &endptr, 10);
#endif

	if (errno != 0 || (endptr != NULL && *endptr != '\0' && *endptr != ' '))
	{
		variable->ValueDec = 0;
	}

	errno = 0;
	endptr = NULL;
#ifndef _WIN64
	variable->ValueHex = static_cast<DWORD64>(strtoul(varIt->second.c_str(), &endptr, 16));
#else
	variable->ValueHex = _strtoui64(varIt->second.c_str(), &endptr, 16);
#endif

	if (errno != 0 || (endptr != NULL && *endptr != '\0' && *endptr != ' '))
	{
		variable->ValueHex = 0;
	}

	return true;
}

bool INI_FILE::GetVariableInSection(char* sectionName, char* variableName, bool* variable)
{
	if (sectionName == NULL || variableName == NULL || variable == NULL)
	{
		return false;
	}

	std::string section(sectionName);
	std::string variableStr(variableName);

	auto sectionIt = data.find(section);
	if (sectionIt == data.end())
	{
		return false;
	}

	auto varIt = sectionIt->second.find(variableStr);
	if (varIt == sectionIt->second.end())
	{
		return false;
	}

	*variable = (strtol(varIt->second.c_str(), NULL, 10) != 0);
	return true;
}

bool INI_FILE::GetVariableInSection(char* sectionName, char* variableName, INI_VAR_BYTEARRAY* variable)
{
	if (sectionName == NULL || variableName == NULL || variable == NULL)
	{
		return false;
	}

	std::string section(sectionName);
	std::string variableStr(variableName);

	auto sectionIt = data.find(section);
	if (sectionIt == data.end())
	{
		return false;
	}

	auto varIt = sectionIt->second.find(variableStr);
	if (varIt == sectionIt->second.end())
	{
		return false;
	}

	strncpy_s(variable->Name, MAX_STRING_LEN, variableName, _TRUNCATE);
	memset(variable->Value, 0, MAX_STRING_LEN);

	return hexToBytes(varIt->second, variable->Value, variable->ArraySize);
}

bool INI_FILE::GetSectionVariablesList(char* sectionName, INI_SECTION_VARLIST* variablesList)
{
	if (sectionName == NULL || variablesList == NULL)
	{
		return false;
	}

	std::string section(sectionName);

	auto sectionIt = data.find(section);
	if (sectionIt == data.end())
	{
		return false;
	}

	DWORD count = static_cast<DWORD>(sectionIt->second.size());
	variablesList->EntriesCount = count;
	variablesList->NamesEntries = new INI_SECTION_VARLIST_ENTRY[count];
	variablesList->ValuesEntries = new INI_SECTION_VARLIST_ENTRY[count];

	DWORD index = 0;
	for (const auto& pair : sectionIt->second)
	{
		strncpy_s(variablesList->NamesEntries[index].String, MAX_STRING_LEN, pair.first.c_str(), _TRUNCATE);
		strncpy_s(variablesList->ValuesEntries[index].String, MAX_STRING_LEN, pair.second.c_str(), _TRUNCATE);
		index++;
	}

	return true;
}

bool INI_FILE::SectionExists(wchar_t* sectionName)
{
	if (sectionName == NULL)
	{
		return false;
	}

	char buffer[MAX_STRING_LEN];
	wcharToChar(sectionName, buffer, MAX_STRING_LEN);
	return SectionExists(buffer);
}

bool INI_FILE::VariableExists(wchar_t* sectionName, wchar_t* variableName)
{
	if (sectionName == NULL || variableName == NULL)
	{
		return false;
	}

	char sectionBuffer[MAX_STRING_LEN];
	char variableBuffer[MAX_STRING_LEN];
	wcharToChar(sectionName, sectionBuffer, MAX_STRING_LEN);
	wcharToChar(variableName, variableBuffer, MAX_STRING_LEN);
	return VariableExists(sectionBuffer, variableBuffer);
}

bool INI_FILE::GetVariableInSection(wchar_t* sectionName, wchar_t* variableName, INI_VAR_STRING* variable)
{
	if (sectionName == NULL || variableName == NULL || variable == NULL)
	{
		return false;
	}

	char sectionBuffer[MAX_STRING_LEN];
	char variableBuffer[MAX_STRING_LEN];
	wcharToChar(sectionName, sectionBuffer, MAX_STRING_LEN);
	wcharToChar(variableName, variableBuffer, MAX_STRING_LEN);
	return GetVariableInSection(sectionBuffer, variableBuffer, variable);
}

bool INI_FILE::GetVariableInSection(wchar_t* sectionName, wchar_t* variableName, INI_VAR_DWORD* variable)
{
	if (sectionName == NULL || variableName == NULL || variable == NULL)
	{
		return false;
	}

	char sectionBuffer[MAX_STRING_LEN];
	char variableBuffer[MAX_STRING_LEN];
	wcharToChar(sectionName, sectionBuffer, MAX_STRING_LEN);
	wcharToChar(variableName, variableBuffer, MAX_STRING_LEN);
	return GetVariableInSection(sectionBuffer, variableBuffer, variable);
}

bool INI_FILE::GetVariableInSection(wchar_t* sectionName, wchar_t* variableName, bool* variable)
{
	if (sectionName == NULL || variableName == NULL || variable == NULL)
	{
		return false;
	}

	char sectionBuffer[MAX_STRING_LEN];
	char variableBuffer[MAX_STRING_LEN];
	wcharToChar(sectionName, sectionBuffer, MAX_STRING_LEN);
	wcharToChar(variableName, variableBuffer, MAX_STRING_LEN);
	return GetVariableInSection(sectionBuffer, variableBuffer, variable);
}

bool INI_FILE::GetVariableInSection(wchar_t* sectionName, wchar_t* variableName, INI_VAR_BYTEARRAY* variable)
{
	if (sectionName == NULL || variableName == NULL || variable == NULL)
	{
		return false;
	}

	char sectionBuffer[MAX_STRING_LEN];
	char variableBuffer[MAX_STRING_LEN];
	wcharToChar(sectionName, sectionBuffer, MAX_STRING_LEN);
	wcharToChar(variableName, variableBuffer, MAX_STRING_LEN);
	return GetVariableInSection(sectionBuffer, variableBuffer, variable);
}

bool INI_FILE::GetSectionVariablesList(wchar_t* sectionName, INI_SECTION_VARLIST* variablesList)
{
	if (sectionName == NULL || variablesList == NULL)
	{
		return false;
	}

	char buffer[MAX_STRING_LEN];
	wcharToChar(sectionName, buffer, MAX_STRING_LEN);
	return GetSectionVariablesList(buffer, variablesList);
}

void INI_FILE::FreeSectionVariablesList(INI_SECTION_VARLIST* variablesList)
{
	if (variablesList == NULL)
	{
		return;
	}

	if (variablesList->NamesEntries != NULL)
	{
		delete[] variablesList->NamesEntries;
		variablesList->NamesEntries = NULL;
	}

	if (variablesList->ValuesEntries != NULL)
	{
		delete[] variablesList->ValuesEntries;
		variablesList->ValuesEntries = NULL;
	}

	variablesList->EntriesCount = 0;
}
