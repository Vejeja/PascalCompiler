#pragma once
#include "pch.h"

#include <iostream>
#include <map>
#include <set>
using namespace std;
using namespace System;
using namespace System::IO;
using namespace System::Reflection;
using namespace System::Reflection::Emit;




class Ident;
class TypeS;
class Name;

enum UseCase
{
	TYPE,
	VAR,
	CONST,
};

enum TypeClass
{
	SCALAR,
	ENUM,
};

class TypeS
{
public:
	TypeClass typeClass;
	set<Ident*> consts;

	void AddConst(Ident* _c)
	{
		consts.insert(_c);
	}
	

	TypeS(TypeClass _typeClass)
	{
		typeClass = _typeClass;
	}
};

class Name
{
public:
	string name;

	Name(string _name)
	{
		name = _name;
	}
};


class Ident
{
public:
	Name* name;
	TypeS* type;
	UseCase useCase;
	int index;

	Ident(Name* _name, UseCase _useCase, TypeS* _type)
	{
		name = _name;
		useCase = _useCase;
		type = _type;
	}
};


class Space
{
public:
	Space* parent;
	map<string, Name*> nametable;
	map<string, Ident*> identtable;
	set<TypeS*> typetable;

	TypeS* intType;
	TypeS* realType;
	TypeS* charType;
	TypeS* boolType;
	TypeS* noneType;

	Name* intName;
	Name* realName;
	Name* charName;
	Name* boolName;
	Name* trueName;
	Name* falseName;

	Ident* tIdent;
	Ident* fIdent;

	~Space()
	{
		map<string, Ident*>::iterator idents;
		map<string, Name*>::iterator names;
		set<TypeS*>::iterator types;
		for (idents = identtable.begin(); idents != identtable.end(); idents++)
			delete idents->second;
		for (names = nametable.begin(); names != nametable.end(); names++)
			delete names->second;
		for (types = typetable.begin(); types != typetable.end(); types++)
			delete (*types);

	}




	TypeS* AddType(TypeClass typeClass)
	{
		TypeS* ret = new TypeS(typeClass);
		typetable.insert(ret);
		return ret;
	}

	Name* AddName(string _name)
	{
		Name* ret = new Name(_name);
		nametable.insert({ _name,ret });
		return ret;
	}

	Ident* AddIdent(Name* _name, UseCase _useCase, TypeS* _type)
	{
		Ident* ret = new Ident(_name, _useCase, _type);
		identtable.insert({ _name->name,ret });
		return ret;
	}

	Ident* FindIdent(string _name)
	{
		map<string, Ident*>::iterator it;
		it = identtable.find(_name);
		if (it == identtable.end())
			return nullptr;
		return it->second;
	}

	bool HasAName(string _name)
	{
		return nametable.find(_name) != nametable.end();
	}


	Space(Space* _parent)
	{
		parent = _parent;


		if (_parent == nullptr)
		{
			intType = AddType(SCALAR);
			realType = AddType(SCALAR);
			charType = AddType(SCALAR);
			boolType = AddType(ENUM);
			noneType = AddType(SCALAR);

			intName = AddName("integer");
			realName = AddName("real");
			charName = AddName("char");
			boolName = AddName("boolean");

			trueName = AddName("true");
			falseName = AddName("false");

			tIdent = AddIdent(trueName, CONST, boolType);
			fIdent = AddIdent(falseName, CONST, boolType);

			boolType->AddConst(tIdent);
			boolType->AddConst(fIdent);



			AddIdent(intName, TYPE, intType);
			AddIdent(realName, TYPE, realType);
			AddIdent(charName, TYPE, charType);
			AddIdent(boolName, TYPE, boolType);

		}
	}
};
