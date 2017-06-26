#pragma pack(1)
struct HashEntry
    {
    uint32_t key;
    uint16_t age;  // 7 bits left
    uint8_t depth;
    uint8_t flags; // 5 bits left
    int16_t score;
    uint32_t move;
    int16_t eval;
    };
#pragma pack()

class HashTable
    {
    public:

    HashTable (uint64_t size_mb) :
        table(NULL),
        size_mb(0)
        {
        if (sizeof(HashEntry) != 16)
            {
            printf("error sizeof(HashEntry) == %d\n", static_cast<int>(sizeof(HashEntry)));
            exit(0);
            }
        init(size_mb);
        }

    void init (uint64_t new_size_mb)
        {
        new_size_mb = pow2(log2(new_size_mb));

        if (new_size_mb == size_mb)
            {
            return;
            }
        size_mb = new_size_mb;
        size = 1024 * 1024 * size_mb / sizeof(HashEntry);
        mask = size - 1;
        size += NUMBER_SLOTS - 1;
        delete [] table;
        table = new HashEntry[size];
        clear();
        }

    __forceinline void clear ()
        {
        memset(table, 0, size * sizeof(HashEntry));
        occupied = 0;
        age = 0;
        }

    void initSearch ()
        {
        age++;
        }

    __forceinline HashEntry * find (const uint64_t key)
        {
        HashEntry * transp = table + (key & mask);

        for ( int i = 0; i < NUMBER_SLOTS; i++, transp++ )
            {
            if (transp->key == key32(key) && transp->flags)
                {
                return transp;
                }
            }
        return 0;
        }

    __forceinline HashEntry * insert (const uint64_t key, const int depth, const int score, const int type,
        const int move, int eval)
        {
        HashEntry * transp = getEntryToReplace(key, depth);

        if (transp->flags == 0)
            {
            occupied++;
            }
        transp->move = transp->key != key32(key) || move != 0 ? move : transp->move;
        transp->key = key32(key);
        transp->score = (int16_t)score;
        transp->depth = (uint8_t)depth;
        transp->flags = (uint8_t)type;
        transp->age = (uint16_t)age;
        transp->eval = (int16_t)eval;
        return transp;
        }

    __forceinline HashEntry * getEntryToReplace (uint64_t key, int depth)
        {
        HashEntry * transp = table + (key & mask);

        if (transp->flags == 0 || transp->key == key32(key))
            {
            return transp;
            }
        HashEntry * replace = transp++;
        int replace_score = (replace->age << 9) + replace->depth;

        for ( int i = 1; i < NUMBER_SLOTS; i++, transp++ )
            {
            if (transp->flags == 0 || transp->key == key32(key))
                {
                return transp;
                }
            int score = (transp->age << 9) + transp->depth;

            if (score < replace_score)
                {
                replace_score = score;
                replace = transp;
                }
            }
        return replace;
        }

    __forceinline int getLoad ()
        {
        return (int)((double)occupied / size * 1000);
        }

    int getSizeMb ()
        {
        return (int)size_mb;
        }

    __forceinline

    static uint32_t key32 (const uint64_t key)
        {
        return key >> 32;
        }
    protected:
    HashEntry * table;
    uint64_t mask;
    uint64_t occupied;
    uint64_t size_mb;
    uint64_t size;
    int age;

    static const int NUMBER_SLOTS = 4;
    };

#pragma pack(1)

struct PawnHashEntry
    {
    uint64_t zkey;
    int16_t eval_mg;
    int16_t eval_eg;
    uint8_t passed_pawn_files[2];
    int16_t unused;
    };
#pragma pack()

class PawnHashTable
    {
    public:

    PawnHashTable (uint64_t size_mb) :
        table(NULL)
        {
        if (sizeof(PawnHashEntry) != 16)
            {
            printf("error sizeof(PawnHashEntry) == %d\n", static_cast<int>(sizeof(PawnHashEntry)));
            exit(0);
            }
        init(size_mb);
        }

    void init (uint64_t size_mb)
        {
        size = 1024 * 1024 * pow2(log2(size_mb)) / sizeof(PawnHashEntry);
        mask = size - 1;
        delete [] table;
        table = new PawnHashEntry[size];
        clear();
        }

    __forceinline void clear ()
        {
        memset(table, 0, size * sizeof(PawnHashEntry));
        }

    __forceinline PawnHashEntry * find (const uint64_t key)
        {
        PawnHashEntry * pawnp = table + (key & mask);

        if (pawnp->zkey != key || pawnp->zkey == 0)
            {
            return 0;
            }
        return pawnp;
        }

    __forceinline PawnHashEntry * insert (const uint64_t key, int score_mg, int score_eg, int * passed_pawn_files)
        {
        PawnHashEntry * pawnp = table + (key & mask);
        pawnp->zkey = key;
        pawnp->eval_mg = (int16_t)score_mg;
        pawnp->eval_eg = (int16_t)score_eg;
        pawnp->passed_pawn_files[0] = (uint8_t)passed_pawn_files[0];
        pawnp->passed_pawn_files[1] = (uint8_t)passed_pawn_files[1];
        return pawnp;
        }
    protected:
    PawnHashEntry * table;
    uint64_t size;
    uint64_t mask;
    };

typedef HashTable Hash;
typedef PawnHashTable PawnHash;