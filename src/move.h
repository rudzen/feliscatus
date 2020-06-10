__forceinline int movePiece (const uint32_t move)
    {
    return (move >> 26) & 15;
    }

__forceinline void moveSetPiece (uint32_t & move, const int piece)
    {
    move |= (piece << 26);
    }

__forceinline int moveCaptured (const uint32_t move)
    {
    return (move >> 22) & 15;
    }

__forceinline void moveSetCaptured (uint32_t & move, const int piece)
    {
    move |= (piece << 22);
    }

__forceinline int movePromoted (const uint32_t move)
    {
    return (move >> 18) & 15;
    }

__forceinline void moveSetPromoted (uint32_t & move, const int piece)
    {
    move |= (piece << 18);
    }

__forceinline int moveType (const uint32_t move)
    {
    return (move >> 12) & 63;
    }

__forceinline void moveSetType (uint32_t & move, const int type)
    {
    move |= (type << 12);
    }

__forceinline uint32_t moveFrom (const uint32_t move)
    {
    return (move >> 6) & 63;
    }

__forceinline void moveSetFrom (uint32_t & move, const int sq)
    {
    move |= (sq << 6);
    }

__forceinline uint32_t moveTo (const uint32_t move)
    {
    return move & 63;
    }

__forceinline void moveSetTo (uint32_t & move, const int sq)
    {
    move |= sq;
    }

__forceinline int movePieceType (const uint32_t move)
    {
    return (move >> 26) & 7;
    }

__forceinline int moveSide (const uint32_t m)
    {
    return (m >> 29) & 1;
    }

__forceinline int sideMask (const uint32_t m)
    {
    return moveSide(m) << 3;
    }

const int QUIET = 1;
const int DOUBLEPUSH = 2;
const int CASTLE = 4;
const int EPCAPTURE = 8;
const int PROMOTION = 16;
const int CAPTURE = 32;

__forceinline int isCapture (const uint32_t m)
    {
    return moveType(m) & (CAPTURE | EPCAPTURE);
    }

__forceinline int isEpCapture (const uint32_t m)
    {
    return moveType(m) & EPCAPTURE;
    }

__forceinline int isCastleMove (const uint32_t m)
    {
    return moveType(m) & CASTLE;
    }

__forceinline int isPromotion (const uint32_t m)
    {
    return moveType(m) & PROMOTION;
    }

__forceinline bool isQueenPromotion (const uint32_t m)
    {
    return isPromotion(m) && (movePromoted(m) & 7) == Queen;
    }

__forceinline bool isNullMove (const uint32_t m)
    {
    return m == 0;
    }

__forceinline void initMove (uint32_t & move, const int piece, const int captured, const uint64_t from, const uint64_t to,
    const int type, const int promoted)
    {
    move = 0;
    moveSetPiece(move, piece);
    moveSetCaptured(move, captured);
    moveSetPromoted(move, promoted);
    moveSetFrom(move, from);
    moveSetTo(move, to);
    moveSetType(move, type);
    }