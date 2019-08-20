#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <unordered_map>


using namespace std;

namespace Parse {

bool operator==(const Token &lhs, const Token &rhs) {
    using namespace TokenType;

    if (lhs.index() != rhs.index()) {
        return false;
    }
    if (lhs.Is<Char>()) {
        return lhs.As<Char>().value == rhs.As<Char>().value;
    } else if (lhs.Is<Number>()) {
        return lhs.As<Number>().value == rhs.As<Number>().value;
    } else if (lhs.Is<String>()) {
        return lhs.As<String>().value == rhs.As<String>().value;
    } else if (lhs.Is<Id>()) {
        return lhs.As<Id>().value == rhs.As<Id>().value;
    } else {
        return true;
    }
}

std::ostream &operator<<(std::ostream &os, const Token &rhs) {
    using namespace TokenType;

#define VALUED_OUTPUT(type) \
  if (auto p = rhs.TryAs<type>()) return os << #type << '{' << p->value << '}';

    VALUED_OUTPUT(Number);
    VALUED_OUTPUT(Id);
    VALUED_OUTPUT(String);
    VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type) \
    if (rhs.Is<type>()) return os << #type;

    UNVALUED_OUTPUT(Class);
    UNVALUED_OUTPUT(Return);
    UNVALUED_OUTPUT(If);
    UNVALUED_OUTPUT(Else);
    UNVALUED_OUTPUT(Def);
    UNVALUED_OUTPUT(Newline);
    UNVALUED_OUTPUT(Print);
    UNVALUED_OUTPUT(Indent);
    UNVALUED_OUTPUT(Dedent);
    UNVALUED_OUTPUT(And);
    UNVALUED_OUTPUT(Or);
    UNVALUED_OUTPUT(Not);
    UNVALUED_OUTPUT(Eq);
    UNVALUED_OUTPUT(NotEq);
    UNVALUED_OUTPUT(LessOrEq);
    UNVALUED_OUTPUT(GreaterOrEq);
    UNVALUED_OUTPUT(None);
    UNVALUED_OUTPUT(True);
    UNVALUED_OUTPUT(False);
    UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

    return os << "Unknown token :(";
}


const int IndentedReader::Eof = std::istream::traits_type::eof();

IndentedReader::IndentedReader(istream &is) : input(is), line_number(0) {
    NextLine();
}

int IndentedReader::Next() {
    if (!input) {
        return Eof;
    }
    char c;
    if (current_line_input >> c) {
        return c;
    } else {
        return '\n';
    }
}

int IndentedReader::Get() {
    if (!input) {
        return Eof;
    }
    if (int c = current_line_input.get(); c != Eof) {
        return c;
    } else {
        return '\n';
    }
}

void IndentedReader::NextLine() {
    auto is_space = [](char c) { return isspace(c); };

    for (string line; getline(input, line);) {
        ++line_number;
        auto it = find_if_not(begin(line), end(line), is_space);
        if (it != end(line)) {
            auto leading_spaces = it - begin(line);
            if (leading_spaces % 2 == 1) {
                throw LexerError("Odd number of spaces at the beginning of line " + line);
            }
            current_indent = leading_spaces / 2;
            current_line_input = istringstream(string(it, end(line)));
            return;
        }
    }
    // When input is exhausted we must set current_indent to zero to produce enough Dedent tokens
    current_indent = 0;
}

Lexer::Lexer(std::istream &input)
    : char_reader(input), cur_char(char_reader.Get()), indent(0), current(NextTokenImpl()) {
}

const Token &Lexer::CurrentToken() const {
    return current;
}

Token Lexer::NextToken() {
    current = NextTokenImpl();
    return current;
}

Token Lexer::NextTokenImpl() {
    using namespace TokenType;

    static const unordered_map<string, Token> keywords = {
        {"class",  Class{}},
        {"return", Return{}},
        {"if",     If{}},
        {"else",   Else{}},
        {"def",    Def{}},
        {"print",  Print{}},
        {"and",    And{}},
        {"or",     Or{}},
        {"not",    Not{}},
        {"None",   None{}},
        {"True",   True{}},
        {"False",  False{}},
    };

    if (indent > char_reader.CurrentIndent()) {
        --indent;
        return Dedent{};
    } else if (indent < char_reader.CurrentIndent()) {
        ++indent;
        return Indent{};
    }

    if (cur_char == '\n') {
        char_reader.NextLine();
        cur_char = char_reader.Get();
        return Newline{};
    }

    if (isspace(cur_char)) {
        cur_char = char_reader.Next();
    }

    if (cur_char == IndentedReader::Eof) {
        return Eof{};
    } else if (isdigit(cur_char)) {
        int value = 0;
        while (isdigit(cur_char)) {
            value = value * 10 + (cur_char - '0');
            cur_char = char_reader.Get();
        }
        return Number{value};
    } else if (cur_char == '"' || cur_char == '\'') {
        auto opener = cur_char;
        bool previous_backslash = false;
        string value;
        while (((cur_char = char_reader.Get()) != opener || previous_backslash) && cur_char != '\n') {
            value += cur_char;
            previous_backslash = (cur_char == '\\');
        }
        if (cur_char != opener) {
            throw LexerError("String " + value + " has unbalanced quotes");
        }
        cur_char = char_reader.Next();
        return String{std::move(value)};
    } else if (isalpha(cur_char) || cur_char == '_') {
        string value;
        do {
            value += cur_char;
            cur_char = char_reader.Get();
        } while (isalnum(cur_char) || cur_char == '_');

        if (auto it = keywords.find(value); it != keywords.end()) {
            return it->second;
        } else {
            return Id{std::move(value)};
        }
    } else if (cur_char == '=') {
        cur_char = char_reader.Get();
        if (cur_char == '=') {
            cur_char = char_reader.Next();
            return Eq{};
        } else {
            return Char{'='};
        }
    } else if (cur_char == '!') {
        cur_char = char_reader.Get();
        if (cur_char == '=') {
            cur_char = char_reader.Next();
            return NotEq{};
        } else {
            return Char{'!'};
        }
    } else if (cur_char == '<') {
        cur_char = char_reader.Get();
        if (cur_char == '=') {
            cur_char = char_reader.Next();
            return LessOrEq{};
        } else {
            return Char{'<'};
        }
    } else if (cur_char == '>') {
        cur_char = char_reader.Get();
        if (cur_char == '=') {
            cur_char = char_reader.Next();
            return GreaterOrEq{};
        } else {
            return Char{'>'};
        }
    } else {
        Char result{static_cast<char>(cur_char)};
        cur_char = char_reader.Next();
        return result;
    }
}

} /* namespace Parse */
