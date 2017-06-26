class Tomcat :
    public ProtocolListener
    {
    public:

    Tomcat () :
        num_threads(1)
        {
        }

    virtual ~Tomcat ()
        {
        }

    virtual int newGame ()
        {
        game->newGame(Game::kStartPosition);
        pawnt->clear();
        transt->clear();
        return 0;
        }

    virtual int setFen (const char * fen)
        {
        return game->newGame(fen);
        }

    virtual int go (int wtime = 0, int btime = 0, int movestogo = 0, int winc = 0, int binc = 0, int movetime = 5000)
        {
        game->pos->pv_length = 0;

        if (game->pos->pv_length == 0)
            {
            goSearch(wtime, btime, movestogo, winc, binc, movetime);
            }

        if (game->pos->pv_length)
            {
            char best_move[12];
            char ponder_move[12];

            protocol->postMoves(game->moveToString(search->pv[0][0].move, best_move),
                game->pos->pv_length > 1 ? game->moveToString(search->pv[0][1].move, ponder_move) : 0);

            game->makeMove(search->pv[0][0].move, true, true);
            }
        return 0;
        }

    virtual void ponderHit ()
        {
        search->search_time += search->start_time.millisElapsed();
        }

    virtual void stop ()
        {
        search->stop_search = true;
        }

    virtual bool makeMove (const char * m)
        {
        const uint32_t * move = game->pos->stringToMove(m);

        if (move)
            {
            return game->makeMove(* move, true, true);
            }
        return false;
        }

    void goSearch (int wtime, int btime, int movestogo, int winc, int binc, int movetime)
        {
        // Shared transposition table
        startWorkers();
        search->go(wtime, btime, movestogo, winc, binc, movetime, num_threads);
        stopWorkers();
        }

    void startWorkers ()
        {
        for ( int i = 0; i < num_threads - 1; i++ )
            {
            workers[i].start(game, transt, pawnt);
            }
        }

    void stopWorkers ()
        {
        for ( int i = 0; i < num_threads - 1; i++ )
            {
            workers[i].stop();
            }
        }

    virtual int setOption (const char * name, const char * value)
        {
        char buf[1024];
        strcpy(buf, "");
        ;

        if (value != NULL)
            {
            if (strieq("Hash", name))
                {
                transt->init(min(65536, max(8, (int)strtol(value, NULL, 10))));
                _snprintf(buf, sizeof(buf), "Hash:%d", transt->getSizeMb());
                }
            else if (strieq("Threads", name) || strieq("NumThreads", name))
                {
                num_threads = min(64, max(1, (int)strtol(value, NULL, 10)));
                _snprintf(buf, sizeof(buf), "Threads:%d", num_threads);
                }
            else if (strieq("UCI_Chess960", name))
                {
                if (strieq(value, "true"))
                    {
                    game->chess960 = true;
                    }
                _snprintf(buf, sizeof(buf), "UCI_Chess960 ", game->chess960 ? on : off);
                }
            else if (strieq("UCI_Chess960_Arena", name))
                {
                if (strieq(value, "true"))
                    {
                    game->chess960 = true;
                    game->xfen = true;
                    }
                }
            }
        return 0;
        }

	void init()
		{
		bitboard::init();
		attacks::init();
		zobrist::init();
		squares::init();
		}

    int run ()
        {
        setbuf(stdout, NULL);
        //setbuf(stdin, NULL);

        bitboard::init();
        attacks::init();
		zobrist::init();
        squares::init();

        game = new Game();
        protocol = new UCIProtocol(this, game);
        transt = new HashTable(256);
        pawnt = new PawnHashTable(8);
        see = new See(game);
        eval = new Eval(* game, pawnt);
        search = new Search(protocol, game, eval, see, transt);

        newGame();

        bool console_mode = true;
        int quit = 0;
		char line[16384];

        while (quit == 0)
            {
            game->pos->generateMoves();

			(void)fgets(line, 16384, stdin);

			if (feof(stdin))
				exit(0);

            char * tokens[1024];
            int num_tokens = tokenize(trim(line), tokens, 1024);

            if (num_tokens == 0)
                {
                continue;
                }
            if (strieq(tokens[0], "uci") || ! console_mode)
                {
                quit = protocol->handleInput((const char * * )tokens, num_tokens);
                console_mode = false;
                }
			else if (strieq(tokens[0], "go"))
				{
				protocol->setFlags(INFINITE_MOVE_TIME);
				go();
				}
            else if (strieq(tokens[0], "perft"))
                {
                Perft(game).perft(6);
                }
            else if (strieq(tokens[0], "divide"))
                {
                Perft(game).perft_divide(6);
                }
            else if (strieq(tokens[0], "tune"))
                {
                Stopwatch sw;
                eval::Tune(* game, * see, * eval);
                double seconds = sw.millisElapsed() / 1000.;
                printf("%f\n", seconds);
                }
			else if (strieq(tokens[0], "quit"))
				{
				quit = 1;
				}
			else if (strieq(tokens[0], "exit"))
				{
				quit = 1;
				}

            for ( int i = 0; i < num_tokens; i++ )
                {
                delete [] tokens[i];
                }
            }
        delete game;
        delete protocol;
        delete pawnt;
        delete eval;
        delete see;
        delete transt;
        delete search;
        return 0;
        }

    public:
    Game * game;
    Eval * eval;
    See * see;
    Search * search;
    Protocol * protocol;
    Hash * transt;
    PawnHash * pawnt;
    Worker workers[64];
    int num_threads;

    static const char * on;
    static const char * off;
    };

const char * Tomcat::on = "ON";
const char * Tomcat::off = "OFF";