class Board
    {
    public:

    void clear ()
        {
        memset(piece, 0, sizeof(piece));
        memset(occupied_by_side, 0, sizeof(occupied_by_side));

        for ( uint32_t sq = 0; sq < 64; sq++ )
            board[sq] = NoPiece;
        king_square[0] = king_square[1] = 64;
        occupied = 0;
        }

    void addPiece (const int p, const int side, const uint64_t sq)
        {
        piece[p + (side << 3)] |= bbSquare(sq);
        occupied_by_side[side] |= bbSquare(sq);
        occupied |= bbSquare(sq);
        board[sq] = p + (side << 3);

        if (p == King)
            {
            king_square[side] = sq;
            }
        }

    __forceinline void removePiece (const int p, const int sq)
        {
        piece[p] &= ~bbSquare(sq);
        occupied_by_side[p >> 3] &= ~bbSquare(sq);
        occupied &= ~bbSquare(sq);
        board[sq] = NoPiece;
        }

    __forceinline void addPiece (const int p, const int sq)
        {
        piece[p] |= bbSquare(sq);
        occupied_by_side[p >> 3] |= bbSquare(sq);
        occupied |= bbSquare(sq);
        board[sq] = p;
        }

    void makeMove (const uint32_t m)
        {
        if (! isCastleMove(m))
            {
            removePiece(movePiece(m), moveFrom(m));

            if (isEpCapture(m))
                {
                if (movePiece(m) < 8)
                    {
                    removePiece(moveCaptured(m), moveTo(m) - 8);
                    }
                else
                    {
                    removePiece(moveCaptured(m), moveTo(m) + 8);
                    }
                }
            else if (isCapture(m))
                {
                removePiece(moveCaptured(m), moveTo(m));
                }

            if (moveType(m) & PROMOTION)
                {
                addPiece(movePromoted(m), moveTo(m));
                }
            else
                {
                addPiece(movePiece(m), moveTo(m));
                }
            }
        else
            {
            removePiece(Rook + sideMask(m), rook_castles_from[moveTo(m)]);
            removePiece(movePiece(m), moveFrom(m));
            addPiece(Rook + sideMask(m), rook_castles_to[moveTo(m)]);
            addPiece(movePiece(m), moveTo(m));
            }

        if ((movePiece(m) & 7) == King)
            {
            king_square[moveSide(m)] = moveTo(m);
            }
        }

    void unmakeMove (const uint32_t m)
        {
        if (! isCastleMove(m))
            {
            if (moveType(m) & PROMOTION)
                {
                removePiece(movePromoted(m), moveTo(m));
                }
            else
                {
                removePiece(movePiece(m), moveTo(m));
                }

            if (isEpCapture(m))
                {
                if (movePiece(m) < 8)
                    {
                    addPiece(moveCaptured(m), moveTo(m) - 8);
                    }
                else
                    {
                    addPiece(moveCaptured(m), moveTo(m) + 8);
                    }
                }
            else if (isCapture(m))
                {
                addPiece(moveCaptured(m), moveTo(m));
                }
            addPiece(movePiece(m), moveFrom(m));
            }
        else
            {
            removePiece(movePiece(m), moveTo(m));
            removePiece(Rook + sideMask(m), rook_castles_to[moveTo(m)]);
            addPiece(movePiece(m), moveFrom(m));
            addPiece(Rook + sideMask(m), rook_castles_from[moveTo(m)]);
            }

        if ((movePiece(m) & 7) == King)
            {
            king_square[moveSide(m)] = moveFrom(m);
            }
        }

    __forceinline int getPiece (const uint64_t sq) const
        {
        return board[sq];
        }

    __forceinline int getPieceType (const uint64_t sq) const
        {
        return board[sq] & 7;
        }

    __forceinline uint64_t getPinnedPieces (const int side, const uint64_t sq)
        {
        uint64_t pinned_pieces = 0;
        int opp = side ^ 1;
        uint64_t pinners = xrayBishopAttacks(occupied, occupied_by_side[side],
            sq) & (piece[Bishop + ((opp) << 3)] | piece[Queen | ((opp) << 3)]);

        while (pinners)
            {
            pinned_pieces |= bb_between[lsb(pinners)][sq] & occupied_by_side[side];
            resetLSB(pinners);
            }
        pinners = xrayRookAttacks(occupied, occupied_by_side[side],
            sq) & (piece[Rook + (opp << 3)] | piece[Queen | (opp << 3)]);

        while (pinners)
            {
            pinned_pieces |= bb_between[lsb(pinners)][sq] & occupied_by_side[side];
            resetLSB(pinners);
            }
        return pinned_pieces;
        }

    __forceinline uint64_t xrayRookAttacks (const uint64_t & occupied, uint64_t blockers, const uint64_t sq)
        {
        uint64_t attacks = rookAttacks(sq, occupied);
        blockers &= attacks;
        return attacks ^ rookAttacks(sq, occupied ^ blockers);
        }

    __forceinline uint64_t xrayBishopAttacks (const uint64_t & occupied, uint64_t blockers, const uint64_t sq)
        {
        uint64_t attacks = bishopAttacks(sq, occupied);
        blockers &= attacks;
        return attacks ^ bishopAttacks(sq, occupied ^ blockers);
        }

    __forceinline uint64_t isOccupied (uint64_t sq)
        {
        return bbSquare(sq) & occupied;
        }

    __forceinline bool isAttacked (const uint64_t sq, const int side) const
        {
        return isAttackedBySlider(sq, side) || isAttackedByKnight(sq, side) || isAttackedByPawn(sq, side)
            || isAttackedByKing(sq, side);
        }

    __forceinline uint64_t pieceAttacks (const int piece, const uint64_t sq) const
        {
        switch (piece & 7)
            {
            case Knight:
                return knightAttacks(sq);

            case Bishop:
                return bishopAttacks(sq, occupied);

            case Rook:
                return rookAttacks(sq, occupied);

            case Queen:
                return queenAttacks(sq, occupied);

            case King:
                return kingAttacks(sq);

            default:
                break; // error
            }
        return 0;
        }

    __forceinline bool isAttackedBySlider (const uint64_t sq, const int side) const
        {
        uint64_t r_attacks = rookAttacks(sq, occupied);

        if (piece[Rook + (side << 3)] & r_attacks)
            {
            return true;
            }
        uint64_t b_attacks = bishopAttacks(sq, occupied);

        if (piece[Bishop + (side << 3)] & b_attacks)
            {
            return true;
            }
        else if (piece[Queen + (side << 3)]&(b_attacks | r_attacks))
            {
            return true;
            }
        return false;
        }

    __forceinline bool isAttackedByKnight (const uint64_t sq, const int side) const
        {
        return (piece[Knight + (side << 3)] & knight_attacks[sq]) != 0;
        }

    __forceinline bool isAttackedByPawn (const uint64_t sq, const int side) const
        {
        return (piece[Pawn | (side << 3)] & pawn_captures[sq | ((side ^ 1) << 6)]) != 0;
        }

    __forceinline bool isAttackedByKing (const uint64_t sq, const int side) const
        {
        return (piece[King | (side << 3)] & king_attacks[sq]) != 0;
        }

    void print () const
        {
        static char piece_letter [] = "PNBRQK. pnbrqk. ";
        printf("\n");

        for ( int rank = 7; rank >= 0; rank-- )
            {
            printf("%d  ", rank + 1);

            for ( int file = 0; file <= 7; file++ )
                {
                int p_and_c = getPiece(rank * 8 + file);
                printf("%c ", piece_letter[p_and_c]);
                }
            printf("\n");
            }
        printf("   a b c d e f g h\n");
        }

    __forceinline const uint64_t & pieces (int p, int side) const
        {
        return piece[p | (side << 3)];
        }

    __forceinline const uint64_t & pawns (int side) const
        {
        return piece[Pawn | (side << 3)];
        }

    __forceinline const uint64_t & knights (int side) const
        {
        return piece[Knight | (side << 3)];
        }

    __forceinline const uint64_t & bishops (int side) const
        {
        return piece[Bishop | (side << 3)];
        }

    __forceinline const uint64_t & rooks (int side) const
        {
        return piece[Rook | (side << 3)];
        }

    __forceinline const uint64_t & queens (int side) const
        {
        return piece[Queen | (side << 3)];
        }

    __forceinline const uint64_t & king (int side) const
        {
        return piece[King | (side << 3)];
        }

    uint64_t piece[2 << 3], occupied_by_side[2], occupied;
    int board[64];
    uint64_t king_square[2];
    uint64_t queen_attacks;

    __forceinline bool isPawnPassed (const uint64_t sq, const int side) const
        {
        return (passed_pawn_front_span[side][sq] & pawns(side ^ 1)) == 0;
        }

    __forceinline bool isPieceOnSquare (const int p, const uint64_t sq, const int side)
        {
        return ((bbSquare(sq) & piece[p + (side << 3)]) != 0);
        }

    __forceinline bool isPieceOnFile (const int p, const uint64_t sq, const int side) const
        {
        return ((bbFile(sq) & piece[p + (side << 3)]) != 0);
        }

    __forceinline bool isPawnIsolated (const uint64_t sq, const int side) const
        {
        const uint64_t & bb = bbSquare(sq);
        uint64_t neighbourFiles = northFill(southFill(westOne(bb) | eastOne(bb)));
        return (pawns(side) & neighbourFiles) == 0;
        }

    __forceinline bool isPawnBehind (const uint64_t sq, const int side) const
        {
        const uint64_t & bbsq = bbSquare(sq);
        return (pawns(side) & (pawnFill[side ^ 1](westOne(bbsq) | eastOne(bbsq)))) == 0;
        }
    };