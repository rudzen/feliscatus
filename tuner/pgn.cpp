#include <fcntl.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cctype>
#include <exception>
#include <fmt/format.h>
#include "pgn.h"
#include "../src/types.h"
#include "../src/util.h"

enum Token : uint8_t { Symbol, Integer, String, NAG, Asterisk, Period, LParen, RParen, LBracket, RBracket, LT, GT, Invalid, None };

namespace {

const char token_string[][12] = {"Symbol", "Integer", "String", "NAG", "Asterisk", "Period", "LParen", "RParen", "LBracket", "RBracket", "LT", "GT", "Invalid", "None"};

constexpr bool is_white_space(const char c) {
  return c == ' ' || c == '\t' || c == 0x0a || c == 0x0d;
}

constexpr std::size_t bufsize = 128 * 1024;

bool start_of_castle_move(const char *p) {
  return std::strlen(p) && p[0] == 'O';
}

bool start_of_promoted_to(const char *p) {
  return std::strlen(p) && p[0] == '=';
}

bool is_square(const char *p, Square &square) {
  if (std::strlen(p) > 1 && util::in_between<'a', 'h'>(p[0]) && util::in_between<'0', '9'>(p[1]))
  {
    square = static_cast<Square>(((p[1] - '1') << 3) + p[0] - 'a');
    return true;
  }
  return false;
}

bool is_rank_digit(const char *p, int &rank) {
  if (std::strlen(p) && util::in_between<'1', '8'>(p[0]))
  {
    rank = p[0] - '1';
    return true;
  }
  return false;
}

bool is_file_letter(const char *p, int &file) {
  if (std::strlen(p) && util::in_between<'a', 'h'>(p[0]))
  {
    file = p[0] - 'a';
    return true;
  }
  return false;
}

bool start_of_pawn_quiet_move(const char *p, Square &to_square) {
  return std::strlen(p) > 1 && is_square(p, to_square);
}

constexpr bool start_of_tag_pair(const Token token) { return token == LBracket; }

constexpr bool start_of_tag_section(const Token token) { return start_of_tag_pair(token); }

constexpr bool start_of_recursive_variation(const Token token) { return token == LParen; }

constexpr bool start_of_tag_name(const Token token) { return token == Symbol; }

constexpr bool start_of_tag_value(const Token token) { return token == String; }

}

struct PGNFile {
  PGNFile(const char *path, const int oflag, const int pmode) {
    if ((fd = open(path, oflag | O_BINARY, pmode)) == -1)
    {
      fmt::print(stderr, "File::File: cannot open file");
      exit(EXIT_FAILURE);
    }
  }

  ~PGNFile() { close(fd); }

  size_t read(unsigned char *buf, const size_t count) const {
    int n;

    if ((n = ::read(fd, static_cast<void *>(buf), count)) == -1)
    {
      fmt::print(stderr, "File::File: cannot read file");
      exit(EXIT_FAILURE);
    }
    return n;
  }

private:
  int fd;
};

class UnexpectedToken final : std::exception {
public:
  UnexpectedToken(const Token expected, const char *found, const size_t line) { s = fmt::format("Expected <{}> but found '{}', line={}", token_string[expected], found, line); }

  UnexpectedToken(const char *expected, const char *found, const size_t line) { s = fmt::format("Expected {} but found '{}', line={}", expected, found, line); }

  [[nodiscard]]
  const char *str() const { return s.data(); }

private:
  std::string s;
};

namespace pgn {

PGNFileReader::PGNFileReader()
  : file_(nullptr) {
  if ((buffer_ = new unsigned char[bufsize]) == nullptr)
  {
    fmt::print(stderr, "PGNFileReader: unable to allocate buffer\n");
    exit(EXIT_FAILURE);
  }
}

PGNFileReader::~PGNFileReader() {
  delete[] buffer_;
}

void PGNFileReader::read(std::string_view path) {
  readpos_    = 0;
  fillpos_    = 0;
  line_       = 1;
  pos_        = 1;
  token_      = None;
  strict_     = true;
  game_count_ = 0;
  fmt::print("attempting to open {}\n", path);

  file_ = std::make_unique<PGNFile>(path.data(), O_RDONLY, 0);

  if (file_ == nullptr)
  {
    fmt::print(stderr, "PGNFileReader::read: unable to create a File\n");
    exit(EXIT_FAILURE);
  }
  read();
}

void PGNFileReader::read() {
  try
  {
    read_token(token_);
    read_pgn_database();

    if (token_ != None)
      throw UnexpectedToken("no more tokens", token_str, line_);
  } catch (const UnexpectedToken &e)
  {
    fmt::print(stderr, "%s\n", e.str());
  }
}

void PGNFileReader::read_pgn_database() {
  while (start_of_pgn_game())
  {
    try
    {
      read_pgn_game();
    } catch (...)
    {
      do
      {
        read_token(token_);

        if (token_ == None)
          break;
      }
      while (!start_of_pgn_game());
    }
  }
}

void PGNFileReader::read_pgn_game() {
  read_tag_section();
  read_move_text_section();
  ++game_count_;
}

void PGNFileReader::read_tag_section() {
  while (start_of_tag_pair(token_))
  {
    read_tag_pair();

    if (token_ != RBracket)
      throw UnexpectedToken(RBracket, token_str, line_);

    read_token(token_);
  }
}

void PGNFileReader::read_tag_pair() {
  read_token(token_);

  if (token_ != Symbol)
    throw UnexpectedToken(Symbol, token_str, line_);

  read_tag_name();

  if (token_ != String)
    throw UnexpectedToken(String, token_str, line_);

  read_tag_value();
}

void PGNFileReader::read_tag_name() {
  strcpy(tag_name_, token_str);
  read_token(token_);
}

void PGNFileReader::read_tag_value() {
  strcpy(tag_value_, token_str);
  read_token(token_);
}

void PGNFileReader::read_move_text_section() {
  read_element_sequence();

  if (start_of_game_termination())
    read_game_termination();
  else
    throw UnexpectedToken("<game-termination>", token_str, line_);
}

void PGNFileReader::read_element_sequence() {
  do
  {
    if (start_of_element())
      read_element();
    else if (start_of_recursive_variation(token_))
      read_recursive_variation();
    else
      break;
  }
  while (true);
}

void PGNFileReader::read_element() {
  if (start_of_move_number_indication())
    read_move_number_indication();
  else if (start_of_san_move())
  {
    read_san_move();
    side_to_move = ~side_to_move;
    read_token(token_);
  } else if (start_of_numeric_annotation_glyph())
    read_numeric_annotation_glyph();
}

void PGNFileReader::read_game_termination() { read_token(token_); }

void PGNFileReader::read_move_number_indication() {
  move_number_ = strtol(token_str, nullptr, 10);

  auto periods = 0;

  do
  {
    read_token(token_);

    if (token_ != Period)
      break;

    periods++;
  }
  while (true);

  side_to_move = periods >= 3 ? BLACK : WHITE;
}

void PGNFileReader::read_san_move() {
  from_piece_  = -1;
  from_file_   = -1;
  from_rank_   = -1;
  from_square_ = no_square;
  to_square_   = no_square;
  promoted_to  = -1;
  pawn_move_   = false;
  castle_move_ = false;
  piece_move_  = false;
  capture_     = false;

  char *p = token_str;

  if (start_of_pawn_move(p))
    read_pawn_move(p);
  else if (start_of_castle_move(p))
    read_castle_move(p);
  else if (start_of_move(p))
    read_move(p);

  while (read_san_move_suffix(p));// may be too relaxed

  if (strlen(token_str) != reinterpret_cast<size_t>(p) - reinterpret_cast<size_t>(token_str))
    throw UnexpectedToken("<end-of-san-move>", p, line_);
}

bool PGNFileReader::read_san_move_suffix(char *&p) {
  const auto len = strlen(p);

  if (len && (p[0] == '+' || p[0] == '#'))
    p += 1;  // NOLINT(bugprone-branch-clone)
  else if (len > 1 && (strncmp(p, "!!", 2) == 0 || strncmp(p, "!?", 2) == 0 || strncmp(p, "?!", 2) == 0 || strncmp(p, "??", 2) == 0))
    p += 2;
  else if ((len && p[0] == '!') || (strlen(p) && p[0] == '?'))
    p += 1;
  else
    return false;

  return true;
}

void PGNFileReader::read_pawn_move(char *&p) {
  if (is_pawn_piece_letter(p))
  {
    if (start_of_pawn_capture_or_quiet_move(++p))
      read_pawn_capture_or_quiet_move(p);
    else
      throw UnexpectedToken("start-of-pawn-capture-or-quiet-move", token_str, line_);
  } else if (start_of_pawn_capture_or_quiet_move(p))
    read_pawn_capture_or_quiet_move(p);

  pawn_move_ = true;
}

void PGNFileReader::read_pawn_capture_or_quiet_move(char *&p) {
  if (start_of_pawn_capture(p))
    read_pawn_capture(p);
  else if (start_of_pawn_quiet_move(p, to_square_))
    read_pawn_quiet_move(p);

  if (start_of_promoted_to(p))
    read_promoted_to(p);
}

void PGNFileReader::read_pawn_capture(char *&p) {
  p += 2;

  if (!is_square(p, to_square_))
    exit(0);

  p += 2;
  capture_ = true;
}

void PGNFileReader::read_promoted_to(char *&p) {
  p += 1;

  if (strlen(p) && is_non_pawn_piece_letter(p, promoted_to))
    p += 1;
  else
    throw UnexpectedToken("<piece-letter>", p, line_);
}

void PGNFileReader::read_move(char *&p) {
  p += 1;

  if (!start_of_capture_or_quiet_move(p))
    exit(0);

  read_capture_or_quiet_move(p);
  piece_move_ = true;
}

void PGNFileReader::read_capture_or_quiet_move(char *&p) {
  if (start_of_capture(p))
    read_capture(p);
  else if (start_of_quiet_move(p))
    read_quiet_move(p);
}

void PGNFileReader::read_capture(char *&p) {
  if (p[0] == 'x')
    p += 1;
  else if (p[1] == 'x' && (is_rank_digit(p, from_rank_) || is_file_letter(p, from_file_)))
    p += 2;
  else if (p[2] == 'x' && is_square(p, from_square_))
    p += 4;

  if (is_square(p, to_square_))
    p += 2;
  else
    throw UnexpectedToken("<to-square>", token_str, line_);

  capture_ = true;
}

void PGNFileReader::read_castle_move(char *&p) {
  const int len = strlen(p);

  constexpr auto queen_side_length = 5;
  constexpr auto king_side_length  = 3;

  if (len >= queen_side_length && strncmp(p, "O-O-O", queen_side_length) == 0)
  {
    to_square_ = ooo_king_to[side_to_move];
    p += queen_side_length;
  } else if (len >= king_side_length && strncmp(p, "O-O", king_side_length) == 0)
  {
    to_square_ = oo_king_to[side_to_move];
    p += king_side_length;
  } else
  {
    // error
  }
  from_piece_  = 'K';
  castle_move_ = true;
}

void PGNFileReader::read_quiet_move(char *&p) {
  if (is_square(p, to_square_))
    p += 2;
  else if (is_rank_digit(p, from_rank_) || is_file_letter(p, from_file_))
  {
    p += 1;

    if (is_square(p, to_square_))
      p += 2;
    else
      throw UnexpectedToken("<to-square>", token_str, line_);
  } else
  {
    // error
  }
}

void PGNFileReader::read_numeric_annotation_glyph() { read_token(token_); }

void PGNFileReader::read_recursive_variation() {
  read_token(token_);
  read_element_sequence();

  if (token_ != RParen)
    throw UnexpectedToken(RParen, token_str, line_);

  read_token(token_);
}

bool PGNFileReader::start_of_pgn_game() { return start_of_tag_section(token_) || start_of_move_text_section(); }

bool PGNFileReader::start_of_move_text_section() { return start_of_element(); }

bool PGNFileReader::start_of_element() { return start_of_move_number_indication() || start_of_san_move() || start_of_numeric_annotation_glyph(); }

bool PGNFileReader::start_of_game_termination() {
  if (token_ != Symbol)
    return false;

  if (strcmp(token_str, "1-0") == 0)
    result_ = WhiteWin;
  else if (strcmp(token_str, "1/2-1/2") == 0)
    result_ = Draw;
  else if (strcmp(token_str, "0-1") == 0)
    result_ = BlackWin;
  else
    return false;

  return true;
}

bool PGNFileReader::start_of_move_number_indication() const { return token_ == Integer; }

bool PGNFileReader::start_of_san_move() { return token_ == Symbol && (start_of_pawn_move(token_str) || start_of_castle_move(token_str) || start_of_move(token_str)); }

bool PGNFileReader::start_of_pawn_move(const char *p) { return is_pawn_piece_letter(p) || start_of_pawn_capture_or_quiet_move(p); }

bool PGNFileReader::is_pawn_piece_letter(const char *p) const { return token_ == Symbol && strlen(p) && p[0] == 'P'; }

bool PGNFileReader::start_of_pawn_capture_or_quiet_move(const char *p) { return start_of_pawn_capture(p) || start_of_pawn_quiet_move(p, to_square_); }

bool PGNFileReader::start_of_pawn_capture(const char *p) { return (strlen(p) > 1 && p[1] == 'x' && is_file_letter(p, from_file_)); }

bool PGNFileReader::start_of_move(const char *p) { return is_non_pawn_piece_letter(p, from_piece_); }

bool PGNFileReader::is_non_pawn_piece_letter(const char *p, int &piece_letter) const {
  if (token_ == Symbol && strlen(p) && (p[0] == 'N' || p[0] == 'B' || p[0] == 'R' || p[0] == 'Q' || p[0] == 'K'))
  {
    piece_letter = p[0];  // NOLINT(bugprone-signed-char-misuse)
    return true;
  }
  return false;
}

bool PGNFileReader::start_of_capture_or_quiet_move(const char *p) { return start_of_capture(p) || start_of_quiet_move(p); }

bool PGNFileReader::start_of_capture(const char *p) {
  return (strlen(p) && p[0] == 'x') || (strlen(p) > 1 && p[1] == 'x' && is_rank_digit(p, from_rank_)) || (strlen(p) > 1 && p[1] == 'x' && is_file_letter(p, from_file_))
         || (strlen(p) > 2 && p[2] == 'x' && is_square(p, from_square_));
}

bool PGNFileReader::start_of_quiet_move(const char *p) {
  return (strlen(p) > 1 && is_square(p, from_square_)) || (strlen(p) && is_rank_digit(p, from_rank_)) || (strlen(p) && is_file_letter(p, from_file_));
}

bool PGNFileReader::start_of_numeric_annotation_glyph() { return strlen(token_str) && token_str[0] == '$'; }

//---------
// scan for tokens
//
void PGNFileReader::read_token(Token &token) {
  do
  {
    read_next_token(token);

    if (strict_ || (token != Invalid && token != LT && token != GT))
      break;
  }
  while (true);
}

void PGNFileReader::read_next_token(Token &token) {
  const auto get = (token != Symbol && token != Integer && token != String && token != NAG);

  if (get || is_white_space(ch_) || ch_ == '{' || ch_ == ';')
  {
    const auto n = get_char(ch_, get, true, true);

    if (n == -1)
      throw 0;

    if (n == 0)
    {
      token        = None;
      token_str[0] = '\0';
      return;
    }
  }

  if (read_symbol())
    return;
  if (read_nag())
    return;
  if (read_string())
    return;

  switch (ch_)
  {
  case '[':
    token = LBracket;
    break;

  case ']':
    token = RBracket;
    break;

  case '(':
    token = LParen;
    break;

  case ')':
    token = RParen;
    break;

  case '.':
    token = Period;
    break;

  case '*':
    token = Asterisk;
    break;

  case '<':
    token = LT;
    break;

  case '>':
    token = GT;
    break;

  default:
    token = Invalid;
    break;
  }
  token_str[0] = ch_;
  token_str[1] = '\0';
}

int PGNFileReader::get_char(unsigned char &c) {
  auto escape = false;

  do
  {
    if (fillpos_ <= readpos_)
    {
      fillpos_ = fillpos_ % bufsize;

      const int n = file_->read(buffer_ + fillpos_, bufsize - fillpos_);

      if (n > 0)
        fillpos_ = fillpos_ + n;
      else
        return n;
    }
    readpos_ = readpos_ % bufsize;
    c        = buffer_[readpos_++];

    if (c == 0x0a || c == 0x0d)
    {
      escape = false;
      pos_   = 1;
      line_++;// not always
    } else
    {
      if (++pos_ == 2 && c == '%')
        escape = true;
    }
  }
  while (escape);
  return 1;
}

bool PGNFileReader::read_symbol() {
  if (!std::isalnum(ch_))
    return false;

  auto len    = 0;
  auto digits = true;

  do
  {
    digits           = digits && std::isdigit(ch_) != 0;
    token_str[len++] = ch_;

    const auto n = get_char(ch_, true, false, false);

    if (n == -1)
      throw 0;
    if (n == 0)
      break;

    if (!std::isalnum(ch_) && ch_ != '_' && ch_ != '+' && ch_ != '/' && ch_ != '#' && ch_ != '=' && ch_ != ':' && ch_ != '-')
      break;
  }
  while (true);

  while (ch_ == '!' || ch_ == '?')
  {
    digits           = false;
    token_str[len++] = ch_;

    const int n = get_char(ch_, true, false, false);

    if (n == -1)
      throw 0;
    if (n == 0)
      break;
  }

  token_         = digits ? Integer : Symbol;
  token_str[len] = '\0';
  return true;
}

bool PGNFileReader::read_nag() {
  if (ch_ != '$')
    return false;

  int len = 0;

  do
  {
    token_str[len++] = ch_;

    const auto n = get_char(ch_, true, false, false);

    if (n == -1)
      throw 0;

    if (n == 0)
      break;
  } while (std::isdigit(ch_));

  token_str[len] = '\0';

  if (len < 2)
    return false;

  token_ = NAG;
  return true;
}

bool PGNFileReader::read_string() {
  if (ch_ != '\"')
    return false;

  auto len  = 0;
  auto i    = 0;
  char prev = 0;

  do
  {
    token_str[len++] = ch_;

    if (ch_ == '\"' && prev != '\\')
      i++;

    prev = ch_;

    const auto n = get_char(ch_, true, false, false);

    if (n == -1)
      throw 0;
    if (n == 0)
      break;

    if (i == 2)
    {
      token_ = String;
      break;
    }
  }
  while (true);
  token_str[len] = '\0';
  return true;
}

int PGNFileReader::get_char(unsigned char &c, bool get, const bool skip_ws, const bool skip_comment) {
  do
  {
    if (get)
    {
      const auto n = get_char(c);

      if (n <= 0)
        return n;
    } else
      get = true;

    if (skip_ws && is_white_space(c))
      continue;

    if (skip_comment && c == '{')
      read_comment1();
    else if (skip_comment && c == ';')
    {
      read_comment2(c);
      get = false;
    } else
      break;
  }
  while (true);

  return 1;
}

void PGNFileReader::read_comment1() {
  unsigned char c;
  auto *p = comment_;

  do
  {
    const auto n = get_char(c);

    if (n <= 0)
      return;

    if (c == '}')
      break;

    if (p - comment_ <= 2047)
    {
      *p = c;
      ++p;
    }
  }
  while (true);

  *p = '\0';
}

void PGNFileReader::read_comment2(unsigned char &c) {
  do
  {
    auto n = get_char(c);

    if (n <= 0)
      return;

    if (c == 0x0a || c == 0x0d)
    {
      do
      {
        n = get_char(c);

        if (n <= 0)
          return;

        if (c != 0x0a && c != 0x0d)
          break;
      }
      while (true);
      break;
    }
  }
  while (true);
}

}
