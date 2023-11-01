#pragma once

#include "pch.h"

#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <queue>
#include <set>
//#include <variant>
#include <map>
using namespace std;
using namespace System;
using namespace System::IO;
using namespace System::Reflection;
using namespace System::Reflection::Emit;
const int errorMessagesLen = 3;



enum TokenType
{
	OPERATOR_TYPE,
	IDENT_TYPE,
	INTCONST_TYPE,
	FLOATCONST_TYPE,
	BOOLCONST_TYPE,
	CHARCONST_TYPE,
	STRINGCONST_TYPE
};

struct TextPosition
{
	int lineNum;
	int charNum;
};

struct Error
{
	string line;
	TextPosition tp;
	int errCode;
};

class Token
{

public:
	TokenType tokenType;
	TextPosition tp;
	string data;
	//variant<int, float, bool, char, string> val;
	int code;


	virtual string info();

	Token(TokenType _tokenType, TextPosition _tp, string _data);

	TokenType type();

};

class OperToken : public Token
{
public:

	OperToken(TextPosition _tp, string _data, int _code) : Token(OPERATOR_TYPE, _tp, _data) { code = _code; }

	virtual string info();

};

class IdentToken : public Token
{
public:
	IdentToken(TextPosition _tp, string _data) : Token(IDENT_TYPE, _tp, _data) { code = 0; }

	virtual string info();
};

class ConstToken : public Token
{
public:
	ConstToken(TokenType _tokenType, TextPosition _tp, string _data);
	virtual string info();
};



class LexicalAnalyzer
{
private:
	int keywordsCnt = 2;
	char ch;


	string errorMessages[3];

public:
	ifstream fin;
	string line = "";
	TextPosition tp;

	map<string, int> keywords;
	queue<Error> errors;

	LexicalAnalyzer(string file);


	~LexicalAnalyzer()
	{
		fin.close();
	}

	void dropErrors(ofstream& fout);

	char nextch();

	Token* nextsym();

};