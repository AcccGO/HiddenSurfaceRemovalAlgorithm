#pragma once
#include <string>
#include <vector>

namespace analyze_obj_file
{
    using namespace std;

    int GetWordLength(const string &str, int start);

    vector<string> Split(const string &s, const string &seperator);

    class AnalyzeObjFile
    {
    private:
        string str;
        int index;

    public:
        AnalyzeObjFile(string str);
        AnalyzeObjFile();
        ~AnalyzeObjFile();

        void init(string str);
        void SkipDelimiters();
        void SkipToNextWord();
        string GetWord();
        int GetInt();
        float GetFloat();
    };
}
