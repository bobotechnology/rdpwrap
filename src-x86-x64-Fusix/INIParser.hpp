#ifndef INIPARSER_HPP
#define INIPARSER_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <bitset>
#include <fstream>
#include <iomanip>
using namespace std;

class INIParser
{
private:
    unordered_map<string, unordered_map<string, string>> data;

    string trim(const string &str)
    {
        size_t first = str.find_first_not_of(" \t");
        size_t last = str.find_last_not_of(" \t");
        if (first == string::npos || last == string::npos)
            return "";
        return str.substr(first, last - first + 1);
    }

    string normalizeSection(const string &section)
    {
        return trim(section);
    }

    string normalizeKey(const string &key)
    {
        return trim(key);
    }

    bool loadInternal(const string &filePath)
    {
        ifstream file(filePath);
        if (!file.is_open())
        {
            return false;
        }

        string line;
        string currentSection;
        while (getline(file, line))
        {
            line = trim(line);
            if (line.empty() || line[0] == '#' || line[0] == ';')
                continue;
            if (line[0] == '[' && line.back() == ']')
            {
                currentSection = normalizeSection(line.substr(1, line.size() - 2));
                continue;
            }
            size_t delimiterPos = line.find('=');
            if (delimiterPos != string::npos)
            {
                string key = normalizeKey(line.substr(0, delimiterPos));
                string value = trim(line.substr(delimiterPos + 1));
                data[currentSection][key] = value;
            }
        }
        return true;
    }

public:
    bool load(const string &filePath)
    {
        data.clear();
        if (!loadInternal(filePath))
        {
            return false;
        }
        return true;
    }

    void reload(const string &filePath)
    {
        loadInternal(filePath);
    }

    string getString(const string &section, const string &name, const string &defaultValue = "")
    {
        string normalizedSection = normalizeSection(section);
        string normalizedKey = normalizeKey(name);

        if (data.find(normalizedSection) != data.end() && data[normalizedSection].find(normalizedKey) != data[normalizedSection].end())
        {
            return data[normalizedSection][normalizedKey];
        }
        return defaultValue;
    }

    int getInt(const string &section, const string &name, int defaultValue = 0)
    {
        string value = getString(section, name, "");
        if (value.empty())
            return defaultValue;
        try
        {
            return stoi(value);
        }
        catch (const exception &)
        {
            return defaultValue;
        }
    }
    unsigned int getDWordHex(const string &section, const string &name, unsigned int defaultValue = 0)
    {
        string value = getString(section, name, "");
        if (value.empty())
            return defaultValue;
        try
        {
            unsigned int hexValue;
            stringstream ss;
            ss << hex << value;
            ss >> hexValue;
            return hexValue;
        }
        catch (const exception &)
        {
            return defaultValue;
        }
    }

    bool getBool(const string &section, const string &name, bool defaultValue = false)
    {
        string value = getString(section, name, "");
        if (value.empty())
            return defaultValue;
        transform(value.begin(), value.end(), value.begin(), ::tolower);
        return (value == "true" || value == "1" || value == "yes");
    }

    string getHexString(const string &section, const string &name, const string &defaultValue = "0x0")
    {
        string value = getString(section, name, defaultValue);
        if (value.empty())
            return defaultValue;

        if (value.size() >= 2 && value[0] == '0' && (value[1] == 'x' || value[1] == 'X'))
        {
            return value;
        }

        try
        {
            unsigned int intValue = stoul(value, nullptr, 16);
            stringstream ss;
            ss << "0x" << hex << intValue;
            return ss.str();
        }
        catch (const exception &)
        {
            return defaultValue;
        }
    }

    unsigned int getHex(const string &section, const string &name, unsigned int defaultValue = 0)
    {
        string value = getString(section, name, "");
        if (value.empty())
            return defaultValue;
        try
        {
            return stoul(value, nullptr, 16);
        }
        catch (const exception &)
        {
            return defaultValue;
        }
    }

    string getDecString(const string &section, const string &name, const string &defaultValue = "0")
    {
        string value = getString(section, name, defaultValue);
        if (value.empty())
            return defaultValue;

        try
        {
            int intValue = stoi(value);
            return to_string(intValue);
        }
        catch (const exception &)
        {
            return defaultValue;
        }
    }

    int getDec(const string &section, const string &name, int defaultValue = 0)
    {
        string value = getString(section, name, "");
        if (value.empty())
            return defaultValue;
        try
        {
            return stoi(value);
        }
        catch (const exception &)
        {
            return defaultValue;
        }
    }

    string getOctString(const string &section, const string &name, const string &defaultValue = "00")
    {
        string value = getString(section, name, defaultValue);
        if (value.empty())
            return defaultValue;

        try
        {
            unsigned int intValue = stoul(value, nullptr, 8);
            stringstream ss;
            ss << "0" << oct << intValue;
            return ss.str();
        }
        catch (const exception &)
        {
            return defaultValue;
        }
    }

    unsigned int getOct(const string &section, const string &name, unsigned int defaultValue = 0)
    {
        string value = getString(section, name, "");
        if (value.empty())
            return defaultValue;
        try
        {
            return stoul(value, nullptr, 8);
        }
        catch (const exception &)
        {
            return defaultValue;
        }
    }

    string getBinString(const string &section, const string &name, const string &defaultValue = "0b0")
    {
        string value = getString(section, name, defaultValue);
        if (value.empty())
            return defaultValue;

        try
        {
            unsigned int intValue = stoul(value, nullptr, 2);
            stringstream ss;
            ss << "0b" << bitset<32>(intValue).to_string();
            return ss.str();
        }
        catch (const exception &)
        {
            return defaultValue;
        }
    }

    unsigned int getBin(const string &section, const string &name, unsigned int defaultValue = 0)
    {
        string value = getString(section, name, "");
        if (value.empty())
            return defaultValue;
        try
        {
            return stoul(value, nullptr, 2);
        }
        catch (const exception &)
        {
            return defaultValue;
        }
    }

    bool validateSection(const string &section)
    {
        string normalizedSection = normalizeSection(section);
        if (data.find(normalizedSection) == data.end())
        {
            return false;
        }
        return true;
    }

    void validateKey(const string &section, const string &key)
    {
        validateSection(section);
        string normalizedSection = normalizeSection(section);
        string normalizedKey = normalizeKey(key);
        if (data[normalizedSection].find(normalizedKey) == data[normalizedSection].end())
        {
            throw runtime_error("键未找到: " + section + "." + key);
        }
    }

    string getStringWithNestedSection(const string &section, const string &name, const string &defaultValue = "")
    {
        string currentSection = section;
        while (!currentSection.empty())
        {
            string value = getString(currentSection, name, "");
            if (!value.empty())
                return value;

            size_t lastDot = currentSection.find_last_of('.');
            if (lastDot == string::npos)
                break;
            currentSection = currentSection.substr(0, lastDot);
        }
        return defaultValue;
    }

    vector<uint8_t> hexToMachineCode(const string &hexString)
    {
        vector<uint8_t> machineCode;
        for (size_t i = 0; i < hexString.length(); i += 2)
        {
            string byteString = hexString.substr(i, 2);
            uint8_t byte = static_cast<uint8_t>(stoul(byteString, nullptr, 16));
            machineCode.push_back(byte);
        }
        return machineCode;
    }

    string machineCodeToHex(const vector<uint8_t> &machineCode)
    {
        stringstream ss;
        for (uint8_t byte : machineCode)
        {
            ss << hex << setw(2) << setfill('0') << static_cast<int>(byte);
        }
        return ss.str();
    }
};

#endif // INIPARSER_HPP