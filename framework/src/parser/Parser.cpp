// Copyright 2023 solar-mist


#include "parser/Parser.h"

#include "lexer/Token.h"
#include "parser/ast/expression/BinaryExpression.h"

#include <iostream>

namespace parsing
{
    Parser::Parser(std::vector<lexing::Token>& tokens)
        : mTokens(tokens)
        , mPosition(0)
    {
    }

    lexing::Token Parser::current() const
    {
        return mTokens.at(mPosition);
    }

    lexing::Token Parser::consume()
    {
        return mTokens.at(mPosition++);
    }

    lexing::Token Parser::peek(int offset) const
    {
        return mTokens.at(mPosition + offset);
    }

    void Parser::expectToken(lexing::TokenType tokenType)
    {
        if (current().getTokenType() != tokenType)
        {
            lexing::Token temp(tokenType);
            std::cerr << "Expected " << temp.toString() << ". Found " << current().toString() << "\n";
            std::exit(1);
        }
    }

    int Parser::getBinaryOperatorPrecedence(lexing::TokenType tokenType)
    {
        switch (tokenType)
        {
            case lexing::TokenType::LeftParen:
                return 55;

            case lexing::TokenType::Plus:
            case lexing::TokenType::Minus:
                return 35;

            case lexing::TokenType::Equals:
                return 10;
            default:
                return 0;
        }
    }

    std::vector<ASTNodePtr> Parser::parse()
    {
        std::vector<ASTNodePtr> result;

        while (mPosition < mTokens.size())
        {
            result.push_back(parseGlobal());
        }

        return result;
    }

    ASTNodePtr Parser::parseGlobal()
    {
        switch (current().getTokenType())
        {
            case lexing::TokenType::Type:
                return parseFunction();
            case lexing::TokenType::ExternKeyword:
                return parseExternFunction();
            default:
                std::cerr << "Unexpected token: " << current().toString() << ". Expected global statement.\n";
                std::exit(1);
        }
    }

    ASTNodePtr Parser::parseExpression(int precedence)
    {
        ASTNodePtr lhs = parsePrimary();

        while (true)
        {
            int binaryOperatorPrecedence = getBinaryOperatorPrecedence(current().getTokenType());
            if (binaryOperatorPrecedence < precedence)
            {
                break;
            }
            
            lexing::Token operatorToken = consume();
            if (operatorToken == lexing::TokenType::LeftParen)
            {
                lhs = parseCallExpression(std::move(lhs));
            }
            else
            {
                ASTNodePtr rhs = parseExpression(binaryOperatorPrecedence);
                lhs = std::make_unique<BinaryExpression>(std::move(lhs), operatorToken.getTokenType(), std::move(rhs));
            }
        }

        return lhs;
    }

    ASTNodePtr Parser::parsePrimary()
    {
        switch (current().getTokenType())
        {
            case lexing::TokenType::ReturnKeyword:
                return parseReturnStatement();

            case lexing::TokenType::IntegerLiteral:
                return parseIntegerLiteral();

            case lexing::TokenType::Type:
                return parseVariableDeclaration();

            case lexing::TokenType::Identifier:
                return parseVariable();
            default:
                std::cerr << "Unexpected token. Expected primary expression.\n";
                std::exit(1);
        }
    }

    Type* Parser::parseType()
    {
        expectToken(lexing::TokenType::Type);
        return Type::Get(consume().getText());
    }

    FunctionPtr Parser::parseFunction()
    {
        Type* type = parseType();

        expectToken(lexing::TokenType::Identifier);
        std::string name = consume().getText();

        std::vector<FunctionArgument> arguments;
        expectToken(lexing::TokenType::LeftParen);
        consume();
        while (current().getTokenType() != lexing::TokenType::RightParen)
        {
            Type* type = parseType();
            const std::string& name = consume().getText();
            arguments.emplace_back(name, type);
            if (current().getTokenType() != lexing::TokenType::RightParen)
            {
                expectToken(lexing::TokenType::Comma);
                consume();
            }
        }
        consume();

        expectToken(lexing::TokenType::LeftBracket);
        consume();

        std::vector<ASTNodePtr> body;
        while (current().getTokenType() != lexing::TokenType::RightBracket)
        {
            body.push_back(parseExpression());
            expectToken(lexing::TokenType::Semicolon);
            consume();
        }
        consume();

        return std::make_unique<Function>(type, name, std::move(arguments), std::move(body));
    }

    ExternFunctionPtr Parser::parseExternFunction()
    {
        consume(); // ExternKeyword
        
        Type* type = parseType();

        expectToken(lexing::TokenType::Identifier);
        std::string name = consume().getText();

        std::vector<FunctionArgument> arguments;
        expectToken(lexing::TokenType::LeftParen);
        consume();
        while (current().getTokenType() != lexing::TokenType::RightParen)
        {
            Type* type = parseType();
            const std::string& name = consume().getText();
            arguments.emplace_back(name, type);
            if (current().getTokenType() != lexing::TokenType::RightParen)
            {
                expectToken(lexing::TokenType::Comma);
                consume();
            }
        }
        consume();

        expectToken(lexing::TokenType::Semicolon);
        consume();

        return std::make_unique<ExternFunction>(type, name, std::move(arguments));
    }

    ReturnStatementPtr Parser::parseReturnStatement()
    {
        consume();

        if (current().getTokenType() == lexing::TokenType::Semicolon)
        {
            return std::make_unique<ReturnStatement>(nullptr);
        }

        return std::make_unique<ReturnStatement>(parseExpression());
    }

    VariableDeclarationPtr Parser::parseVariableDeclaration()
    {
        Type* type = parseType();

        expectToken(lexing::TokenType::Identifier);
        std::string name = consume().getText();

        if (current().getTokenType() == lexing::TokenType::Semicolon)
        {
            return std::make_unique<VariableDeclaration>(type, std::move(name), nullptr);
        }

        expectToken(lexing::TokenType::Equals);
        consume();

        return std::make_unique<VariableDeclaration>(type, std::move(name), parseExpression());
    }

    IntegerLiteralPtr Parser::parseIntegerLiteral()
    {
        return std::make_unique<IntegerLiteral>(std::stoll(consume().getText()));
    }
    
    VariablePtr Parser::parseVariable()
    {
        return std::make_unique<Variable>(consume().getText());
    }

    CallExpressionPtr Parser::parseCallExpression(ASTNodePtr callee)
    {
        std::vector<ASTNodePtr> parameters;
        while (current().getTokenType() != lexing::TokenType::RightParen)
        {
            parameters.push_back(parseExpression());

            if (current().getTokenType() != lexing::TokenType::RightParen)
            {
                expectToken(lexing::TokenType::Comma);
                consume();
            }
        }
        consume();

        return std::make_unique<CallExpression>(std::move(callee), std::move(parameters));
    }
}