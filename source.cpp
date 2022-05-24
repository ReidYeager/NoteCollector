
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

std::string rootDir;

int main()
{
  std::string outDir;
  std::string todo;
  std::string note;

  GetSettings(&outDir, &rootDir, &todo, &note);

  if (todo.size() > 0)
  {
    tokenStrings.push_back(todo);
    tokenStartCharacters.push_back(todo.c_str()[0]);
  }
  if (note.size() > 0)
  {
    tokenStrings.push_back(note);
    tokenStartCharacters.push_back(note.c_str()[0]);
  }
  tokenStartCharacters.push_back('\n'); // Used to count lines

  std::ofstream outFile;
  outFile.open(outDir, std::ios_base::binary);
  outFile.clear();

  char rootBuffer[1024];
  sprintf(rootBuffer, "Root %s %s\n\n", outStringSeparator.c_str(), rootDir.c_str());

  outFile.write(rootBuffer, strlen(rootBuffer));

  SearchDirectory(&outFile, rootDir);

  outFile.close();

  return 0;
}

void GetSettings(std::string* _output,
                 std::string* _rootDir,
                 std::string* _todo,
                 std::string* _note)
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

  ITools::Lexer lexer(settingsText);
  ITools::LexerToken token;

  while (!lexer.CompletedStream())
  {
    if (lexer.ExpectString("ToDo token ="))
    {
      lexer.ReadThrough('"');
      token = lexer.ReadTo('"');
      *_todo = token.string;
      lexer.ReadThrough('\n');
    }
    else if (lexer.ExpectString("Note token ="))
    {
      lexer.ReadThrough('"');
      token = lexer.ReadTo('"');
      *_note = token.string;
      lexer.ReadThrough('\n');
    }
    else if (lexer.ExpectString("Scan Root ="))
    {
      lexer.ReadThrough('"');
      token = lexer.ReadTo('"');
      *_rootDir = token.string;
      lexer.ReadThrough('\n');
    }
    else if (lexer.ExpectString("Output File ="))
    {
      lexer.ReadThrough('"');
      token = lexer.ReadTo('"');
      *_output = token.string;
      lexer.ReadThrough('\n');
    }
    else if (lexer.ExpectString("Output string endcaps ="))
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
    else if (lexer.ExpectString("Output string separator = "))
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

  const char* defaultSettings = "ToDo token = \"TODO : \"\n"
                                "Note token = \"NOTE : \"\n"
                                "Scan Root = \".\"\n"
                                "Output File = \"ToDo.txt\"\n"
                                "Output string endcaps = \"'\" \"'\"\n"
                                "Output string separator = \":\"\0";


  outFile.write(defaultSettings, strlen(defaultSettings));
  outFile.close();

  _file->open(SETTINGS_FILE_DIRECTORY, std::ios_base::ate | std::ios_base::binary);
}

void SearchDirectory(std::ofstream* _output, std::string _searchDir)
{
  char lineBuffer[1024];

  for (const auto& entry : std::filesystem::directory_iterator(_searchDir))
  {
    if (entry.is_directory())
    {
      SearchDirectory(_output, entry.path().u8string());
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
        lineStart = token.start + token.string.size() + 1;
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
        token = lexer.ReadTo('\n');
        std::string line(lineStart, (token.start + token.string.size() - 1) - lineStart);

        if (shoudPrintFileName)
        {
          shoudPrintFileName = false;

          std::string localPath = entry.path().u8string();
          localPath.erase(0, rootDir.size());
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
        lineStart = token.start + token.string.size() + 1;
        lexer.Read(1); // Don't re-read '\n'
        continue;
      }
    }

  }
}
