#include "pch.h"

#include "Analysis.h"
#include "Semantic.h"
#include "Syntax.h"

using namespace System;
using namespace System::IO;
using namespace System::Reflection;
using namespace System::Reflection::Emit;

Token::Token(TokenType _tokenType, TextPosition _tp, string _data)
{
	data = _data;
	tp = _tp;
	tokenType = _tokenType;
}

TokenType Token::type()
{
	return tokenType;
}

string Token::info() { return ""; }

string OperToken::info()
{
	return "Operator/keyword";
}

string IdentToken::info()
{
	return "Identifier";
}

ConstToken::ConstToken(TokenType _tokenType, TextPosition _tp, string _data) : Token(_tokenType, _tp, _data)
{
	code = 1;
}

string ConstToken::info()
{
	switch (tokenType)
	{
	case INTCONST_TYPE:
		return "int const";
	case FLOATCONST_TYPE:
		return "float const";
	case CHARCONST_TYPE:
		return "char const";
	case STRINGCONST_TYPE:
		return "string const";
	default:
		return "const";
	}
}



LexicalAnalyzer::LexicalAnalyzer(string file)
{
	fin.open(file);
	tp.charNum = 0;
	tp.lineNum = 0;
	while (line == "")
	{
		getline(fin, line);
		tp.lineNum++;
	}
	ch = line[0];

	errorMessages[0] = "incorrect number value";
	errorMessages[1] = "incorrect char value";
	errorMessages[2] = "invalid character";

	keywords.insert({ "begin" , keywordsCnt++ });
	keywords.insert({ "case", keywordsCnt++ });
	keywords.insert({ "do", keywordsCnt++ });
	keywords.insert({ "else", keywordsCnt++ });
	keywords.insert({ "end", keywordsCnt++ });
	keywords.insert({ "for",keywordsCnt++ });
	keywords.insert({ "goto",keywordsCnt++ });
	keywords.insert({ "and",keywordsCnt++ });
	keywords.insert({ "or",keywordsCnt++ });
	keywords.insert({ "not",keywordsCnt++ });
	keywords.insert({ "if", keywordsCnt++ });
	keywords.insert({ "then", keywordsCnt++ });
	keywords.insert({ "program",keywordsCnt++ });
	keywords.insert({ "until", keywordsCnt++ });
	keywords.insert({ "var", keywordsCnt++ });
	keywords.insert({ "while", keywordsCnt++ });
	keywords.insert({ "with", keywordsCnt++ });
	keywords.insert({ "function", keywordsCnt++ });
	keywords.insert({ "writeln", keywordsCnt++ });
	keywords.insert({ "+", keywordsCnt++ });
	keywords.insert({ "+=", keywordsCnt++ });
	keywords.insert({ "-", keywordsCnt++ });
	keywords.insert({ "-=", keywordsCnt++ });
	keywords.insert({ "*", keywordsCnt++ });
	keywords.insert({ "*=", keywordsCnt++ });
	keywords.insert({ "/", keywordsCnt++ });
	keywords.insert({ "/=", keywordsCnt++ });
	keywords.insert({ "=", keywordsCnt++ });
	keywords.insert({ ":=", keywordsCnt++ });
	keywords.insert({ ".", keywordsCnt++ });
	keywords.insert({ "..", keywordsCnt++ });
	keywords.insert({ ">", keywordsCnt++ });
	keywords.insert({ ">=", keywordsCnt++ });
	keywords.insert({ "<", keywordsCnt++ });
	keywords.insert({ "<=", keywordsCnt++ });
	keywords.insert({ "<>", keywordsCnt++ });
	keywords.insert({ "(", keywordsCnt++ });
	keywords.insert({ ")", keywordsCnt++ });
	keywords.insert({ "[", keywordsCnt++ });
	keywords.insert({ "]", keywordsCnt++ });
	keywords.insert({ "^", keywordsCnt++ });
	keywords.insert({ ",", keywordsCnt++ });
	keywords.insert({ ";", keywordsCnt++ });
	keywords.insert({ ":", keywordsCnt++ });


}

void LexicalAnalyzer::dropErrors(ofstream& fout)
{
	while (!errors.empty())
	{
		Error err = errors.front();
		errors.pop();
		fout << "ERROR:  line:" << err.tp.lineNum << "  position:" << err.tp.charNum + 1 << endl << err.line << endl << errorMessages[err.errCode] << endl;
	}
}

char LexicalAnalyzer::nextch()
{
	if (tp.charNum == line.size())
	{
		getline(fin, line);
		while (line == "")
		{
			getline(fin, line);
			tp.lineNum++;
		}

		tp.charNum = 0;
		tp.lineNum++;
	}
	else
	{
		tp.charNum++;
	}
	ch = line[tp.charNum];
	return ch;
}

Token* LexicalAnalyzer::nextsym()
{
	Token* ret = nullptr;
	string data;
	while (ch == ' ') nextch();
	TextPosition tpTmp = tp;
	if (ch >= 'A' && ch <= 'Z' || ch >= 'a' && ch <= 'z' || ch == '_')
	{
		data = "";
		while ((ch >= 'A' && ch <= 'Z' || ch >= 'a' && ch <= 'z' || ch == '_' || ch >= '0' && ch <= '9') && fin && tp.charNum != line.size())
		{
			data += ch;
			nextch();
		}
		map <string, int> ::iterator it;
		it = keywords.find(data);
		if (it == keywords.end())
			ret = new IdentToken(tpTmp, data);
		else
			ret = new OperToken(tpTmp, data, it->second);


	}
	else if (ch >= '0' && ch <= '9')
	{
		data = "";
		int points = 0;
		while ((ch >= '0' && ch <= '9' || ch == '.') && fin && tp.charNum != line.size())
		{
			data += ch;
			if (ch == '.')
				points++;
			nextch();
		}
		if (points > 1 || points == 0 && data.size() > 10) // точек больше чем одна или integer состоит более чем из 10 цифр
		{
			Error err;
			err.tp = tpTmp;
			err.errCode = 0;
			err.line = line;
			errors.push(err);
			return nullptr;
		}
		if (points)
			ret = new ConstToken(FLOATCONST_TYPE, tpTmp, data);
		else
			ret = new ConstToken(INTCONST_TYPE, tpTmp, data);

	}
	else
	{
		switch (ch)
		{
		case '\"':
			data = "";
			nextch();
			while (ch != '\"' && fin)
			{
				data += ch;
				nextch();
			}

			ret = new ConstToken(STRINGCONST_TYPE, tpTmp, data);
			nextch();
			break;

		case '\'':
			data = "";
			nextch();
			while (ch != '\'' && fin)
			{
				data += ch;
				nextch();
			}

			nextch();
			if (data.size() != 1)
			{
				Error err;
				err.tp = tpTmp;
				err.errCode = 1;
				err.line = line;
				errors.push(err);
				return nullptr;
			}
			ret = new ConstToken(CHARCONST_TYPE, tpTmp, data);
			break;

		case '{':
			nextch();
			while (ch != '}' && fin)
				nextch();
			nextch();
			break;

		case ':':
			nextch();
			if (ch == '=')
			{
				ret = new OperToken(tpTmp, ":=", keywords.find(":=")->second);
				nextch();
			}
			else
			{
				ret = new OperToken(tpTmp, ":", keywords.find(":")->second);
			}

			break;

		case '<':
			nextch();
			if (ch == '=')
			{
				ret = new OperToken(tpTmp, "<=", keywords.find("<=")->second);
				nextch();
			}
			else if (ch == '>')
			{
				ret = new OperToken(tpTmp, "<>", keywords.find("<>")->second);
				nextch();
			}
			else
			{
				ret = new OperToken(tpTmp, "<", keywords.find("<")->second);
			}

			break;

		case '>':
			nextch();
			if (ch == '=')
			{
				ret = new OperToken(tpTmp, ">=", keywords.find(">=")->second);
				nextch();
			}
			else
			{
				ret = new OperToken(tpTmp, ">", keywords.find(">")->second);
			}

			break;

		case '+':
			nextch();
			if (ch == '=')
			{
				ret = new OperToken(tpTmp, "+=", keywords.find("+=")->second);
				nextch();
			}
			else
			{
				ret = new OperToken(tpTmp, "+", keywords.find("+")->second);
			}
			break;

		case '-':
			nextch();
			if (ch == '=')
			{
				ret = new OperToken(tpTmp, "-=", keywords.find("-=")->second);
				nextch();
			}
			else
			{
				ret = new OperToken(tpTmp, "-", keywords.find("-")->second);
			}
			break;

		case '*':
			nextch();
			if (ch == '=')
			{
				ret = new OperToken(tpTmp, "*=", keywords.find("*=")->second);
				nextch();
			}
			else
			{
				ret = new OperToken(tpTmp, "*", keywords.find("*")->second);
			}
			break;

		case '/':
			nextch();
			if (ch == '=')
			{
				ret = new OperToken(tpTmp, "/=", keywords.find("/=")->second);
				nextch();
			}
			else
			{
				ret = new OperToken(tpTmp, "/", keywords.find("/")->second);
			}
			break;

		case '.':
			nextch();
			if (ch == '.')
			{
				ret = new OperToken(tpTmp, "..", keywords.find("..")->second);
				nextch();
			}
			else
			{
				ret = new OperToken(tpTmp, ".", keywords.find(".")->second);
			}
			break;

		case '=':
			ret = new OperToken(tpTmp, "=", keywords.find("=")->second);
			nextch();
			break;

		case ';':
			ret = new OperToken(tpTmp, ";", keywords.find(";")->second);
			nextch();
			break;

		case '(':
			ret = new OperToken(tpTmp, "(", keywords.find("(")->second);
			nextch();
			break;

		case ')':
			ret = new OperToken(tpTmp, ")", keywords.find(")")->second);
			nextch();
			break;

		case '[':
			ret = new OperToken(tpTmp, "[", keywords.find("[")->second);
			nextch();
			break;

		case ']':
			ret = new OperToken(tpTmp, "]", keywords.find("]")->second);
			nextch();
			break;

		case '^':
			ret = new OperToken(tpTmp, "^", keywords.find("^")->second);
			nextch();
			break;

		case ',':
			ret = new OperToken(tpTmp, ",", keywords.find(",")->second);
			nextch();
			break;

		default:
			if ((int)ch != 0 && (int)ch != 9)
			{
				Error err;
				err.tp = tp;
				err.errCode = 2;
				err.line = line;
				errors.push(err);
			}
			nextch();
			return nullptr;
			break;
		}
	}
	return ret;
}