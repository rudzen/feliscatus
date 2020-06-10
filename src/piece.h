const int Pawn = 0;
const int Knight = 1;
const int Bishop = 2;
const int Rook = 3;
const int Queen = 4;
const int King = 5;
const int NoPiece = 6;

const static char piece_notation [] = " nbrqk";

const char * pieceToString (int piece, char * buf)
    {
    sprintf(buf, "%c", piece_notation[piece]);
    return buf;
    }
