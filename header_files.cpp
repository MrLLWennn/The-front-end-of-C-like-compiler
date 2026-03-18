//
// Created by 86198 on 2024/10/20.
//


#include"header_files.h"



//打开指定文件输入/输出
Return input_file(fstream& inFile, const string& fileName)
{
    inFile.open(fileName);
    if(!inFile)
    {
        cout << "文件" << fileName << "打开失败" << endl;
        return FAILED;
    }
    cout << "文件" << fileName << "打开成功" << endl;
    return SUCCEED;
}


Return output_file(fstream& outFile, const string& fileName)
{
    outFile.open(fileName, ios::out);
    if(!outFile)
    {
        cout << "文件" << fileName << "打开失败" << endl;
        return FAILED;
    }
    cout << "文件" << fileName << "打开成功" << endl;
    return SUCCEED;
}


//关闭输入输出文件
void close_file(fstream &file1, fstream &file2)
{
    cout << "读/写文件关闭" << endl;
    file1.close();
    file2.close();
}


//分割线
void divide_line()
{
    cout << "------------------------------------------------------------------------------" << endl;
}



//去除字符串前后多余的空格
void remove_front_back_blank(string &str)
{
    if (str[0] == ' ')
        str.assign(str, 1, str.length() - 1);		//首字符为空格
    if (str[str.length() - 1] == ' ')
        str.assign(str, 0, str.length() - 1);		//末字符为空格
}


// 字符串分割的辅助函数
vector<string> split_string(const string& production)
{
    vector<string> result;
    size_t start = 0, index = production.find(' ', start);
    while(index != string::npos)
    {
        string token = production.substr(start, index - start);
        remove_front_back_blank(token);
        if (!token.empty())
            result.push_back(token);
        start = index + 1;
        index = production.find(' ', start);
    }
    string token = production.substr(start);
    remove_front_back_blank(token);
    if (!token.empty())
        result.push_back(token);

    return result;
}
