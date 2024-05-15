// Copyright 2024 solar-mist


#include "parser/ast/expression/NullptrLiteral.h"
#include "type/PointerType.h"

#include <vipir/IR/Constant/ConstantNullPtr.h>

namespace parser
{
    NullptrLiteral::NullptrLiteral(Type* type)
    {
        mType = type ? type : PointerType::Create(Type::Get("i8"));
    }

    vipir::Value* NullptrLiteral::emit(vipir::IRBuilder& builder, vipir::Module& module, Scope* scope, diagnostic::Diagnostics& diag)
    {
        return vipir::ConstantNullPtr::Get(module, mType->getVipirType());
    }
}