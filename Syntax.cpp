#include "pch.h"

#include "Syntax.h"
#include "Analysis.h"
#include "Semantic.h"
using namespace System;
using namespace System::IO;
using namespace System::Reflection;
using namespace System::Reflection::Emit;

TypeS* SyntacticalAnalyzer::compareTypes(TypeS* t1, TypeS* t2)
{
	if (t1 == fictSpace->noneType)
		return t2;
	if (t2 == fictSpace->noneType)
		return t1;
	if (t1 == fictSpace->intType && t2 == fictSpace->realType)
		return t1;
	if (t1 == fictSpace->realType && t2 == fictSpace->intType)
		return t1;
	if (t1 == t2)
		return t1;
	return nullptr;
}

void SyntacticalAnalyzer::nexttoken()
{
	tok = la->nextsym();
	while (tok == nullptr)
		tok = la->nextsym();
	buf.push(tok);
}

void SyntacticalAnalyzer::dropErrors(ofstream& fout)
{
	while (!errors.empty())
	{
		SyntSemError err = errors.front();
		errors.pop();
		fout << "ERROR:  line:" << err.tp.lineNum << endl << err.line << endl << err.msg << endl << endl;
	}
}

void SyntacticalAnalyzer::accept(int _code, string s)
{
	if (tok->code == _code)
		nexttoken();
	else
	{
		SyntSemError newErr;
		newErr.tp = la->tp;
		newErr.line = la->line;
		newErr.msg = "expected: " + s;
		errors.push(newErr);
	}
}

void SyntacticalAnalyzer::program(ILGenerator^ il)
{
	accept(code("program"), "program");
	accept(IDENT);
	accept(code(";"), ";");
	block(il);
	accept(code("."), ".");
}

void SyntacticalAnalyzer::block(ILGenerator^ il)
{
	varpart(il);
	compoundsstatement(il);
}

void SyntacticalAnalyzer::varpart(ILGenerator^ il)
{
	if (tok->code == code("var"))
	{
		nexttoken();
		do
		{
			vardeclaration(il);
			accept(code(";"), ";");
		} while (tok->code == IDENT);
	}
}

void SyntacticalAnalyzer::newvar(ILGenerator^ il)
{
	if (tok->code == IDENT)
	{
		vars.push(tok);
	}
}

void SyntacticalAnalyzer::addvars(ILGenerator^ il)
{
	Space* curSpace = progSpace;
	Ident* typeIdent = nullptr;
	while (curSpace != nullptr)
	{
		typeIdent = curSpace->FindIdent(tok->data);
		if (typeIdent != nullptr)
			break;
		curSpace = curSpace->parent;
	}
	if (typeIdent != nullptr)
	{
		if (typeIdent->useCase != TYPE)
		{
			SyntSemError newErr;
			newErr.tp = la->tp;
			newErr.line = la->line;
			newErr.msg = "wrong use case";
			errors.push(newErr);
		}
		while (!vars.empty())
		{
			Token* curvar = vars.top();
			vars.pop();
			if (progSpace->HasAName(curvar->data))
			{
				SyntSemError newErr;
				newErr.tp = la->tp;
				newErr.line = la->line;
				newErr.msg = "identifier " + curvar->data + " is already declared";
				errors.push(newErr);
			}
			else
			{
				Ident* newIdent = progSpace->AddIdent(progSpace->AddName(curvar->data), VAR, typeIdent->type);
				if (typeIdent->type == fictSpace->intType)
					newIdent->index = il->DeclareLocal(Int32::typeid)->LocalIndex;
				else if (typeIdent->type == fictSpace->realType)
					newIdent->index = il->DeclareLocal(float::typeid)->LocalIndex;
				else if (typeIdent->type == fictSpace->boolType)
					newIdent->index = il->DeclareLocal(Boolean::typeid)->LocalIndex;
				else if (typeIdent->type == fictSpace->charType)
					newIdent->index = il->DeclareLocal(Char::typeid)->LocalIndex;

			}
		}
	}
	else
	{
		SyntSemError newErr;
		newErr.tp = la->tp;
		newErr.line = la->line;
		newErr.msg = "identifier " + tok->data + " is not defined";
		errors.push(newErr);
	}
}

void SyntacticalAnalyzer::vardeclaration(ILGenerator^ il)
{
	newvar(il);
	accept(IDENT);
	while (tok->code == code(","))
	{
		nexttoken();
		newvar(il);
		accept(IDENT);
	}
	accept(code(":"), ":");
	if (tok->code == IDENT)
	{
		addvars(il);
	}
	accept(IDENT);
}

void SyntacticalAnalyzer::statement(ILGenerator^ il)
{
	int tc = tok->code;
	if (tc == IDENT)
		assignment(il);
	else if (tc == code("writeln"))
		writeln(il);
	else if (tc == code("begin"))
		compoundsstatement(il);
	else if (tc == code("if"))
		ifstatement(il);
	else if (tc == code("while"))
		whilestatement(il);
}

void SyntacticalAnalyzer::assignment(ILGenerator^ il)
{
	TypeS* varType = ident(il, false);
	Space* curSpace = progSpace;
	Ident* varIdent = nullptr;
	while (curSpace != nullptr)
	{
		varIdent = curSpace->FindIdent(tok->data);
		if (varIdent != nullptr)
			break;
		curSpace = curSpace->parent;
	}
	accept(IDENT);
	accept(code(":="), ":=");
	TypeS* exprType = expression(il);
	TypeS* resultType = compareTypes(varType, exprType);
	if (resultType == nullptr)
	{
		SyntSemError newErr;
		newErr.line = la->line;
		newErr.tp = la->tp;
		newErr.msg = "type mismatch";
		errors.push(newErr);
	}
	il->Emit(OpCodes::Stloc, varIdent->index);
}

void SyntacticalAnalyzer::writeln(ILGenerator^ il)
{
	accept(code("writeln"), "writeln");
	accept(code("("), "(");
	expression(il);
	accept(code(")"), ")");
	LocalBuilder^ loc = il->DeclareLocal(Int32::typeid);
	il->Emit(OpCodes::Stloc, loc->LocalIndex);
	il->EmitWriteLine(loc);
}

void SyntacticalAnalyzer::whilestatement(ILGenerator^ il)
{
	accept(code("while"), "while");
	Label skipWhile = il->DefineLabel();
	Label startWhile = il->DefineLabel();

	il->MarkLabel(startWhile);
	TypeS* whileType = expression(il);
	il->Emit(OpCodes::Brfalse_S, skipWhile);
	if (!(whileType == fictSpace->boolType || whileType == fictSpace->noneType))
	{
		SyntSemError newErr;
		newErr.line = la->line;
		newErr.tp = la->tp;
		newErr.msg = "expected boolean expression after while-symbol";
		errors.push(newErr);
	}
	accept(code("do"), "do");
	statement(il);
	il->Emit(OpCodes::Br_S, startWhile);
	il->MarkLabel(skipWhile);
}

void SyntacticalAnalyzer::compoundsstatement(ILGenerator^ il)
{
	accept(code("begin"), "begin");
	statement(il);
	while (tok->code == code(";"))
	{
		nexttoken();
		statement(il);
	}
	accept(code("end"), "end");
}

void SyntacticalAnalyzer::ifstatement(ILGenerator^ il)
{
	accept(code("if"), "if");
	Label endLabel = il->DefineLabel();
	Label skipElse = il->DefineLabel();
	TypeS* ifType = expression(il);
	il->Emit(OpCodes::Brfalse_S, endLabel);
	if (!(ifType == fictSpace->boolType || ifType == fictSpace->noneType))
	{
		SyntSemError newErr;
		newErr.line = la->line;
		newErr.tp = la->tp;
		newErr.msg = "expected boolean expression after if-symbol";
		errors.push(newErr);
	}
	accept(code("then"), "then");
	statement(il);
	il->Emit(OpCodes::Br_S, skipElse);
	il->MarkLabel(endLabel);
	if (tok->code == code("else"))
	{
		nexttoken();
		statement(il);
	}
	il->MarkLabel(skipElse);
}

TypeS* SyntacticalAnalyzer::addingoperator(int opCode, TypeS* t1, TypeS* t2, ILGenerator^ il)
{
	TypeS* retType = fictSpace->noneType;
	if (opCode == code("+") || opCode == code("-"))
	{
		retType = compareTypes(t1, t2);
		if (retType == nullptr || !(retType == fictSpace->intType || retType == fictSpace->realType))
		{
			SyntSemError newErr;
			newErr.line = la->line;
			newErr.tp = la->tp;
			newErr.msg = "type mismatch";
			errors.push(newErr);
			retType = fictSpace->noneType;
		}
	}
	else if (opCode == code("or"))
	{
		retType = compareTypes(t1, t2);
		if (retType != fictSpace->boolType)
		{
			SyntSemError newErr;
			newErr.line = la->line;
			newErr.tp = la->tp;
			newErr.msg = "type mismatch";
			errors.push(newErr);
			retType = fictSpace->noneType;
		}
	}
	return retType;
}

TypeS* SyntacticalAnalyzer::multiplyingoperator(int opCode, TypeS* t1, TypeS* t2, ILGenerator^ il)
{
	TypeS* retType = fictSpace->noneType;
	if (opCode == code("*") || opCode == code("/"))
	{
		retType = compareTypes(t1, t2);
		if (retType == nullptr || !(retType == fictSpace->intType || retType == fictSpace->realType))
		{
			SyntSemError newErr;
			newErr.line = la->line;
			newErr.tp = la->tp;
			newErr.msg = "type mismatch";
			errors.push(newErr);
			retType = fictSpace->noneType;
		}
	}
	else if (opCode == code("and"))
	{
		retType = compareTypes(t1, t2);
		if (retType != fictSpace->boolType)
		{
			SyntSemError newErr;
			newErr.line = la->line;
			newErr.tp = la->tp;
			newErr.msg = "type mismatch";
			errors.push(newErr);
			retType = fictSpace->noneType;
		}
	}
	return retType;
}

TypeS* SyntacticalAnalyzer::expression(ILGenerator^ il)
{
	TypeS* t1 = simpleexpression(il);
	TypeS* t2;
	if (IsRelationOperator(tok->code))
	{
		int opCode = tok->code;
		nexttoken();
		t2 = expression(il);
		t1 = compareTypes(t1, t2);
		if (!(t1 == fictSpace->intType || t1 == fictSpace->realType || t1 == fictSpace->noneType))
		{
			SyntSemError newErr;
			newErr.line = la->line;
			newErr.tp = la->tp;
			newErr.msg = "expected boolean";
			errors.push(newErr);
		}
		t1 = fictSpace->boolType;
		if (opCode == code("="))
			il->Emit(OpCodes::Ceq);
		else if (opCode == code(">"))
			il->Emit(OpCodes::Cgt);
		else if (opCode == code("<"))
			il->Emit(OpCodes::Clt);
		else if (opCode == code(">="))
		{
			il->Emit(OpCodes::Clt);
			il->Emit(OpCodes::Not);
		}
		else if (opCode == code("<="))
		{
			il->Emit(OpCodes::Cgt);
			il->Emit(OpCodes::Not);
		}

	}
	return t1;
}

TypeS* SyntacticalAnalyzer::simpleexpression(ILGenerator^ il)
{
	TypeS* t1 = term(il);
	TypeS* t2;

	if (IsAddingOperation(tok->code))
	{
		int opCode = tok->code;
		nexttoken();
		t2 = simpleexpression(il);
		t1 = addingoperator(opCode, t1, t2, il);
		if (opCode == code("+"))
			il->Emit(OpCodes::Add);
		else if (opCode == code("-"))
			il->Emit(OpCodes::Sub);
		else
			il->Emit(OpCodes::Or);
	}
	return t1;

}

TypeS* SyntacticalAnalyzer::term(ILGenerator^ il)
{
	bool hasASign = false;
	int signCode;
	if (IsSign(tok->code))
	{
		signCode = tok->code;
		nexttoken();
		hasASign = true;
		il->Emit(OpCodes::Ldc_I4, (Int32)0);

	}
	TypeS* t1 = factor(il);
	TypeS* t2;
	if (hasASign && !(t1 == fictSpace->intType || t1 == fictSpace->realType || t1 == fictSpace->noneType))
	{
		SyntSemError newErr;
		newErr.line = la->line;
		newErr.tp = la->tp;
		newErr.msg = "type mismatch";
		errors.push(newErr);
		t1 = fictSpace->noneType;
	}

	if (IsMultiplyingOperation(tok->code))
	{
		int opCode = tok->code;
		nexttoken();
		t2 = term(il);
		t1 = multiplyingoperator(opCode, t1, t2, il);
		if (opCode == code("*"))
			il->Emit(OpCodes::Mul);
		else if (opCode == code("/"))
			il->Emit(OpCodes::Div);
		else
			il->Emit(OpCodes::And);
	}
	if (hasASign)
	{

		if (signCode == code("+"))
			il->Emit(OpCodes::Add);
		else
			il->Emit(OpCodes::Sub);
	}
	return t1;
}

TypeS* SyntacticalAnalyzer::factor(ILGenerator^ il)
{
	TypeS* retType;
	if (tok->code == code("("))
	{
		accept(code("("));
		retType = expression(il);
		accept(code(")"));
	}
	else if (tok->code == code("not"))
	{
		nexttoken();
		retType = factor(il);
		il->Emit(OpCodes::Not);
		if (!(retType == fictSpace->boolType || retType == fictSpace->noneType))
		{
			SyntSemError newErr;
			newErr.line = la->line;
			newErr.tp = la->tp;
			newErr.msg = "expected boolean";
			errors.push(newErr);
			retType = fictSpace->noneType;
		}
	}
	else if (tok->code == IDENT)
	{
		retType = ident(il);
		accept(IDENT);
	}
	else
	{
		retType = unsignedconst(il);
		accept(CONST);
	}
	return retType;
}

TypeS* SyntacticalAnalyzer::ident(ILGenerator^ il, bool ldlock)
{
	Space* curSpace = progSpace;
	Ident* varIdent = nullptr;
	while (curSpace != nullptr)
	{
		varIdent = curSpace->FindIdent(tok->data);
		if (varIdent != nullptr)
			break;
		curSpace = curSpace->parent;
	}
	if (varIdent == nullptr)
	{
		SyntSemError newErr;
		newErr.tp = la->tp;
		newErr.line = la->line;
		newErr.msg = "identifier " + tok->data + " is not defined";
		errors.push(newErr);
		varIdent = progSpace->AddIdent(progSpace->AddName(tok->data), VAR, fictSpace->noneType);
	}
	if (ldlock)
		il->Emit(OpCodes::Ldloc, varIdent->index);
	return varIdent->type;
}

TypeS* SyntacticalAnalyzer::unsignedconst(ILGenerator^ il)
{
	if (tok->tokenType == INTCONST_TYPE)
	{
		il->Emit(OpCodes::Ldc_I4, (Int32)stoi(tok->data));
		return fictSpace->intType;
	}
	if (tok->tokenType == FLOATCONST_TYPE)
	{
		il->Emit(OpCodes::Ldc_R4, (float)stof(tok->data));
		return fictSpace->realType;
	}
	if (tok->tokenType == CHARCONST_TYPE)
	{
		il->Emit(OpCodes::Ldc_I4, tok->data[0]);
		return fictSpace->charType;
	}
	return fictSpace->noneType;
}