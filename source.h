
#ifndef SOURCE_H_
#define SOURCE_H_

#include "lexer.h"

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>

#define SETTINGS_FILE_DIRECTORY "./NoteCollectorSettings.txt"

void GetSettings(std::string* _output,
                 std::string* _rootDir,
                 std::string* _todo,
                 std::string* _note);

void CreateSettingsFile(std::ifstream* _file);

void SearchDirectory(std::ofstream* _output, std::string _searchDir);

void PrintLine(std::ofstream* _output,
               ITools::LexerToken& token,
               int lineNumber,
               const char* lineStart,
               bool shouldPrintFileName,
               std::string _filePath);

#endif // !SOURCE_H_
