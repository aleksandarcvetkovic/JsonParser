#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <memory>
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

using namespace::std;

// Enum to represent different types of JSON values
enum class JsonType { OBJECT, ARRAY, STRING, NUMBER, BOOLEAN, NIL };

// Class to represent a JSON value
class JsonValue {
public:
    JsonType type;
    unordered_map<string, JsonValue> objectValue;
    vector<JsonValue> arrayValue;
    string stringValue;
    double numberValue;
    bool boolValue;

    JsonValue() : type(JsonType::NIL) {}
    JsonValue(double num) : type(JsonType::NUMBER), numberValue(num) {}
    JsonValue(bool b) : type(JsonType::BOOLEAN), boolValue(b) {}
    JsonValue(const string& str) : type(JsonType::STRING), stringValue(str) {}
    JsonValue(const unordered_map<string, JsonValue>& obj) : type(JsonType::OBJECT), objectValue(obj) {}
    JsonValue(const vector<JsonValue>& arr) : type(JsonType::ARRAY), arrayValue(arr) {}

    JsonValue& operator[](const string& key) {
        if (type != JsonType::OBJECT) throw runtime_error("Not an object");
        return objectValue[key];
    }

    JsonValue& operator[](size_t index) {
        if (type != JsonType::ARRAY) throw runtime_error("Not an array");
        if (index >= arrayValue.size()) throw runtime_error("Index out of bounds");
        return arrayValue[index];
    }

    size_t size() const {
        if (type == JsonType::OBJECT) return objectValue.size();
        if (type == JsonType::ARRAY) return arrayValue.size();
        if (type == JsonType::STRING) return stringValue.size();
        throw runtime_error("Invalid type for size");
    }

    double toNumber() const {
        if (type == JsonType::NUMBER) return numberValue;
        throw runtime_error("Value is not a number");
    }
};

// Function to skip whitespace characters
void skipWhitespace(const string& json, size_t& pos) {
    while (pos < json.size() && isspace(json[pos])) {
        pos++;
    }
}

// Forward declaration
JsonValue parseJsonValue(const string& json, size_t& pos);

// Function to parse a JSON string
string parseJsonString(const string& json, size_t& pos) {
    if (json[pos] != '"') throw runtime_error("Expected '\"'");
    pos++;
    string result;
    while (pos < json.size() && json[pos] != '"') {
            result += json[pos];
        pos++;
    }
    if (json[pos] != '"') throw runtime_error("Expected '\"'");
    pos++;
    return result;
}

// Function to parse a JSON number
double parseJsonNumber(const string& json, size_t& pos) {
    size_t start = pos;
    while (pos < json.size() && (isdigit(json[pos]) || json[pos] == '.' || json[pos] == '-' || json[pos] == '+')) {
        pos++;
    }
    return stod(json.substr(start, pos - start));
}

// Function to parse a JSON array
JsonValue parseJsonArray(const string& json, size_t& pos) {
    if (json[pos] != '[') throw runtime_error("Expected '['");
    pos++;
    skipWhitespace(json, pos);
    vector<JsonValue> values;
    if (json[pos] != ']') {
        while (true) {
            values.push_back(parseJsonValue(json, pos));
            skipWhitespace(json, pos);
            if (json[pos] == ']') break;
            if (json[pos] != ',') throw runtime_error("Expected ','");
            pos++;
            skipWhitespace(json, pos);
        }
    }
    pos++;
    return JsonValue(values);
}

// Function to parse a JSON object
JsonValue parseJsonObject(const string& json, size_t& pos) {
    if (json[pos] != '{') throw runtime_error("Expected '{'");
    pos++;
    skipWhitespace(json, pos);
    unordered_map<string, JsonValue> object;
    if (json[pos] != '}') {
        while (true) {
            string key = parseJsonString(json, pos);
            skipWhitespace(json, pos);
            if (json[pos] != ':') throw runtime_error("Expected ':'");
            pos++;
            skipWhitespace(json, pos);
            object[key] = parseJsonValue(json, pos);
            skipWhitespace(json, pos);
            if (json[pos] == '}') break;
            if (json[pos] != ',') throw runtime_error("Expected ','");
            pos++;
            skipWhitespace(json, pos);
        }
    }
    pos++;
    return JsonValue(object);
}

// Function to parse a JSON value
JsonValue parseJsonValue(const string& json, size_t& pos) {
    skipWhitespace(json, pos);
    if (json[pos] == '{') return parseJsonObject(json, pos);
    if (json[pos] == '[') return parseJsonArray(json, pos);
    if (json[pos] == '"') return JsonValue(parseJsonString(json, pos));
    if (isdigit(json[pos]) || json[pos] == '-') return JsonValue(parseJsonNumber(json, pos));
    if (json.substr(pos, 4) == "true") {
        pos += 4;
        return JsonValue(true);
    }
    if (json.substr(pos, 5) == "false") {
        pos += 5;
        return JsonValue(false);
    }
    if (json.substr(pos, 4) == "null") {
        pos += 4;
        return JsonValue();
    }
    throw runtime_error("Invalid JSON value");
}

// Function to read and parse the JSON file
JsonValue readJson(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Could not open file: " + filename);
    }
    stringstream buffer;
    buffer << file.rdbuf();
    size_t pos = 0;
    return parseJsonValue(buffer.str(), pos);
}

// Token types for expression parsing
enum class TokenType { IDENTIFIER, NUMBER, COMMA, LEFT_PAREN, RIGHT_PAREN, LEFT_BRACKET, RIGHT_BRACKET, DOT, END };

// Token structure
struct Token {
    enum TokenType type;
    string value;
};

// Tokenizer class for breaking down expressions
class Tokenizer {
    string expression;
    size_t pos;

public:
    Tokenizer(const string& expr) : expression(expr), pos(0) {}

    Token nextToken() {
        while (pos < expression.size() && isspace(expression[pos])) pos++;
        if (pos >= expression.size()) return { TokenType::END, "" };

        char current = expression[pos];
        if (isalpha(current)) {
            size_t start = pos;
            while (pos < expression.size() && (isalnum(expression[pos]) || expression[pos] == '_')) pos++;
            return { TokenType::IDENTIFIER, expression.substr(start, pos - start) };
        }
        else if (isdigit(current) || current == '-') {
            size_t start = pos;
            while (pos < expression.size() && (isdigit(expression[pos]) || expression[pos] == '.')) pos++;
            return { TokenType::NUMBER, expression.substr(start, pos - start) };
        }
        else if (current == ',') {
            pos++;
            return { TokenType::COMMA, "," };
        }
        else if (current == '(') {
            pos++;
            return { TokenType::LEFT_PAREN, "(" };
        }
        else if (current == ')') {
            pos++;
            return { TokenType::RIGHT_PAREN, ")" };
        }
        else if (current == '[') {
            pos++;
            return { TokenType::LEFT_BRACKET, "[" };
        }
        else if (current == ']') {
            pos++;
            return { TokenType::RIGHT_BRACKET, "]" };
        }
        else if (current == '.') {
            pos++;
            return { TokenType::DOT, "." };
        }

        throw runtime_error("Unexpected character in expression");
    }
};

// Abstract Syntax Tree (AST) Node for expressions
struct AstNode {
    enum class Type { LITERAL, IDENTIFIER, FUNCTION_CALL, SUBSCRIPT, MEMBER_ACCESS } type;
    string value;
    vector<shared_ptr<AstNode>> children;

    AstNode(Type t, const string& val) : type(t), value(val) {}
};

// Parser class for building the AST
class ExpressionParser {
    Tokenizer tokenizer;
    Token currentToken;

    void advance() {
        currentToken = tokenizer.nextToken();
    }

    shared_ptr<AstNode> parsePrimary() {
        if (currentToken.type == TokenType::NUMBER || currentToken.type == TokenType::IDENTIFIER) {
            auto node = make_shared<AstNode>(AstNode::Type::LITERAL, currentToken.value);
            advance();
            return node;
        }
        else if (currentToken.type == TokenType::LEFT_PAREN) {
            advance();
            auto node = parseExpression();
            if (currentToken.type != TokenType::RIGHT_PAREN) {
                throw runtime_error("Expected ')'");
            }
            advance();
            return node;
        }
        throw runtime_error("Invalid expression");
    }

    // Method to parse a subscript or function call
    shared_ptr<AstNode> parsePostfix(shared_ptr<AstNode> node) {
        while (currentToken.type == TokenType::LEFT_BRACKET || currentToken.type == TokenType::DOT) {
            if (currentToken.type == TokenType::LEFT_BRACKET) {
                advance();
                auto subscriptNode = make_shared<AstNode>(AstNode::Type::SUBSCRIPT, "");
                subscriptNode->children.push_back(node);
                subscriptNode->children.push_back(parseExpression());
                if (currentToken.type != TokenType::RIGHT_BRACKET) {
                    throw runtime_error("Expected ']'");
                }
                advance();
                node = subscriptNode;
            }
            else if (currentToken.type == TokenType::DOT) {
                advance();
                if (currentToken.type != TokenType::IDENTIFIER) {
                    throw runtime_error("Expected an identifier after '.'");
                }
                auto memberNode = make_shared<AstNode>(AstNode::Type::MEMBER_ACCESS, currentToken.value);
                memberNode->children.push_back(node);
                advance();
                node = memberNode;
            }
        }
        return node;
    }

    // Method to parse a function call
    shared_ptr<AstNode> parseFunctionCall() {
        if (currentToken.type == TokenType::IDENTIFIER) {
            string functionName = currentToken.value;
            advance();
            if (currentToken.type == TokenType::LEFT_PAREN) {
                advance();
                auto functionNode = make_shared<AstNode>(AstNode::Type::FUNCTION_CALL, functionName);
                if (currentToken.type != TokenType::RIGHT_PAREN) {
                    while (true) {
                        functionNode->children.push_back(parseExpression());
                        if (currentToken.type == TokenType::RIGHT_PAREN) break;
                        if (currentToken.type != TokenType::COMMA) {
                            throw runtime_error("Expected ',' or ')'");
                        }
                        advance();
                    }
                }
                advance();
                return functionNode;
            }
            // If it's not a function call, it's just an identifier
            return make_shared<AstNode>(AstNode::Type::IDENTIFIER, functionName);
        }
        return parsePrimary();
    }

    // Method to parse an expression
    shared_ptr<AstNode> parseExpression() {
        auto node = parseFunctionCall();
        return parsePostfix(node);
    }

public:
    ExpressionParser(const string& expr) : tokenizer(expr) {
        advance();
    }

    shared_ptr<AstNode> parse() {
        return parseExpression();
    }
};

// Function to evaluate the AST
JsonValue evaluateAst(const shared_ptr<AstNode>& node, const JsonValue& json) {
    if (node->type == AstNode::Type::LITERAL) {
        // Handle number and identifier literals
        if (all_of(node->value.begin(), node->value.end(), ::isdigit)) {
            return JsonValue(stod(node->value));
        }
        else {
            if (json.type == JsonType::OBJECT && json.objectValue.count(node->value)) {
                return json.objectValue.at(node->value);
            }
            throw runtime_error("Unknown identifier: " + node->value);
        }
    }
    else if (node->type == AstNode::Type::IDENTIFIER) {
        if (json.type == JsonType::OBJECT && json.objectValue.count(node->value)) {
            return json.objectValue.at(node->value);
        }
        throw runtime_error("Unknown identifier: " + node->value);
    }
    else if (node->type == AstNode::Type::FUNCTION_CALL) {
        if (node->value == "min") {
            double minVal = numeric_limits<double>::infinity();
            for (const auto& child : node->children) {
                double num = evaluateAst(child, json).toNumber();
                minVal = min(minVal, num);
            }
            return JsonValue(minVal);
        }
        else if (node->value == "max") {
            double maxVal = -numeric_limits<double>::infinity();
            for (const auto& child : node->children) {
                double num = evaluateAst(child, json).toNumber();
                maxVal = max(maxVal, num);
            }
            return JsonValue(maxVal);
        }
        else if (node->value == "size") {
            if (node->children.size() != 1) {
                throw runtime_error("size() expects exactly one argument");
            }
            return JsonValue(static_cast<double>(evaluateAst(node->children[0], json).size()));
        }
        throw runtime_error("Unknown function: " + node->value);
    }
    else if (node->type == AstNode::Type::SUBSCRIPT) {
        JsonValue array = evaluateAst(node->children[0], json);
        JsonValue index = evaluateAst(node->children[1], json);
        if (array.type != JsonType::ARRAY) {
            throw runtime_error("Subscript operator applied to non-array");
        }
        return array.arrayValue.at(static_cast<size_t>(index.toNumber()));
    }
    else if (node->type == AstNode::Type::MEMBER_ACCESS) {
        JsonValue obj = evaluateAst(node->children[0], json);
        if (obj.type != JsonType::OBJECT) {
            throw runtime_error("Member access applied to non-object");
        }
        if (obj.objectValue.count(node->value)) {
            return obj.objectValue.at(node->value);
        }
        throw runtime_error("Member not found: " + node->value);
    }
    throw runtime_error("Unknown AST node type");
}

void printJsonValue(const JsonValue& value) {
    if (value.type == JsonType::OBJECT) {
        cout << "{ ";
        for (auto it = value.objectValue.begin(); it != value.objectValue.end(); ++it) {
            cout << it->first << ": ";
            printJsonValue(it->second);
            if (next(it) != value.objectValue.end()) {
                cout << ", ";
            }
        }
        cout << "}";
    }
    else if (value.type == JsonType::ARRAY) {
        cout << "[";
        for (auto it = value.arrayValue.begin(); it != value.arrayValue.end(); ++it) {
            printJsonValue(*it);
            if (next(it) != value.arrayValue.end()) {
                cout << ", ";
            }
        }
        cout << "]" ;
    }
    else {
        // For simplicity, print other types as their string representation
        if (value.type == JsonType::STRING) {
            cout << "\"" << value.stringValue << "\"";
        }
        else if (value.type == JsonType::NUMBER) {
            cout << value.numberValue;
        }
        else if (value.type == JsonType::BOOLEAN) {
            cout << (value.boolValue ? "true" : "false") ;
        }
        else if (value.type == JsonType::NIL) {
            cout << "null";
        }
    }
}
// Function to get the directory of the executable
filesystem::path getExecutablePath() {
    char buffer[1024];
#ifdef _WIN32
    GetModuleFileNameA(NULL, buffer, sizeof(buffer));
#else
    readlink("/proc/self/exe", buffer, sizeof(buffer));
#endif
    return filesystem::path(buffer).parent_path();
}
void Test(string path, string expression) {
    try {
        // Read and parse the JSON file
        // Set the current working directory to the directory of the executable
        filesystem::path exePath = getExecutablePath();
        filesystem::current_path(exePath);

        // Read and parse the JSON file using a relative path
        string relativePath = path;
        JsonValue json = readJson(relativePath);

        cout << "JSON ";
        printJsonValue(json);
        cout << endl << "Expression: " << expression << endl;

        ExpressionParser parser(expression);
        auto ast = parser.parse();

        // Evaluate the AST
        cout << "Result: ";
        JsonValue result = evaluateAst(ast, json);

        // Output the result
        printJsonValue(result);
        cout << endl << endl;


    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return ;
    }


}
// Main function
int main(int argc, char* argv[]) {
    

    Test("Json.txt", "a.b[1]");
    Test("Json.txt", "a.b[2].c");
    Test("Json.txt", "a.b");
    Test("Json.txt", "a.b[a.b[1]].c");
    Test("Json.txt", "max(a.b[0], a.b[1])");
    Test("Json.txt", "min(a.b[3])");
    Test("Json.txt", "size(a)");
    Test("Json.txt", "size(a.b)");
    Test("Json.txt", "size(a.b[a.b[1]].c)");
    Test("Json.txt", "max(a.b[0], 10, a.b[1], 15)");


    return 0;
}
