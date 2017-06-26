namespace zobrist
    {
    uint64_t zobrist_pst[14][64];
    uint64_t zobrist_castling[16];
    uint64_t zobrist_side;
    uint64_t zobrist_ep_file[8];

    static void init ()
        {
        std::mt19937_64 prng64;
        prng64.seed(std::mt19937_64::default_seed);

        for ( int p = Pawn; p <= King; p++ )
            {
            for ( int sq = 0; sq < 64; sq++ )
                {
                zobrist_pst[p][sq] = prng64();
                zobrist_pst[p + 8][sq] = prng64();
                }
            }

        for ( int i = 0; i < 16; i++ )
            {
            zobrist_castling[i] = prng64();
            }

        for ( int i = 0; i < 8; i++ )
            {
            zobrist_ep_file[i] = prng64();
            }
        zobrist_side = prng64();
        }
    } //namespace zobrist

using namespace zobrist;