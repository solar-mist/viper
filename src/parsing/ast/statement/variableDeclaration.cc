#include <parsing/ast/statement/variableDeclaration.hh>
#include <environment.hh>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>

namespace Parsing
{
    VariableDeclaration::VariableDeclaration(const std::string& name, std::unique_ptr<ASTNode> initVal, bool isFunction, std::shared_ptr<Environment> scope, std::shared_ptr<Type> type)
        :ASTNode(ASTNodeType::VariableDeclaration), _name(name), _initVal(std::move(initVal)), _isFunction(isFunction), _scope(scope)
    {
        _nodeType = (isFunction ? ASTNodeType::Function : ASTNodeType::VariableDeclaration);
        _type = type;
    }

    void VariableDeclaration::Print(std::ostream& stream, int indent) const
    {
        stream << std::string(indent, ' ') << (_isFunction ? "<Function>:\n" : "<Variable-Declaration>:\n");
        stream << std::string(indent, ' ') << "Name: " << _name;
        if(_initVal)
        {
            stream << "\n" << std::string(indent, ' ') << "Value: \n";
            _initVal->Print(stream, indent + 2);
        }
    }

    llvm::Value* VariableDeclaration::Emit(llvm::LLVMContext& ctx, llvm::Module& mod, llvm::IRBuilder<>& builder, std::shared_ptr<Environment> scope)
    {
        if(_isFunction)
        {
            llvm::FunctionType* funcTy = llvm::FunctionType::get(_type->GetLLVMType(), {}, false);
            llvm::Function* func = llvm::Function::Create(funcTy, llvm::GlobalValue::ExternalLinkage, _name, mod);

            llvm::BasicBlock* bb = llvm::BasicBlock::Create(ctx, _name, func);
            builder.SetInsertPoint(bb);

            _initVal->Emit(ctx, mod, builder, _scope);

            return func;
        }
        llvm::AllocaInst* alloca = builder.CreateAlloca(_type->GetLLVMType(), nullptr, _name);
        if(_initVal)
        {
            llvm::Value* initVal = _initVal->Emit(ctx, mod, builder, scope);
            builder.CreateStore(initVal, alloca);
        }
        scope->GetNamedValues()[_name] = alloca;
        return nullptr;
    }
}