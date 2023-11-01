#pragma once
#include "pch.h"

#include <iostream>
#include "Analysis.h"
#include "Semantic.h"
#include <stack>
#include <queue>
#define IDENT 0
#define CONST 1
using namespace System;
using namespace System::IO;
using namespace System::Reflection;
using namespace System::Reflection::Emit;


struct SyntSemError
{
	string line;
	TextPosition tp;
	string msg;
};

class SyntacticalAnalyzer
{
private:

	stack<Token*> vars;
	stack<Token*> buf;

	bool IsRelationOperator(int _code)
	{
		return _code == code(">") || _code == code("<") || _code == code(">=") || _code == code("<=") || _code == code("=");
	}

	bool IsAddingOperation(int _code)
	{
		return _code == code("+") || _code == code("-") || _code == code("or");
	}

	bool IsMultiplyingOperation(int _code)
	{
		return _code == code("*") || _code == code("/") || _code == code("and");
	}

	bool IsSign(int _code)
	{
		return _code == code("+") || _code == code("-");
	}

	TypeS* compareTypes(TypeS* t1, TypeS* t2);

	Space* fictSpace;
	Space* progSpace;

public:
	LexicalAnalyzer* la;
	Token* tok;
	queue<SyntSemError> errors;


	void RunCompilation(ILGenerator^ il)
	{
		program(il);
	}

	SyntacticalAnalyzer(string file)
	{
		la = new LexicalAnalyzer(file);
		fictSpace = new Space(nullptr);
		progSpace = new Space(fictSpace);
		nexttoken();
	}

	~SyntacticalAnalyzer()
	{
		delete la;
		delete fictSpace;
		delete progSpace;
		while (!buf.empty())
		{
			Token* t = buf.top();
			buf.pop();
			delete t;
		}
	}


	void dropErrors(ofstream& fout);

	void nexttoken();

	int code(string s)
	{
		return la->keywords.find(s)->second;
	}

	void accept(int _code, string s = "ident or const");

	void program(ILGenerator^ il);

	void block(ILGenerator^ il);

	void varpart(ILGenerator^ il);

	void newvar(ILGenerator^ il);

	void addvars(ILGenerator^ il);

	void vardeclaration(ILGenerator^ il);

	void statement(ILGenerator^ il);

	void assignment(ILGenerator^ il);

	void writeln(ILGenerator^ il);

	void whilestatement(ILGenerator^ il);

	void compoundsstatement(ILGenerator^ il);

	void ifstatement(ILGenerator^ il);



	TypeS* addingoperator(int opCode, TypeS* t1, TypeS* t2, ILGenerator^ il);

	TypeS* multiplyingoperator(int opCode, TypeS* t1, TypeS* t2, ILGenerator^ il);

	TypeS* expression(ILGenerator^ il);

	TypeS* simpleexpression(ILGenerator^ il);

	TypeS* term(ILGenerator^ il);

	TypeS* factor(ILGenerator^ il);

	TypeS* ident(ILGenerator^ il, bool ldlock = true);

	TypeS* unsignedconst(ILGenerator^ il);


};

