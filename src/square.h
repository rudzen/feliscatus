namespace squares
    {
    __forceinline int rankOf (const uint64_t sq)
        {
        return sq >> 3;
        }

    __forceinline int fileOf (const uint64_t sq)
        {
        return sq & 7;
        }

    __forceinline uint64_t square (int f, int r)
        {
        return (r << 3) + f;
        }

    __forceinline bool isDark (const uint64_t sq)
        {
        return ((9 * sq) & 8) == 0;
        }

    __forceinline bool sameColor (const uint64_t sq1, const uint64_t sq2)
        {
        return isDark(sq1) == isDark(sq2);
        }

    enum Square
        {
		a1, b1, c1, d1, e1, f1, g1, h1,
		a2, b2, c2, d2, e2, f2, g2, h2,
		a3, b3, c3, d3, e3, f3, g3, h3,
		a4, b4, c4, d4, e4, f4, g4, h4,
		a5, b5, c5, d5, e5, f5, g5, h5,
		a6, b6, c6, d6, e6, f6, g6, h6,
		a7, b7, c7, d7, e7, f7, g7, h7,
		a8, b8, c8, d8, e8, f8, g8, h8
        };

    static uint32_t oo_allowed_mask[2] =
        {
        1, 4
        };

    static uint32_t ooo_allowed_mask[2] =
        {
        2, 8
        };

    static uint32_t oo_king_from[2];
    static uint32_t oo_king_to[2] =
        {
        g1, g8
        };

    static uint32_t ooo_king_from[2];
    static uint32_t ooo_king_to[2] =
        {
        c1, c8
        };

    static uint32_t rook_castles_to[64];   // indexed by position of the king
    static uint32_t rook_castles_from[64]; // also
    static uint32_t castle_rights_mask[64];
    static uint32_t dist[64][64];      // chebyshev distance
    static uint32_t flip[2][64];

    static void init ()
        {
        for ( uint32_t sq = 0; sq < 64; sq++ )
            {
            flip[0][sq] = fileOf(sq) + ((7 - rankOf(sq)) << 3);
            flip[1][sq] = fileOf(sq) + (rankOf(sq) << 3);
            }

        for ( uint32_t sq1 = 0; sq1 < 64; sq1++ )
            {
            for ( uint32_t sq2 = 0; sq2 < 64; sq2++ )
                {
                uint32_t ranks = abs(rankOf(sq1) - rankOf(sq2));
                uint32_t files = abs(fileOf(sq1) - fileOf(sq2));
                dist[sq1][sq2] = max(ranks, files);
                }
            }

        for ( int side = 0; side <= 1; side++ )
            {
            rook_castles_to[flip[side][g1]] = flip[side][f1];
            rook_castles_to[flip[side][c1]] = flip[side][d1];
            }
        // Arrays castle_right_mask, rook_castles_from, ooo_king_from and oo_king_from
        // are initd in method setupCastling of class Game.
        }

    const char * squareToString (const uint64_t sq, char * buf)
        {
        sprintf(buf, "%c%d", (char)(fileOf(sq) + 'a'), rankOf(sq) + 1);
        return buf;
        }
    } //namespace squares

using namespace squares;