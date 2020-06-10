#include "define.h"
#include "pgn.h"
#include "util.h"
#include "square.h"
#include "bitboard.h"
#include "magic.h"
#include "piece.h"
#include "move.h"
#include "board.h"
#include "hash.h"
#include "material.h"
#include "moves.h"
#include "zobrist.h"
#include "position.h"
#include "game.h"
#include "see.h"
#include "eval.h"
#include "uci.h"
#include "search.h"
#include "pgn_player.h"
#include "tune.h"
#include "perft.h"
#include "worker.h"
#include "tomcat.h"

int main ()
    {
    Tomcat * engine = new Tomcat();
	engine->init();
    engine->run();
    }