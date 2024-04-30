// Copyright 2024 solar-mist


#include "scope/Scope.h"

#include <iostream>

std::unordered_map<std::string, FunctionSymbol> GlobalFunctions;
std::unordered_map<std::string, GlobalSymbol>   GlobalVariables;

LocalSymbol::LocalSymbol(vipir::AllocaInst* alloca)
    : alloca{alloca}
{
}

FunctionSymbol::FunctionSymbol(vipir::Function* function)
    : function(function)
{
}

GlobalSymbol::GlobalSymbol(vipir::GlobalVar* global)
    : global(global)
{
}

Scope::Scope(Scope* parent)
    : parent(parent)
{
}

LocalSymbol* Scope::findVariable(const std::string& name)
{
    Scope* scope = this;
    while (scope)
    {
        if (scope->locals.find(name) != scope->locals.end())
        {
            return &scope->locals.at(name);
        }

        scope = scope->parent;
    }

    return nullptr;
}