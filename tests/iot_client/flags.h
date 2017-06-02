#ifndef _FLAGS_H_
#define _FLAGS_H_

#include <string>
#include <vector>
#include <map>

class Flags
{
public:
    class Option
    {
    public:
        const char *name;
        const char *description;
        const char *defaultValue;
    };
    typedef std::vector<Option> Options;
    typedef std::map<std::string, std::string> StringMap;

    Flags(int argc, char *argv[], const Options& options);
    const StringMap& GetAll() const;
    std::string Get(const std::string& name) const;
    bool GetBoolean(const std::string& name) const;
    bool Exist(const std::string& name) const;
    void PrintUsage() const;

private:
    void ParseArgs(int argc, char *argv[]);
    void ParseArg(const char *arg);
    void SetProgramName(const std::string& programName);
    void ApplyDefaults();

    StringMap m_args;
    Options m_options;
    std::string m_programName;
};

#endif // _FLAGS_H_
