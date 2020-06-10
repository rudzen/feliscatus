struct perft_result
    {
    perft_result ()
        {
        nodes = enpassants = castles = captures = promotions = 0;
        }
    uint64_t nodes;
    uint32_t enpassants;
    uint32_t castles;
    uint32_t captures;
    uint32_t promotions;
    };

class Perft
    {
    public:

    Perft (Game * game, int flags = LEGALMOVES)
        {
        this->game = game;
        this->flags = flags;
        }

    void perft (int depth)
        {
        perft_result result;
		double nps = 0;

        for ( int i = 1; i <= depth; i++ )
            {
            perft_result result;
            Stopwatch sw;
            perft_(i, result);
            double time = sw.millisElapsed() / (double)1000;
			if (time)
				nps = result.nodes / time;
			printf("depth %d: %llu nodes, %.2f secs, %.0f nps\n", i, result.nodes, time, nps);
            }
        }

    void perft_divide (int depth)
        {
        printf("depth: %d\n", depth);

        perft_result result;
        Position * pos = game->pos;
        char buf[12];
		double time = 0;
		double nps = 0;

        pos->generateMoves(0, 0, flags);

        while (const MoveData * move_data = pos->nextMove())
            {
            const uint32_t * m = & move_data->move;

            if (! game->makeMove(* m, flags == 0 ? true : false, true))
                {
                continue;
                }
            uint64_t nodes_start = result.nodes;
			Stopwatch sw;
            perft_(depth - 1, result);
			time += sw.millisElapsed() / (double)1000;
            game->unmakeMove();
            printf("move %s: %llu nodes\n", game->moveToString(* m, buf), result.nodes - nodes_start);
            }
		if (time)
			nps = result.nodes / time;
        printf("%llu nodes, %.0f nps\n", result.nodes, nps);
        }
	
    private:
    double total_time;

    int perft_ (int depth, perft_result & result)
        {
        if (depth == 0)
            {
            result.nodes++;
            return 0;
            }
        Position * pos = game->pos;
        pos->generateMoves(0, 0, flags);

        if ((flags & STAGES) == 0 && depth == 1)
            {
            result.nodes += pos->moveCount();
            }
        else
            {
            while (const MoveData * move_data = pos->nextMove())
                {
                const uint32_t * m = & move_data->move;

                if (! game->makeMove(* m, (flags & LEGALMOVES) ? false : true, true))
                    {
                    continue;
                    }
                perft_(depth - 1, result);
                game->unmakeMove();
                }
            }
        return 0;
        }

    Game * game;
    Search * search;
    ProtocolListener * app;
    int flags;
    };