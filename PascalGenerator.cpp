#include "pch.h"
#include "Analysis.h"
#include <string>
#include "Syntax.h"
#include <iostream>
#include <array>
using namespace System;
using namespace System::IO;
using namespace System::Reflection;
using namespace System::Reflection::Emit;


int main(cli::array<System::String ^> ^args)
{
    String^ assemblyName = "PROGRAM";
    String^ modName = "PROGRAM.exe";
    String^ typeName = "PROGRAM";
    String^ methodName = "method";

    AssemblyName^ name = gcnew AssemblyName(assemblyName);
    AppDomain^ domain = System::Threading::Thread::GetDomain();

    AssemblyBuilder^ builder = domain->DefineDynamicAssembly(name, AssemblyBuilderAccess::RunAndSave);
    ModuleBuilder^ module = builder->DefineDynamicModule(modName, true);

    TypeBuilder^ typeBuilder = module->DefineType(typeName, TypeAttributes::Public | TypeAttributes::Class);
    MethodBuilder^ methodBuilder = typeBuilder->DefineMethod(methodName, MethodAttributes::Static | MethodAttributes::Public, void::typeid, gcnew cli::array<Type^> {});

    ILGenerator^ ilGenerator = methodBuilder->GetILGenerator();


    SyntacticalAnalyzer sa("program.txt");
    sa.RunCompilation(ilGenerator);

    if (sa.errors.size() > 0 || sa.la->errors.size() > 0)
    {
        std::ofstream fout("output.txt");
        sa.dropErrors(fout);
        sa.la->dropErrors(fout);
    }
    else
    {
        ilGenerator->Emit(OpCodes::Ret);
        builder->SetEntryPoint(methodBuilder);

        Type^ obj = typeBuilder->CreateType();
        builder->Save(modName);
    }
    
    return 0;
}
