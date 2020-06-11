
#pragma once

#include <fcntl.h>
#include <io.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>

namespace pgn {

class File {
public:
  File(const char *path, const int oflag, const int pmode) {
    if ((fd = open(path, oflag | O_BINARY, pmode)) == -1)
    {
      perror("File::File: cannot open file");
      exit(EXIT_FAILURE);
    }
  }

  ~File() { close(fd); }

  size_t read(unsigned char *buf, const size_t count) const {
    int n;

    if ((n = ::read(fd, static_cast<void *>(buf), count)) == -1)
    {
      perror("File::read: cannot read file");
      exit(EXIT_FAILURE);
    }
    return n;
  }

private:
  int fd;
};

enum Token { Symbol, Integer, String, NAG, Asterisk, Period, LParen, RParen, LBracket, RBracket, LT, GT, Invalid, None };

static const char token_string[][12] = {"Symbol", "Integer", "String", "NAG", "Asterisk", "Period", "LParen", "RParen", "LBracket", "RBracket", "LT", "GT", "Invalid", "None"};

enum Result { WhiteWin, Draw, BlackWin };

class UnexpectedToken {
public:
  UnexpectedToken(const Token expected, const char *found, const size_t line) { sprintf(buf, "Expected <%s> but found '%s', line=%llu", token_string[expected], found, line); }

  UnexpectedToken(const char *expected, const char *found, const size_t line) { sprintf(buf, "Expected %s but found '%s', line=%llu", expected, found, line); }

  const char *str() const { return buf; }

private:
  char buf[2048];
};

class PGNFileReader {
public:
  PGNFileReader() : file_(nullptr) {
    if ((buffer_ = new unsigned char[bufsize]) == nullptr)
    {
      fprintf(stderr, "PGNFileReader: unable to allocate buffer\n");
      exit(EXIT_FAILURE);
    }
  }

  ~PGNFileReader() {
    delete file_;
    delete[] buffer_;
  }

  virtual void read(const char *path) {
    if (file_)
    {
      delete file_;
      file_ = nullptr;
    }
    readpos_    = 0;
    fillpos_    = 0;
    line_       = 1;
    pos_        = 1;
    token_      = None;
    strict_     = true;
    game_count_ = 0;

    if ((file_ = new File(path, O_RDONLY, 0)) == nullptr)
    {
      fprintf(stderr, "PGNFileReader::read: unable to create a File\n");
      exit(EXIT_FAILURE);
    }
    read();
  }

protected:
  virtual void read() {
    try
    {
      read_token(token_);
      read_pgn_database();

      if (token_ != None)
      {
        throw UnexpectedToken("no more tokens", token_str, line_);
      }
    } catch (const UnexpectedToken &e)
    { fprintf(stderr, "%s\n", e.str()); }
  }

  virtual void read_pgn_database() {
    while (start_of_pgn_game())
    {
      try
      { readPGNGame(); } catch (...)
      {
        do
        {
          read_token(token_);

          if (token_ == None)
          {
            break;
          }
        } while (!start_of_pgn_game());
      }
    }
  }

  virtual void readPGNGame() {
    readTagSection();
    readMoveTextSection();
    ++game_count_;
  }

  virtual void readTagSection() {
    while (start_of_tag_pair())
    {
      read_tag_pair();

      if (token_ != RBracket)
      {
        throw UnexpectedToken(RBracket, token_str, line_);
      }

      read_token(token_);
    }
  }

  virtual void read_tag_pair() {
    read_token(token_);

    if (token_ != Symbol)
    {
      throw UnexpectedToken(Symbol, token_str, line_);
    }

    read_tag_name();

    if (token_ != String)
    {
      throw UnexpectedToken(String, token_str, line_);
    }
    readTagValue();
  }

  virtual void read_tag_name() {
    strcpy(tag_name_, token_str);
    read_token(token_);
  }

  virtual void readTagValue() {
    strcpy(tag_value_, token_str);
    read_token(token_);
  }

  virtual void readMoveTextSection() {
    readElementSequence();

    if (start_of_game_termination())
    {
      read_game_termination();
    } else
    { throw UnexpectedToken("<game-termination>", token_str, line_); }
  }

  virtual void readElementSequence() {
    for (;;)
    {
      if (start_of_element())
      {
        read_element();
      } else if (start_of_recursive_variation())
      {
        read_recursive_variation();
      } else
      { break; }
    }
  }

  virtual void read_element() {
    if (start_of_move_number_indication())
    {
      read_move_number_indication();
    } else if (start_of_san_move())
    {
      read_san_move();
      side_to_move ^= 1;
      read_token(token_);
    } else if (start_of_numeric_annotation_glyph())
    { read_numeric_annotation_glyph(); }
  }

  virtual void read_game_termination() { read_token(token_); }

  virtual void read_move_number_indication() {
    move_number_ = strtol(token_str, nullptr, 10);

    int periods = 0;

    for (;;)
    {
      read_token(token_);

      if (token_ != Period)
      {
        break;
      }
      periods++;
    }

    if (periods >= 3)
    {
      side_to_move = 1;
    } else
    { side_to_move = 0; }
  }

  virtual void read_san_move() {
    from_piece_  = -1;
    from_file_   = -1;
    from_rank_   = -1;
    from_square_ = -1;
    to_square_   = -1;
    promoted_to  = -1;
    pawn_move_   = false;
    castle_move_ = false;
    piece_move_  = false;
    capture_     = false;

    char *p = token_str;

    if (start_of_pawn_move(p))
    {
      read_pawn_move(p);
    } else if (start_of_castle_move(p))
    {
      read_castle_move(p);
    } else if (start_of_move(p))
    { read_move(p); }

    while (read_san_move_suffix(p))
      ;// may be too relaxed

    if (strlen(token_str) != reinterpret_cast<size_t>(p) - reinterpret_cast<size_t>(token_str))
    {
      throw UnexpectedToken("<end-of-san-move>", p, line_);
    }
  }

  virtual bool read_san_move_suffix(char *&p) {
    const size_t len = strlen(p);

    if (len && p[0] == '+')
    {
      p += 1;
    } else if (len && p[0] == '#')
    {
      p += 1;
    } else if (len > 1 && strncmp(p, "!!", 2) == 0)
    {
      p += 2;
    } else if (len > 1 && strncmp(p, "!?", 2) == 0)
    {
      p += 2;
    } else if (len > 1 && strncmp(p, "?!", 2) == 0)
    {
      p += 2;
    } else if (len > 1 && strncmp(p, "??", 2) == 0)
    {
      p += 2;
    } else if (len && p[0] == '!')
    {
      p += 1;
    } else if (strlen(p) && p[0] == '?')
    {
      p += 1;
    } else
    { return false; }
    return true;
  }

  virtual void read_pawn_move(char *&p) {
    if (is_pawn_piece_letter(p))
    {
      if (start_of_pawn_capture_or_quiet_move(++p))
      {
        read_pawn_capture_or_quiet_move(p);
      } else
      { throw UnexpectedToken("start-of-pawn-capture-or-quiet-move", token_str, line_); }
    } else if (start_of_pawn_capture_or_quiet_move(p))
    { read_pawn_capture_or_quiet_move(p); }
    pawn_move_ = true;
  }

  virtual void read_pawn_capture_or_quiet_move(char *&p) {
    if (start_of_pawn_capture(p))
      read_pawn_capture(p);
    else if (start_of_pawn_quiet_move(p, to_square_))
      read_pawn_quiet_move(p);

    if (start_of_promoted_to(p))
      read_promoted_to(p);
  }

  virtual void read_pawn_capture(char *&p) {
    p += 2;

    if (!is_square(p, to_square_))
    {
      exit(0);
    }
    p += 2;
    capture_ = true;
  }

  virtual void read_pawn_quiet_move(char *&p) { p += 2; }

  virtual void read_promoted_to(char *&p) {
    p += 1;

    if (strlen(p) && is_non_pawn_piece_letter(p, promoted_to))
    {
      p += 1;
    } else
    { throw UnexpectedToken("<piece-letter>", p, line_); }
  }

  virtual void read_move(char *&p) {
    p += 1;

    if (!start_of_capture_or_quiet_move(p))
    {
      exit(0);
    }
    read_capture_or_quiet_move(p);
    piece_move_ = true;
  }

  virtual void read_capture_or_quiet_move(char *&p) {
    if (startOfCapture(p))
    {
      read_capture(p);
    } else if (start_of_quiet_move(p))
    { read_quiet_move(p); }
  }

  virtual void read_capture(char *&p) {
    if (p[0] == 'x')
    {
      p += 1;
    } else if (p[1] == 'x' && is_rank_digit(p, from_rank_))
    {
      p += 2;
    } else if (p[1] == 'x' && is_file_letter(p, from_file_))
    {
      p += 2;
    } else if (p[2] == 'x' && is_square(p, from_square_))
    { p += 4; }

    if (is_square(p, to_square_))
    {
      p += 2;
    } else
    { throw UnexpectedToken("<to-square>", token_str, line_); }
    capture_ = true;
  }

  virtual void read_castle_move(char *&p) {
    const int len = strlen(p);

    if (len > 4 && strncmp(p, "O-O-O", 5) == 0)
    {
      to_square_ = side_to_move == 0 ? 2 : 58;
      p += 5;
    } else if (len > 2 && strncmp(p, "O-O", 3) == 0)
    {
      to_square_ = side_to_move == 0 ? 6 : 62;
      p += 3;
    } else
    {
      // error
    }
    from_piece_  = 'K';
    castle_move_ = true;
  }

  virtual void read_quiet_move(char *&p) {
    if (is_square(p, to_square_))
    {
      p += 2;
    } else if (is_rank_digit(p, from_rank_))
    {
      p += 1;

      if (is_square(p, to_square_))
      {
        p += 2;
      } else
      { throw UnexpectedToken("<to-square>", token_str, line_); }
    } else if (is_file_letter(p, from_file_))
    {
      p += 1;

      if (is_square(p, to_square_))
      {
        p += 2;
      } else
      { throw UnexpectedToken("<to-square>", token_str, line_); }
    } else
    {
      // error
    }
  }

  virtual void read_numeric_annotation_glyph() { read_token(token_); }

  virtual void read_recursive_variation() {
    read_token(token_);
    readElementSequence();

    if (token_ != RParen)
    {
      throw UnexpectedToken(RParen, token_str, line_);
    }
    read_token(token_);
  }

  bool start_of_pgn_game() { return start_of_tag_section() || start_of_move_text_section(); }

  [[nodiscard]]
  bool start_of_tag_section() const { return start_of_tag_pair(); }

  [[nodiscard]]
  bool start_of_move_text_section() { return start_of_element(); }

  [[nodiscard]]
  bool start_of_tag_pair() const { return token_ == LBracket; }

  [[nodiscard]]
  bool start_of_element() { return start_of_move_number_indication() || start_of_san_move() || start_of_numeric_annotation_glyph(); }

  [[nodiscard]]
  bool start_of_recursive_variation() const { return token_ == LParen; }

  [[nodiscard]]
  bool start_of_tag_name() const { return token_ == Symbol; }

  [[nodiscard]]
  bool start_of_tag_value() const { return token_ == String; }

  [[nodiscard]]
  bool start_of_game_termination() {
    if (token_ != Symbol)
    {
      return false;
    }

    if (strcmp(token_str, "1-0") == 0)
    {
      result_ = WhiteWin;
    } else if (strcmp(token_str, "1/2-1/2") == 0)
    {
      result_ = Draw;
    } else if (strcmp(token_str, "0-1") == 0)
    {
      result_ = BlackWin;
    } else
    { return false; }
    return true;
  }

  [[nodiscard]]
  bool start_of_move_number_indication() const { return token_ == Integer; }

  [[nodiscard]]
  bool start_of_san_move() { return token_ == Symbol && (start_of_pawn_move(token_str) || start_of_castle_move(token_str) || start_of_move(token_str)); }

  [[nodiscard]]
  bool start_of_pawn_move(const char *p) { return is_pawn_piece_letter(p) || start_of_pawn_capture_or_quiet_move(p); }

  [[nodiscard]]
  bool is_pawn_piece_letter(const char *p) const { return token_ == Symbol && strlen(p) && p[0] == 'P'; }

  [[nodiscard]]
  bool start_of_pawn_capture_or_quiet_move(const char *p) { return start_of_pawn_capture(p) || start_of_pawn_quiet_move(p, to_square_); }

  [[nodiscard]]
  bool start_of_pawn_capture(const char *p) { return (strlen(p) > 1 && p[1] == 'x' && is_file_letter(p, from_file_)); }

  [[nodiscard]]
  static bool start_of_pawn_quiet_move(const char *p, int &to_square) { return strlen(p) > 1 && is_square(p, to_square); }

  [[nodiscard]]
  static bool is_file_letter(const char *p, int &file) {
    if (strlen(p) && p[0] >= 'a' && p[0] <= 'h')
    {
      file = p[0] - 'a';
      return true;
    }
    return false;
  }

  [[nodiscard]]
  static bool is_rank_digit(const char *p, int &rank) {
    if (strlen(p) && p[0] >= '1' && p[0] <= '8')
    {
      rank = p[0] - '1';
      return true;
    }
    return false;
  }

  [[nodiscard]]
  static bool is_square(const char *p, int &square) {
    if (strlen(p) > 1 && p[0] >= 'a' && p[0] <= 'h' && p[1] >= '0' && p[1] <= '9')
    {
      square = ((p[1] - '1') << 3) + p[0] - 'a';
      return true;
    }
    return false;
  }

  [[nodiscard]]
  static bool start_of_promoted_to(const char *p) { return strlen(p) && p[0] == '='; }

  [[nodiscard]]
  bool start_of_move(const char *p) { return is_non_pawn_piece_letter(p, from_piece_); }

  [[nodiscard]]
  bool is_non_pawn_piece_letter(const char *p, int &piece_letter) const {
    if (token_ == Symbol && strlen(p) && (p[0] == 'N' || p[0] == 'B' || p[0] == 'R' || p[0] == 'Q' || p[0] == 'K'))
    {
      piece_letter = p[0];
      return true;
    }
    return false;
  }

  [[nodiscard]]
  bool start_of_capture_or_quiet_move(const char *p) { return startOfCapture(p) || start_of_quiet_move(p); }

  [[nodiscard]]
  bool startOfCapture(const char *p) {
    return (strlen(p) && p[0] == 'x') || (strlen(p) > 1 && p[1] == 'x' && is_rank_digit(p, from_rank_)) || (strlen(p) > 1 && p[1] == 'x' && is_file_letter(p, from_file_))
           || (strlen(p) > 2 && p[2] == 'x' && is_square(p, from_square_));
  }

  [[nodiscard]]
  bool start_of_quiet_move(const char *p) {
    return (strlen(p) > 1 && is_square(p, from_square_)) || (strlen(p) && is_rank_digit(p, from_rank_)) || (strlen(p) && is_file_letter(p, from_file_));
  }

  [[nodiscard]]
  static bool start_of_castle_move(const char *p) { return strlen(p) && p[0] == 'O'; }

  [[nodiscard]]
  bool start_of_numeric_annotation_glyph() { return strlen(token_str) && token_str[0] == '$'; }

  //---------
  // scan for tokens
  //
  virtual void read_token(Token &token) {
    for (;;)
    {
      read_next_token(token);

      if (strict_ || (token != Invalid && token != LT && token != GT))
      {
        break;
      }
    }
  }

  virtual void read_next_token(Token &token) {
    const bool get = (token != Symbol && token != Integer && token != String && token != NAG);

    if (get || is_white_space(ch_) || ch_ == '{' || ch_ == ';')
    {
      int n = getChar(ch_, get, true, true);

      if (n == -1)
      {
        throw 0;
      } else if (n == 0)
      {
        token        = None;
        token_str[0] = '\0';
        return;
      }
    }

    if (read_symbol())
    {
      return;
    } else if (readNAG())
    {
      return;
    } else if (readString())
    { return; }

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

  [[nodiscard]]
  int get_char(unsigned char &c) {
    bool escape = false;

    for (;;)
    {
      if (fillpos_ <= readpos_)
      {
        fillpos_ = fillpos_ % bufsize;

        int n = file_->read(buffer_ + fillpos_, bufsize - fillpos_);

        if (n > 0)
        {
          fillpos_ = fillpos_ + n;
        } else
        { return n; }
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
        {
          escape = true;
        }
      }

      if (escape == false)
      {
        break;
      }
    }
    return 1;
  }

  [[nodiscard]]
  bool read_symbol() {
    if (!isAlNum(ch_))
    {
      return false;
    }
    int len     = 0;
    bool digits = true;

    for (;;)
    {
      digits           = digits && isDigit(ch_) != 0;
      token_str[len++] = ch_;

      int n = getChar(ch_, true, false, false);

      if (n == -1)
      {
        throw 0;
      } else if (n == 0)
      { break; }

      if (!isAlNum(ch_) && ch_ != '_' && ch_ != '+' && ch_ != '/' && ch_ != '#' && ch_ != '=' && ch_ != ':' && ch_ != '-')
      {
        break;
      }
    }

    while (ch_ == '!' || ch_ == '?')
    {
      digits           = false;
      token_str[len++] = ch_;

      int n = getChar(ch_, true, false, false);

      if (n == -1)
      {
        throw 0;
      }
      if (n == 0)
        break;
    }

    if (digits)
    {
      token_ = Integer;
    } else
    { token_ = Symbol; }
    token_str[len] = '\0';
    return true;
  }

  bool readNAG() {
    if (ch_ != '$')
    {
      return false;
    }
    int len = 0;

    for (;;)
    {
      token_str[len++] = ch_;

      int n = getChar(ch_, true, false, false);

      if (n == -1)
      {
        throw 0;
      } else if (n == 0)
      { break; }

      if (!isDigit(ch_))
      {
        break;
      }
    }
    token_str[len] = '\0';

    if (len < 2)
    {
      return false;
    }
    token_ = NAG;
    return true;
  }

  bool readString() {
    if (ch_ != '\"')
    {
      return false;
    }
    int len   = 0;
    int i     = 0;
    char prev = 0;

    for (;;)
    {
      token_str[len++] = ch_;

      if (ch_ == '\"' && prev != '\\')
      {
        i++;
      }
      prev = ch_;

      int n = getChar(ch_, true, false, false);

      if (n == -1)
      {
        throw 0;
      } else if (n == 0)
      { break; }

      if (i == 2)
      {
        token_ = String;
        break;
      }
    }
    token_str[len] = '\0';
    return true;
  }

  int getChar(unsigned char &c, bool get, bool skip_ws, bool skip_comment) {
    for (;;)
    {
      if (get)
      {
        int n = get_char(c);

        if (n <= 0)
        {
          return n;
        }
      } else
      { get = true; }

      if (skip_ws && is_white_space(c))
      {
        continue;
      } else if (skip_comment && c == '{')
      {
        readComment1();
      } else if (skip_comment && c == ';')
      {
        readComment2(c);
        get = false;
      } else
      { break; }
    }
    return 1;
  }

  virtual void readComment1() {
    unsigned char c;
    char *p = comment_;

    for (;;)
    {
      int n = get_char(c);

      if (n <= 0)
      {
        return;
      }

      if (c == '}')
      {
        break;
      }

      if (p - comment_ <= 2047)
      {
        *p = c;
        ++p;
      }
    }
    *p = '\0';
  }

  virtual void readComment2(unsigned char &c) {
    for (;;)
    {
      int n = get_char(c);

      if (n <= 0)
      {
        return;
      }

      if (c == 0x0a || c == 0x0d)
      {
        for (;;)
        {
          int n = get_char(c);

          if (n <= 0)
          {
            return;
          }

          if (c != 0x0a && c != 0x0d)
          {
            break;
          }
        }
        break;
      }
    }
  }

  // TODO : replace with std

  static constexpr bool is_white_space(const char c) { return c == ' ' || c == '\t' || c == 0x0a || c == 0x0d; }

  bool isDigit(const char c) { return c >= '0' && c <= '9'; }

  bool isAlpha(const char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }

  bool isAlNum(const char c) { return isAlpha(c) || isDigit(c); }

protected:
  File *file_;
  unsigned char *buffer_;
  size_t readpos_;
  size_t fillpos_;
  size_t line_;
  size_t pos_;
  unsigned char ch_;
  Token token_;
  char token_str[1024];
  bool strict_;
  char tag_name_[1024];
  char tag_value_[1024];
  char comment_[2048];
  int from_file_;
  int from_rank_;
  int from_piece_;
  int from_square_;
  int to_square_;
  int promoted_to;
  int side_to_move;
  int move_number_;
  bool pawn_move_;
  bool castle_move_;
  bool piece_move_;
  bool capture_;
  int game_count_;
  Result result_;

private:
  static constexpr std::size_t bufsize = 128 * 1024;
};
}// namespace pgn

/*
http://www6.chessclub.com/help/PGN-spec
http://www.saremba.de/chessgml/standards/pgn/pgn-complete.htm

<PGN-database> ::= <PGN-game> <PGN-database>
                   <empty>

<PGN-game> ::= <tag-section> <movetext-section>

<tag-section> ::= <tag-pair> <tag-section>
                  <empty>

<tag-pair> ::= [ <tag-name> <tag-value> ]

<tag-name> ::= <identifier>

<tag-value> ::= <string>

<movetext-section> ::= <element-sequence> <game-termination>

<element-sequence> ::= <element> <element-sequence>
                       <recursive-variation> <element-sequence>
                       <empty>

<element> ::= <move-number-indication>
              <SAN-move>
              <numeric-annotation-glyph>

<recursive-variation> ::= ( <element-sequence> )

<game-termination> ::= 1-0
                       0-1
                       1/2-1/2
                       *
<empty> ::=







-----------------------------------------------------------------

<SAN-move> ::= <pawn-move>|<castle-move>|<piece-move> [SAN-move-suffix]

[SAN-move-suffix] ::= +|#|!|?|!!|!?|?!|!!

<pawn-move> ::= [P] <pawn-capture-or-quiet-move> [<promoted-to>]

<pawn-capture-or-quiet-move> ::= <pawn-capture>|<pawn-quiet_move>

<pawn-capture> ::= <from-file-letter> x <to-square>

<pawn-quiet-move> ::= <to-square>

<promoted-to> ::= = <piece-letter>

<castle-move> ::= O-O|O-O-O

<piece-move> ::= <piece-letter> <capture-or-quiet-move>

<piece-letter> ::= one of N, B, R, Q, K

<capture-or-quiet-move> ::= <capture>|<quiet-move>

<capture> ::= [<from-file-letter>|<from-rank-digit>|<from-square>] x <to-square>

<quiet-move> ::= [<from-file-letter>|<from-rank-digit>|<from-square>] <to-square>

<file-letter> ::= a..h

<rank-digit> ::= one of 1..8

<square> ::= a1..h8

*/