#include "Options.h"

#include <lexer/Lexer.h>
#include <lexer/Token.h>

#include <diagnostic/Diagnostic.h>

#include <parser/Parser.h>

#include <type/Type.h>

#include <vipir/Module.h>
#include <vipir/ABI/SysV.h>

#include <fstream>
#include <iostream>
#include <sstream>

using namespace std::literals;

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "viper: no input files\n";
        return 1;
    }

    auto options = Option::ParseOptions(argc, argv);
    auto inputFilePath = Option::GetInputFile(options);

    std::ifstream inputFile(inputFilePath);
    if (!inputFile.is_open())
    {
        std::cerr << "viper: could not find file '" << inputFilePath << "'\n";
        return 1;
    }

    std::stringstream ss;
    ss << inputFile.rdbuf();
    std::string text = ss.str();

    diagnostic::Diagnostics diag;
    diag.setText(text);
    for (const auto& option : options)
    {
        if (option.type == OptionType::WarningSpec)
        {
            if (option.value.starts_with("no-"))
                diag.setWarning(false, option.value.substr(3));
            else
                diag.setWarning(true, option.value);
        }
    }

    Type::Init();

    lexer::Lexer lexer(text, argv[1]);
    auto tokens = lexer.lex();
    lexer.scanInvalidTokens(tokens, diag);

    Scope globalScope(nullptr, "", true);
    parser::Parser parser(tokens, diag, &globalScope);
    auto ast = parser.parse();

    bool hadTypeCheckerErrors = false;
    for (auto& node : ast)
    {
        node->typeCheck(diag, hadTypeCheckerErrors);
    }
    if (hadTypeCheckerErrors)
    {
        return EXIT_FAILURE;
    }

    vipir::Module module(argv[1]);
    module.setABI<vipir::abi::SysV>();
    
    // TODO: Add flags to enable optimizations
    module.addPass(vipir::Pass::DeadCodeElimination);
    module.addPass(vipir::Pass::PeepholeOptimization);
    module.addPass(vipir::Pass::ConstantFolding);
    
    vipir::IRBuilder builder;
    for (auto& node : ast)
    {
        node->codegen(builder, module, diag);
    }

    std::ofstream outputFile(argv[1] + ".o"s);
    module.emit(outputFile, vipir::OutputFormat::ELF);

    return 0;
}