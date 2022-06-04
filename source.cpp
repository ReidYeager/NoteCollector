
#include "source.h"
#include "lexer.h"

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>

std::string tokenStartCharacters;
std::vector<std::string> tokenStrings;

std::string outStringLeftCap = "";
std::string outStringRightCap = "";

std::string outStringSeparator = ":";

int main()
{
  std::string outDir;
  std::vector<std::string> rootDirs;

  GetSettings(&rootDirs, &outDir);
  tokenStartCharacters.push_back('\n'); // Used to count lines\

  std::ofstream outFile;
  outFile.open(outDir, std::ios_base::binary);
  outFile.clear();

  char rootBuffer[1024];
  int rootIndex = 0;

  for (std::string& r : rootDirs)
  {
    sprintf(rootBuffer, "%s==========\nRoot %s %s\n\n",
            rootIndex ? "\n" : "",
            outStringSeparator.c_str(),
            r.c_str());
    outFile.write(rootBuffer, strlen(rootBuffer));

    SearchDirectory(r.size(), &outFile, r);

    rootIndex++;
  }

  outFile.close();

  return 0;
}

void GetSettings(std::vector<std::string>* _rootDirs, std::string* _output)
{
  // Settings file =====

  std::ifstream settingsFile;
  settingsFile.open(SETTINGS_FILE_DIRECTORY, std::ios_base::ate | std::ios_base::binary);

  if (!settingsFile.is_open())
  {
    CreateSettingsFile(&settingsFile);
  }

  long long settingsFileSize = settingsFile.tellg();
  settingsFile.seekg(0);

  std::vector<char> settingsText(settingsFileSize);
  settingsFile.read(settingsText.data(), settingsFileSize);

  settingsFile.close();

  // Read settings =====

  std::vector<std::string>& roots = *_rootDirs;

  ITools::Lexer lexer(settingsText);
  ITools::LexerToken token;

  while (!lexer.CompletedStream())
  {
    if (lexer.ExpectString("Token") && lexer.ExpectString("="))
    {
      lexer.ReadThrough('"');
      token = lexer.ReadTo('"');
      tokenStrings.push_back(token.string);
      tokenStartCharacters.push_back(token.string.c_str()[0]);
      lexer.ReadThrough('\n');
    }
    else if (lexer.ExpectString("Root") && lexer.ExpectString("="))
    {
      lexer.ReadThrough('"');
      token = lexer.ReadTo('"');
      roots.push_back(token.string);
      lexer.ReadThrough('\n');
    }
    else if (lexer.ExpectString("Output File") && lexer.ExpectString("="))
    {
      lexer.ReadThrough('"');
      token = lexer.ReadTo('"');
      *_output = token.string;
      lexer.ReadThrough('\n');
    }
    else if (lexer.ExpectString("Output string endcaps") && lexer.ExpectString("="))
    {
      // Left endcap
      lexer.ReadThrough('"');
      token = lexer.ReadTo('"');
      outStringLeftCap = token.string;
      lexer.ReadThrough('"');

      // Right endcap
      lexer.ReadThrough('"');
      token = lexer.ReadTo('"');
      outStringRightCap = token.string;
      lexer.ReadThrough('\n');
    }
    else if (lexer.ExpectString("Output string separator") && lexer.ExpectString("="))
    {
      lexer.ReadThrough('"');
      token = lexer.ReadTo('"');
      outStringSeparator = token.string;
    }
    else
    {
      lexer.NextToken(); // Skip unknown string
    }
  }
}

void CreateSettingsFile(std::ifstream* _file)
{
  std::ofstream outFile;
  outFile.open(SETTINGS_FILE_DIRECTORY, std::ios_base::binary);

  const char* defaultSettings = "Token = \"TODO : \"\n"
                                "Token = \"NOTE : \"\n"
                                "Root = \".\"\n"
                                "Output File = \"ToDo.txt\"\n"
                                "Output string endcaps = \"'\" \"'\"\n"
                                "Output string separator = \":\"\0";


  outFile.write(defaultSettings, strlen(defaultSettings));
  outFile.close();

  _file->open(SETTINGS_FILE_DIRECTORY, std::ios_base::ate | std::ios_base::binary);
}

void SearchDirectory(unsigned int _rootStringLen, std::ofstream* _output, std::string _searchDir)
{
  char lineBuffer[1024];

  for (const auto& entry : std::filesystem::directory_iterator(_searchDir))
  {
    if (entry.is_directory())
    {
      SearchDirectory(_rootStringLen, _output, entry.path().u8string());
      continue;
    }

    printf("Checking : '%s'\n", entry.path().u8string().c_str());

    // Load file =====

    std::ifstream inFile;
    inFile.open(entry.path(), std::ios_base::binary | std::ios_base::ate);

    size_t size = inFile.tellg();
    inFile.seekg(0);

    // TODO : Analyze the files directly from disk rather than copying them into a buffer
    std::vector<char> text(size);
    inFile.read(text.data(), size);

    inFile.close();

    if (text.size() == 0)
    {
      continue;
    }

    // Analyze =====

    ITools::Lexer lexer(text);
    ITools::LexerToken token;

    int index = 0;
    unsigned int lineNumber = 1;
    const char* lineStart = text.data();

    bool shoudPrintFileName = true;

    while (!lexer.CompletedStream())
    {
      index = lexer.ReadToFirst(tokenStartCharacters, &token);

      if (index == -1) {}
      else if (index == tokenStrings.size())
      {
        lineNumber++;
        lineStart = token.end;
        lexer.Read(1); // Don't re-read '\n'
        continue;
      }

      // Multiple tokens may share the same starting character.
      // Check all remaining search tokens
      int matchIndex = -1;
      for (int i = index; i < tokenStrings.size(); i++)
      {
        if (tokenStrings[i][0] == tokenStrings[index][0] &&
             lexer.ExpectString(tokenStrings[i], &token))
        {
          matchIndex = i;
          break;
        }
      }

      if (matchIndex != -1)
      {
        // Print the line =====

        token = lexer.ReadTo('\n');
        std::string line(lineStart, token.end - lineStart);

        if (shoudPrintFileName)
        {
          shoudPrintFileName = false;

          std::string localPath = entry.path().u8string();
          localPath.erase(0, _rootStringLen);
          localPath.append("\n");
          _output->write(localPath.c_str(), localPath.size());
        }

        // Trim left spaces
        line.erase(line.begin(), std::find_if(line.begin(),
                                              line.end(),
                                              [](unsigned char c) { return !std::isspace(c); }));
        // Trim right spaces
        line.erase(std::find_if(line.rbegin(),
                                line.rend(),
                                [](unsigned char c) { return !std::isspace(c); }).base(),
                   line.end());

        sprintf(lineBuffer,
                "  Line %4d %s %s%s%s\n",
                lineNumber,
                outStringSeparator.c_str(),
                outStringLeftCap.c_str(),
                line.c_str(),
                outStringRightCap.c_str());

        _output->write(lineBuffer, strlen(lineBuffer));

        lineStart = token.start;
      }
      else if (tokenStartCharacters.c_str()[index] != '\n')
      {
        lexer.Read(1); // Don't re-read the token false start
      }
      else
      {
        // In case search tokens use '\n' at their start
        lineNumber++;
        lineStart = token.end;
        lexer.Read(1); // Don't re-read '\n'
        continue;
      }
    }

  }
}
