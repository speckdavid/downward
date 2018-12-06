#include "errors.h"

using namespace std;

namespace options {
ArgError::ArgError(const string &msg)
    : msg(msg) {
}

ostream &operator<<(ostream &out, const ArgError &err) {
    return out << "argument error: " << err.msg;
}

OptionParserError::OptionParserError(const string &msg)
    : msg(msg) {
}

ostream &operator<<(ostream &out, const OptionParserError &err) {
    return out << "option parser error: " << err.msg;
}

ParseError::ParseError(const string &msg, ParseTree parse_tree)
    : msg(msg),
      parse_tree(parse_tree) {
}

ParseError::ParseError(
    const string &msg, const ParseTree &parse_tree, const string &substring)
    : msg(msg),
      parse_tree(parse_tree),
      substring(substring) {
}

ostream &operator<<(ostream &out, const ParseError &parse_error) {
    out << "parse error: " << endl
        << parse_error.msg << " at: " << endl;
    kptree::print_tree_bracketed<ParseNode>(parse_error.parse_tree, out);
    if (!parse_error.substring.empty()) {
        out << " (cannot continue parsing after \"" << parse_error.substring
            << "\")" << endl;
    }
    return out;
}


string get_demangling_hint(const string &type_name) {
    return "To retrieve the demangled C++ type for gcc/clang, you can call \n"
           "c++filt -t " + type_name;
}


void print_demangling_hint(const string &type_name) {
    cerr << get_demangling_hint(type_name) << endl;
}
}
