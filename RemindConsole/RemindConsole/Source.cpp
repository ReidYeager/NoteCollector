
#include <stdio.h>
#include <fstream>
#include <string>
#include <iostream>
#include <filesystem>
#include <vector>
#include <cinttypes>

#define NOTE_TOKEN " NOTE"
#define TODO_TOKEN " TODO"

// TODO : Validate that found tokens are actually part of comments
// TODO : A lot of general optimization and refactoring

void PrintAllFiles(std::string _initialDir, std::string _searchDir, std::ofstream& _outFile)
{
	for (const auto& entry : std::filesystem::directory_iterator(_searchDir))
	{
		if (entry.is_directory()) // Check subdirectories
		{
			PrintAllFiles(_initialDir, entry.path().u8string(), _outFile);
		}
		else if (entry.path().filename().u8string().compare("TODO_and_Notes.txt") == 0) // Don't record what is in the output file
		{
			continue;
		}
		else
		{
			printf("%s\n", entry.path().u8string().c_str());
			std::ifstream readFile;
			readFile.open(entry.path(), std::ios_base::binary | std::ios_base::ate);

			uint64_t fileSize = readFile.tellg();
			readFile.seekg(0);

			printf("\t%" PRIu64 "\n", fileSize);

			// NOTE : Pretty sure ~18.5 exabyte files won't be a problem any time soon.
			if (fileSize == 0 || fileSize == -1)
				continue;

			char* wholeFile = (char*)malloc(sizeof(char) * fileSize);
			readFile.read(wholeFile, fileSize);
			readFile.seekg(0);

			std::string fileString = wholeFile;
			free(wholeFile);

			bool printedName = false;
			uint64_t linePos = 0, prevLinePos = 0, lineNumber = 0;
			uint64_t todoPos = 0, notePos = 0;
			todoPos = fileString.find(TODO_TOKEN, 0);
			notePos = fileString.find(NOTE_TOKEN, 0);
			while (todoPos != std::string::npos || notePos != std::string::npos)
			{
				// Write this file's name to the output file
				if (!printedName)
				{
					std::string filePath = entry.path().u8string();
					filePath.erase(0, _initialDir.size());

					_outFile.write(filePath.c_str(), filePath.length());
					_outFile.write("\n", 1);
					printedName = true;
				}

				// Determine the line number within the file
				// NOTE : This can be done a lot better.
				lineNumber = 0;
				prevLinePos = linePos;
				linePos = fileString.find("\n", 0);
				while (linePos != std::string::npos && (linePos <= todoPos || todoPos == std::string::npos) && (linePos <= notePos || notePos == std::string::npos))
				{
					prevLinePos = linePos;
					linePos = fileString.find("\n", prevLinePos + 1);
					lineNumber++;
				}
				linePos = prevLinePos;

				// Use the lower note/todo
				// Maintains the same order as the file
				uint64_t& printPos = (todoPos < notePos) ? todoPos : notePos;
				const char* printToken = (todoPos < notePos) ? TODO_TOKEN : NOTE_TOKEN;

				// Retrieve & trim the line with the token
				char readLine[1024];
				readFile.seekg(prevLinePos+1);
				readFile.getline(readLine, 1024);

				std::string trimLine = readLine;
				// Trim left
				trimLine.erase(
					trimLine.begin(),
					std::find_if(
						trimLine.begin(),
						trimLine.end(),
						[](unsigned char ch) {return !std::isspace(ch);}
						)
					);
				// Trim right
				trimLine.erase(
					std::find_if(
						trimLine.rbegin(),
						trimLine.rend(),
						[](unsigned char ch) {return !std::isspace(ch);}
						).base(),
					trimLine.end()
					);

				// Write the type, line number, and line to the output file
				char* number = new char[6];
				int err = sprintf_s(number, 6, "%5" PRIu64, lineNumber);
				if (err < 0)
					printf("sprintf returned error %d\n", err);
				std::string numberString = number;

				std::string finalLine = "\t";
				finalLine = finalLine + "Line ";
				finalLine = finalLine + number;
				finalLine = finalLine + " :--: ";
				finalLine = finalLine + trimLine;
				finalLine = finalLine + "\n";

				_outFile.write(finalLine.c_str(), finalLine.size());

				delete [] number;

				printPos = fileString.find(printToken, printPos + 1);
			}

			readFile.close();
		}
	}
}

int main(int argc, char** argv)
{
	std::filesystem::path sp = std::filesystem::current_path();
	std::string startDir = sp.u8string();
	std::string outDir = startDir + "\\TODO_and_Notes.txt";

	std::ofstream outFile;
	outFile.open(outDir, std::ios_base::out | std::ios_base::binary);

	PrintAllFiles(startDir, startDir, outFile);

	outFile.close();

	return EXIT_SUCCESS;
}

