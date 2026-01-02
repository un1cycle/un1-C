#include <array>
#include <cstdlib>
#include <expected>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
// #include <unordered_map>

enum class lex_error {};

enum class lex_types { WORD, INT, FLOAT, SYMBOL, MULTI_CHARACTER_SYMBOL };

auto multi_character_symbols =
    std::to_array({"+=", "*="}); // sort by longest -> shortest in length

class token {
public:
  enum lex_types type;
  std::string value;

  token(enum lex_types type, std::string value) : type(type), value(value) {}
};

std::expected<std::vector<token>, lex_error>
lex_code(const std::string &buffer) {
  std::vector<token> result;

  for (unsigned int i = 0; i < buffer.length(); ++i) {
    enum lex_types type;
    std::string value = "";

    if (buffer.substr(i, 2) == "//") {
      while (i != buffer.length() && buffer[i] != '\n')
        ++i;
      if (i == buffer.length())
        break;
      --i;
      continue;
    }

    if (buffer.substr(i, 2) == "/*") {
      while (i != buffer.length() && buffer.substr(i, 2) != "*/")
        ++i;
      ++i; // to move past the rest of the comment symbol
      if (i == buffer.length())
        break;
      continue;
    }

    if ((buffer[i] >= '0' && buffer[i] <= '9') || buffer[i] == '.') {
      while (i != buffer.length() &&
             ((buffer[i] >= '0' && buffer[i] <= '9') || buffer[i] == '.')) {
        value += buffer[i];
        ++i;
      }
      if (value != ".") { // so that "." will go continue to the symbol
                          // definition section
        type = lex_types::INT;
        for (const char c : value)
          if (c == '.')
            type = lex_types::FLOAT;

        result.push_back({type, value});
        --i;
        continue;
      } else {
        --i; // to move back for the dot
      }
    }

    if ((buffer[i] >= '0' && buffer[i] <= '9') ||
        (buffer[i] >= 'A' && buffer[i] <= 'Z') ||
        (buffer[i] >= 'a' && buffer[i] <= 'z') || (buffer[i] == '_')) {
      while (i != buffer.length() &&
             ((buffer[i] >= '0' && buffer[i] <= '9') ||
              (buffer[i] >= 'A' && buffer[i] <= 'Z') ||
              (buffer[i] >= 'a' && buffer[i] <= 'z') || (buffer[i] == '_'))) {
        value += buffer[i];
        ++i;
      }

      type = lex_types::WORD;
      result.push_back({type, value});
      --i;
      continue;
    }

    bool found_matching_multi_character_symbol = false;
    for (std::string n : multi_character_symbols) {
      if (n == buffer.substr(i, n.length())) {
        type = lex_types::MULTI_CHARACTER_SYMBOL;
        value = n;
        result.push_back({type, value});
        found_matching_multi_character_symbol = true;
        i += n.length() - 1;
        break;
      }
    }

    if (found_matching_multi_character_symbol)
      continue;

    if (buffer[i] == ' ') {
      continue;
    }

    type = lex_types::SYMBOL;
    value += buffer[i];
    result.push_back({type, value});
  }

  return result;
}

class Node {
public:
  Node *back;
  std::unique_ptr<Node> left;
  std::unique_ptr<Node> right;

  std::array<std::string, 1>
      attributes; // this should be an array of enums, using a
                  // string for convenience currently

  Node(Node *back, std::unique_ptr<Node> left, std::unique_ptr<Node> right)
      : back(back), left(std::move(left)), right(std::move(right)) {}
};

enum class parse_error {
  NO_MATCH_FOUND = 100,
  UNBALANCED_BRACES,
  OUT_OF_CODE,
  // pratt errors = 200 in this same enum
};

// enum class types {
//   I32,
//   I64,
//   F32,
//   F64,
//   CHAR,
//   STRING,
//   BOOL,
//   I32_p,
//   I64_p,
//   F32_p,
//   F64_p,
//   CHAR_p,
//   STRING_p,
//   BOOL_p
// }; // _p means pointer
//
// const std::unordered_map<std::string, bool> is_allowed_in_pratt_parse_map{
//     {"+", true}, {"-", true}};
//
// bool is_allowed_in_pratt_parse(const std::string &symbol) {
//   if (is_allowed_in_pratt_parse_map.count(symbol) == 0)
//     return false;
//   return true;
// }

std::expected<std::unique_ptr<Node>, parse_error>
pratt_parse(std::unique_ptr<Node> root, const std::vector<token> &buffer_lex) {
  for (int i = 0; i < buffer_lex.size(); ++i) {
    // incomplete!
  }

  return std::move(root);
}

std::expected<std::unique_ptr<Node>, parse_error>
parse(std::unique_ptr<Node> root, const std::vector<token> &buffer_lex,
      int index_begin);

std::expected<std::unique_ptr<Node>, parse_error>
handle_control_flow_else(std::unique_ptr<Node> root,
                         const std::vector<token> &buffer_lex, int index_begin,
                         int i,
                         const std::string &keyword) { // i means current index
  std::vector<token> temp_buffer = {};
  i++; // now buffer_lex[i].value == "{"

  root->attributes = {
      keyword}; // root->left should be code, root->right should be later code
  temp_buffer = {};
  i++;
  int bracket_count = 1;
  while (true) {
    if (i >= buffer_lex.size())
      return std::unexpected(parse_error::UNBALANCED_BRACES);
    if (bracket_count == 0)
      break;
    if (buffer_lex[i].value == "{") // means overstepped buffer length
      bracket_count++;
    else if (buffer_lex[i].value == "}")
      bracket_count--;

    temp_buffer.push_back(buffer_lex[i]);
    i++;
  } // now buffer_lex[    i - 1   ].value == "}"

  temp_buffer.pop_back();

  auto code_result = parse(std::make_unique<Node>(root.get(), nullptr, nullptr),
                           temp_buffer, 0);
  if (!code_result.has_value()) {
    return std::unexpected(code_result.error());
  }
  root->left = std::move(*code_result);

  root->right = std::make_unique<Node>(nullptr, nullptr, nullptr);
  auto later_code_result = parse(std::move(root->right), buffer_lex, i);
  if (!later_code_result.has_value()) {
    return std::unexpected(later_code_result.error());
  }
  root->right = std::move(*later_code_result);

  return std::move(root);
}
std::expected<std::unique_ptr<Node>, parse_error> handle_control_flow_keyword(
    std::unique_ptr<Node> root, const std::vector<token> &buffer_lex,
    int index_begin, int i,
    const std::string &keyword) { // i means current index
  std::vector<token> temp_buffer = {};
  i++;
  while (true) {
    if (i >= buffer_lex.size())
      return std::unexpected(parse_error::UNBALANCED_BRACES);
    if (buffer_lex[i].value == "{")
      break;
    temp_buffer.push_back(
        buffer_lex[i]); // means you didn't open if statement code
    i++;
  } // now buffer_lex[i].value == "{"

  root->attributes = {
      keyword}; // root->left->left should be conditional, root->left->right
                // should be code, root->right should be rest of code
  root->left = std::make_unique<Node>(
      root.get(), std::make_unique<Node>(nullptr, nullptr, nullptr),
      std::make_unique<Node>(nullptr, nullptr, nullptr));
  auto conditional_result =
      pratt_parse(std::move(root->left->left), temp_buffer);
  if (!conditional_result.has_value()) {
    return std::unexpected(conditional_result.error());
  }

  root->left->left = std::move(*conditional_result);

  temp_buffer = {};
  i++;
  int bracket_count = 1;
  while (true) {
    if (i >= buffer_lex.size())
      return std::unexpected(parse_error::UNBALANCED_BRACES);
    if (bracket_count == 0)
      break;
    if (buffer_lex[i].value == "{") // means overstepped buffer length
      bracket_count++;
    else if (buffer_lex[i].value == "}")
      bracket_count--;

    temp_buffer.push_back(buffer_lex[i]);
    i++;
  } // now buffer_lex[    i - 1   ].value == "}"

  temp_buffer.pop_back();

  auto code_result = parse(std::move(root->left->right), temp_buffer, 0);
  if (!code_result.has_value()) {
    return std::unexpected(code_result.error());
  }
  root->left->right = std::move(*code_result);

  root->right = std::make_unique<Node>(nullptr, nullptr, nullptr);
  auto later_code_result = parse(std::move(root->right), buffer_lex, i);
  if (!later_code_result.has_value()) {
    return std::unexpected(later_code_result.error());
  }
  root->right = std::move(*later_code_result);

  return std::move(root);
}

std::expected<std::unique_ptr<Node>, parse_error>
handle_all_control_flow(std::unique_ptr<Node> root,
                        const std::vector<token> &buffer_lex, int index_begin,
                        int i) { // i means current index
  if (buffer_lex[i].type == lex_types::WORD && buffer_lex[i].value == "if") {
    auto temp_root = handle_control_flow_keyword(std::move(root), buffer_lex,
                                                 index_begin, i, "if");
    if (!temp_root.has_value())
      return std::unexpected(temp_root.error());
    root = std::move(*temp_root);
  }

  else if (buffer_lex[i].type == lex_types::WORD &&
           buffer_lex[i].value == "while") {
    auto temp_root = handle_control_flow_keyword(std::move(root), buffer_lex,
                                                 index_begin, i, "while");
    if (!temp_root.has_value())
      return std::unexpected(temp_root.error());
    root = std::move(*temp_root);
  } else if (buffer_lex[i].type == lex_types::WORD &&
             buffer_lex[i].value == "for") {
    auto temp_root = handle_control_flow_keyword(std::move(root), buffer_lex,
                                                 index_begin, i, "for");
    if (!temp_root.has_value())
      return std::unexpected(temp_root.error());
    root = std::move(*temp_root);
  } else if (buffer_lex[i].type == lex_types::WORD &&
             buffer_lex[i].value == "else") {
    auto temp_root = handle_control_flow_else(std::move(root), buffer_lex,
                                              index_begin, i, "else");
    if (!temp_root.has_value())
      return std::unexpected(temp_root.error());
    root = std::move(*temp_root);
  } else if (buffer_lex[i].type == lex_types::WORD &&
             buffer_lex[i].value == "elif") {
    auto temp_root = handle_control_flow_keyword(std::move(root), buffer_lex,
                                                 index_begin, i, "elif");
    if (!temp_root.has_value())
      return std::unexpected(temp_root.error());
    root = std::move(*temp_root);
  } else { // time to handle actual statements
    std::vector<token> statement = {};
    while (i < buffer_lex.size() && (buffer_lex[i].type != lex_types::SYMBOL ||
                                     buffer_lex[i].value != ";")) {
      statement.push_back(buffer_lex[i]);
      ++i;
    } // this ends on the ';'

    auto temp_root = pratt_parse(
        std::make_unique<Node>(root.get(), nullptr, nullptr), statement);
    if (!temp_root.has_value())
      return std::unexpected(temp_root.error());
    root->left = std::move(*temp_root);

    root->right = std::make_unique<Node>(nullptr, nullptr, nullptr);
    auto later_code_result = parse(std::move(root->right), buffer_lex, i + 1);
    if (!later_code_result.has_value()) {
      return std::unexpected(later_code_result.error());
    }
    root->right = std::move(*later_code_result);
    // what do now is throw the entire statement in the pratt parser,
    // hook that parse up to the tree, and then start the continuing code on the
    // right branch

    // bugcheck in the morning
  }
  return std::move(root);
}

std::expected<std::unique_ptr<Node>, parse_error>
parse(std::unique_ptr<Node> root, const std::vector<token> &buffer_lex,
      int index_begin) {
  for (int i = index_begin; i < buffer_lex.size(); ++i) {
    auto temp_root =
        handle_all_control_flow(std::move(root), buffer_lex, index_begin, i);

    if (!temp_root.has_value())
      return std::unexpected(temp_root.error());
    root = std::move(*temp_root);

    return std::move(root);
  }
  return std::move(root);
}

int main(int argc, char **argv) {
  if (argc <= 1) {
    std::cout << "ERR: You must provide a file as your first CLI argument!\n";
    return EXIT_FAILURE;
  }

  std::ifstream file(argv[1]);
  std::string buffer{std::istreambuf_iterator<char>(file),
                     std::istreambuf_iterator<char>()};
  std::vector<token> lexed_code;
  auto buffer_lex = lex_code(buffer);
  if (!buffer_lex.has_value()) {
    std::cout << "ERR: lexing interrupted by error: " << (int)buffer_lex.error()
              << '\n';
    return EXIT_FAILURE;
  }

  std::unique_ptr<Node> initial_root =
      std::make_unique<Node>(nullptr, nullptr, nullptr);
  auto evaluated_root = parse(std::move(initial_root), *buffer_lex, 0);
  if (!evaluated_root.has_value()) {
    std::cout << "ERR: parsing interrupted by error: "
              << (int)evaluated_root.error() << '\n';
    return EXIT_FAILURE;
  }

  // evaluate starting from *evaluated_root node

  return EXIT_SUCCESS;
}
