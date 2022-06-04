
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

void GetSettings(std::vector<std::string>* _rootDirs, std::string* _output);

void CreateSettingsFile(std::ifstream* _file);

void SearchDirectory(unsigned int _rootStringLen, std::ofstream* _output, std::string _searchDir);

#endif // !SOURCE_H_
