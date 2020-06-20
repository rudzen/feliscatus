#pragma once

#include <memory>
#include <string_view>
#include "../src/square.h"

struct PGNFile;
enum Token : uint8_t;

enum Result : uint8_t { WhiteWin, Draw, BlackWin };

namespace pgn {

class PGNFileReader {
public:
  PGNFileReader();

  virtual ~PGNFileReader();

  virtual void read(std::string_view path);

protected:
  virtual void read();

  virtual void read_pgn_database();

  virtual void read_pgn_game();

  virtual void read_tag_section();

  virtual void read_tag_pair();

  virtual void read_tag_name();

  virtual void read_tag_value();

  virtual void read_move_text_section();

  virtual void read_element_sequence();

  virtual void read_element();

  virtual void read_game_termination();

  virtual void read_move_number_indication();

  virtual void read_san_move();

  virtual bool read_san_move_suffix(char *&p);

  virtual void read_pawn_move(char *&p);

  virtual void read_pawn_capture_or_quiet_move(char *&p);

  virtual void read_pawn_capture(char *&p);

  virtual void read_pawn_quiet_move(char *&p) { p += 2; }

  virtual void read_promoted_to(char *&p);

  virtual void read_move(char *&p);

  virtual void read_capture_or_quiet_move(char *&p);

  virtual void read_capture(char *&p);

  virtual void read_castle_move(char *&p);

  virtual void read_quiet_move(char *&p);

  virtual void read_numeric_annotation_glyph();

  virtual void read_recursive_variation();

  bool start_of_pgn_game();

  [[nodiscard]]
  bool start_of_move_text_section();

  [[nodiscard]]
  bool start_of_element();

  [[nodiscard]]
  bool start_of_game_termination();

  [[nodiscard]]
  bool start_of_move_number_indication() const;

  [[nodiscard]]
  bool start_of_san_move();

  [[nodiscard]]
  bool start_of_pawn_move(const char *p);

  [[nodiscard]]
  bool is_pawn_piece_letter(const char *p) const;

  [[nodiscard]]
  bool start_of_pawn_capture_or_quiet_move(const char *p);

  [[nodiscard]]
  bool start_of_pawn_capture(const char *p);

  [[nodiscard]]
  bool start_of_move(const char *p);

  [[nodiscard]]
  bool is_non_pawn_piece_letter(const char *p, int &piece_letter) const;

  [[nodiscard]]
  bool start_of_capture_or_quiet_move(const char *p);

  [[nodiscard]]
  bool start_of_capture(const char *p);

  [[nodiscard]]
  bool start_of_quiet_move(const char *p);

  [[nodiscard]]
  bool start_of_numeric_annotation_glyph();

  virtual void read_token(Token &token);

  virtual void read_next_token(Token &token);

  [[nodiscard]]
  int get_char(unsigned char &c);

  [[nodiscard]]
  bool read_symbol();

  [[nodiscard]]
  bool read_nag();

  [[nodiscard]]
  bool read_string();

  [[nodiscard]]
  int get_char(unsigned char &c, bool get, bool skip_ws, bool skip_comment);

  virtual void read_comment1();

  virtual void read_comment2(unsigned char &c);

  std::unique_ptr<PGNFile> file_;
  unsigned char *buffer_;
  std::size_t readpos_;
  std::size_t fillpos_;
  std::size_t line_;
  std::size_t pos_;
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
  Square from_square_;
  Square to_square_;
  int promoted_to;
  Color side_to_move;
  int move_number_;
  bool pawn_move_;
  bool castle_move_;
  bool piece_move_;
  bool capture_;
  int game_count_;
  Result result_;
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