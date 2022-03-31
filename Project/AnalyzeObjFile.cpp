#include "AnalyzeObjFile.h"

namespace analyze_obj_file
{
    using namespace std;

    AnalyzeObjFile::AnalyzeObjFile(string str)
    {
        this->str   = str;
        this->index = 0;
    }

    AnalyzeObjFile::AnalyzeObjFile() {}
    AnalyzeObjFile::~AnalyzeObjFile() {}

    // Initialize StringParser object.
    void AnalyzeObjFile::init(string str)
    {
        this->str   = str;
        this->index = 0;
    }

    // Skip delimiters.
    void AnalyzeObjFile::SkipDelimiters()
    {
        int len = this->str.size();
        int i;
        for (i = this->index; i < len; ++i) {
            char c = this->str.at(i);
            if (c == '\t' || c == ' ' || c == '(' || c == ')' || c == '"') continue;
            break;
        }
        this->index = i;
    }

    // Skip to the next word.
    void AnalyzeObjFile::SkipToNextWord()
    {
        this->SkipDelimiters();
        int n = GetWordLength(this->str, this->index);
        this->index += (n + 1);
    }

    // Get word.
    string AnalyzeObjFile::GetWord()
    {
        this->SkipDelimiters();
        int n = GetWordLength(this->str, this->index);
        if (n == 0) return "";
        string word = this->str.substr(this->index, n);
        this->index += (n + 1);

        return word;
    }

    // Get integer.
    int AnalyzeObjFile::GetInt()
    {
        int i;
        i = atoi(this->GetWord().c_str());
        return i;
    }

    // Get floating number.
    float AnalyzeObjFile::GetFloat()
    {
        float f;
        f = atof(this->GetWord().c_str());
        return f;
    }

    int GetWordLength(const string &str, int start)
    {
        int n   = 0;
        int len = str.size();
        int i;
        for (i = start; i < len; i++) {
            char c = str.at(i);
            if (c == '\t' || c == ' ' || c == '(' || c == ')' || c == '"') break;
        }
        return i - start;
    }

    vector<string> Split(const string &s, const string &seperator)
    {
        vector<string>    result;
        string::size_type i = 0;

        while (i != s.size()) {
            int flag = 0;
            while (i != s.size() && flag == 0) {
                flag = 1;
                for (string::size_type x = 0; x < seperator.size(); x++)
                    if (s[i] == seperator[x]) {
                        ++i;
                        flag = 0;
                        break;
                    }
            }

            flag                = 0;
            string::size_type j = i;
            while (j != s.size() && flag == 0) {
                for (string::size_type x = 0; x < seperator.size(); x++)
                    if (s[j] == seperator[x]) {
                        flag = 1;
                        break;
                    }
                if (flag == 0) ++j;
            }
            if (i != j) {
                result.push_back(s.substr(i, j - i));
                i = j;
            }
        }
        return result;
    }
}