#include "flags.h"
#include <iostream>
#include <string.h>

using std::cout;
using std::endl;

namespace
{
    const char whiteSpaces[] = " \t\r\n";

    std::string TrimRight(const std::string& s, const char *chars = whiteSpaces)
    {
        std::string res = s;
        res.erase(s.find_last_not_of(chars) + 1);
        return res;
    }

    std::string TrimLeft(const std::string& s, const char *chars = whiteSpaces)
    {
        std::string res = s;
        res.erase(0, s.find_first_not_of(chars));
        return res;
    }

    std::string Trim(const std::string& s, const char *chars = whiteSpaces)
    {
        return TrimLeft(TrimRight(s, chars), chars);
    }
}

Flags::Flags(int argc, char * argv[], const Options & options) : m_options(options)
{
    ParseArgs(argc, argv);
    ApplyDefaults();
}

const Flags::StringMap & Flags::GetAll() const
{
    return m_args;
}

std::string Flags::Get(const std::string & name) const
{
    StringMap::const_iterator arg = m_args.find(name);
    return arg != m_args.end() ? arg->second : "";
}

bool Flags::Exist(const std::string & name) const
{
    return m_args.find(name) != m_args.end();
}

void Flags::ParseArgs(int argc, char * argv[])
{
    if (argc > 0)
    {
        SetProgramName(argv[0]);
    }

    for (int i = 1; i < argc; ++i)
    {
        ParseArg(argv[i]);
    }
}

void Flags::ParseArg(const char * arg)
{
    while (*arg == '-' && *arg != '\0')
    {
        ++arg;
    }
    std::string str = arg;
    size_t pos = str.find_first_of("=");
    m_args[Trim(str.substr(0, pos))] = (pos != std::string::npos) ? Trim(str.substr(pos + 1)) : "";
}

void Flags::SetProgramName(const std::string & programName)
{
    size_t pos = programName.find_last_of("/\\");
    m_programName = (pos != std::string::npos) ? programName.substr(pos + 1) : programName;
}

void Flags::ApplyDefaults()
{
    for (Options::iterator i = m_options.begin(); i != m_options.end(); ++i)
    {
        if (!Exist(i->name) && i->defaultValue != NULL && *(i->defaultValue) != '\0')
        {
            m_args[i->name] = i->defaultValue;
        }
    }
}

static std::string Align(const std::string& s, size_t len)
{
    int diff = len - s.length();
    if (diff <= 0)
    {
        return s;
    }

    std::string res = s;
    res.insert(res.end(), diff, ' ');

    return res;
}

void Flags::PrintUsage() const
{
    size_t maxNameLen = 0;
    for (Options::const_iterator i = m_options.begin(); i != m_options.end(); ++i)
    {
        maxNameLen = std::max(maxNameLen, strlen(i->name));
    }
    maxNameLen += 3;

    cout << "Usage:" << endl;
    cout << m_programName << " [<option>=<value>...]" << endl;
    cout << "Available options:" << endl;
    for (Options::const_iterator i = m_options.begin(); i != m_options.end(); ++i)
    {
        const char* defVal = (i->defaultValue != NULL && *(i->defaultValue) != '\0') ? i->defaultValue : "<none>";
        std::string name = std::string("-") + i->name + ":";
        cout << "    " << Align(name, maxNameLen) << i->description;
        if (i->defaultValue != NULL && *(i->defaultValue) != '\0')
        {
            cout << ". Default: " << i->defaultValue;
        }
        cout << endl;
    }
    cout << endl;
}
