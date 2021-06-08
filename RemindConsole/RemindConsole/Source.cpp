
#include <stdio.h>
#include <fstream>
#include <string>
#include <iostream>
#include <filesystem>
#include <vector>
#include <cinttypes>

#include <thread>

#define NOTE_TOKEN "// NOTE"
#define TODO_TOKEN "// TODO"

// TODO : Validate that found tokens are actually part of comments
// TODO : A lot of general optimization and refactoring

std::string GetFromFile(std::string _initialDir, std::filesystem::directory_entry entry)
{
  std::ifstream readFile;
  readFile.open(entry.path(), std::ios_base::binary | std::ios_base::ate);

  uint64_t fileSize = readFile.tellg();
  readFile.seekg(0);

  if (fileSize == 0 || fileSize == -1)
    return "";

  printf("In: %s\n", entry.path().u8string().c_str());
  printf("\t%" PRIu64 "\n", fileSize);

  char* wholeFile = (char*)malloc(sizeof(char) * fileSize);
  readFile.read(wholeFile, fileSize);
  readFile.seekg(0);

  std::string fileString = wholeFile;
  free(wholeFile);

  std::string FoundNotes("");

  uint64_t linePos = 0, prevLinePos = 0, lineNumber = 0;
  uint64_t todoPos = 0, notePos = 0;
  todoPos = fileString.find(TODO_TOKEN, 0);
  notePos = fileString.find(NOTE_TOKEN, 0);
  while (todoPos != std::string::npos || notePos != std::string::npos)
  {
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
    readFile.seekg(prevLinePos + 1);
    readFile.getline(readLine, 1024);

    std::string trimLine = readLine;
    // Trim left
    trimLine.erase(
      trimLine.begin(),
      std::find_if(
        trimLine.begin(),
        trimLine.end(),
        [](unsigned char ch) {return !std::isspace(ch); }
      )
    );
    // Trim right
    trimLine.erase(
      std::find_if(
        trimLine.rbegin(),
        trimLine.rend(),
        [](unsigned char ch) {return !std::isspace(ch); }
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

    FoundNotes.append(finalLine);

    delete[] number;

    printPos = fileString.find(printToken, printPos + 1);
  }

  readFile.close();

  std::string FinalOut("");
  if (FoundNotes != "")
  {
    // Write this file's name to the output file
    std::string filePath = entry.path().u8string();
    filePath.erase(0, _initialDir.size());
    FinalOut = filePath;
    FinalOut.append("\n");

    FinalOut.append(FoundNotes.c_str());
  }

  return FinalOut;
}

void PrintAllFiles(
  std::filesystem::directory_entry* _entries, const uint32_t _count, const char* _initialDir, std::string* _outFile)
{
  for (uint32_t i = 0; i < _count; i++)
  {
    std::string s = GetFromFile(_initialDir, *(_entries + i));
    //_outFile->write(s.c_str(), s.size());
    _outFile->append(s.c_str());
  }
}

std::vector<std::filesystem::directory_entry> EnumerateFiles(const char* _directory)
{
  std::vector<std::filesystem::directory_entry> entries(0);

  for (const auto& entry : std::filesystem::directory_iterator(_directory))
  {
    if (entry.is_directory()) // Check subdirectories
    {
      // Add all files from the subdirectory to the entries list
      std::vector<std::filesystem::directory_entry> alt =
          EnumerateFiles(entry.path().u8string().c_str());
      entries.resize(entries.size() + alt.size());
      entries.insert(entries.end(), alt.begin(), alt.end());
    }
    else if (entry.path().filename().u8string().compare("TODO_and_Notes.txt") == 0) // Don't record what is in the output file
    {
      continue;
    }
    else
    {
      // TODO : Add file type filtering?
      entries.push_back(entry);
    }
  }

  return entries;
}

int main(int argc, char** argv)
{
  std::filesystem::path sp = std::filesystem::current_path();
  std::string startDir = sp.u8string();
  //std::string startDir = "D:/dev/Ice_REPO";
  std::string outDir = startDir + "/TEST_NOTE_TODO.txt";

  std::ofstream outFile;
  outFile.open(outDir, std::ios_base::out | std::ios_base::binary);

  std::vector<std::filesystem::directory_entry> entries = EnumerateFiles(startDir.c_str());

  uint32_t threadCount = 32;

  std::vector<std::vector<std::filesystem::directory_entry>> threadRations(threadCount);
  uint32_t averageRation = entries.size() / threadCount;
  uint32_t rationOverflow = entries.size() % threadCount;

  printf("total: %u -- avg: %u, over: %u", entries.size(), averageRation, rationOverflow);

  uint32_t ration = averageRation;
  uint32_t start, end;
  for (uint32_t i = 0; i < threadCount; i++)
  {
    if (i == threadCount - 1)
      ration += rationOverflow;

    start = i * averageRation;
    end = start + ration;

    threadRations[i].resize(ration);
    threadRations[i].insert(threadRations[i].begin(), entries.begin() + start, entries.begin() + end);
  }

  std::vector<std::thread> threads;
  threads.reserve(threadCount);
  std::vector<std::string> strings(threadCount);

  for (uint32_t i = 0; i < threadCount; i++)
  {
    threads.emplace_back(std::thread(PrintAllFiles, threadRations[i].data(), threadRations[i].size(), startDir.c_str(), &strings[i]));
  }

  std::string outString("");
  for (uint32_t i = 0; i < threadCount; i++)
  {
    threads[i].join();
    outString.append(strings[i]);
  }

  outFile.write(outString.c_str(), outString.size());
  outFile.close();

  return EXIT_SUCCESS;
}

