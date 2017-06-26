const int FIXED_MOVE_TIME = 1;
const int FIXED_DEPTH = 2;
const int INFINITE_MOVE_TIME = 4;
const int PONDER_SEARCH = 8;

class ProtocolListener
    {
    public:
    virtual int newGame() = 0;
    virtual int setFen(const char * fen) = 0;
    virtual int go(int wtime, int btime, int movestogo, int winc, int binc, int movetime) = 0;
    virtual void ponderHit() = 0;
    virtual void stop() = 0;
    virtual int setOption(const char * name, const char * value) = 0;
    };

class Protocol
    {
    public:

    Protocol (ProtocolListener * callback, Game * game)
        {
        this->callback = callback;
        this->game = game;
        }

    virtual ~Protocol ()
        {
        }

    virtual int handleInput(const char * params [], int num_params) = 0;
    virtual void checkInput() = 0;
    virtual void postMoves(const char * bestmove, const char * pondermove) = 0;

    virtual void postInfo(const int depth, int selective_depth, uint64_t node_count, uint64_t nodes_per_sec,
        uint64_t time, int hash_full) = 0;

    virtual void postCurrMove(const uint32_t curr_move, int curr_move_number) = 0;

    virtual void postPV(const int depth, int max_ply, uint64_t node_count, uint64_t nodes_per_second, uint64_t time,
        int hash_full, int score, const char * pv, int node_type) = 0;

    __forceinline int isAnalysing ()
        {
        return flags &(INFINITE_MOVE_TIME | PONDER_SEARCH);
        }

    __forceinline int isFixedTime ()
        {
        return flags & FIXED_MOVE_TIME;
        }

    __forceinline int isFixedDepth ()
        {
        return flags & FIXED_DEPTH;
        }

    __forceinline int getDepth ()
        {
        return depth;
        }

    __forceinline void setFlags (int flags)
        {
        this->flags = flags;
        }
    protected:
    int flags;
    int depth;
    ProtocolListener * callback;
    Game * game;
    };

class UCIProtocol :
    public Protocol
    {
    public:

    UCIProtocol (ProtocolListener * callback, Game * game) :
        Protocol(callback, game)
        {
        }

	int InputAvailable()
		{
		//	if (stdin->cnt > 0) return 1;

		static HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
		DWORD mode;
		static BOOL console = GetConsoleMode(hInput, &mode);

		if (!console)
			{
			DWORD total_bytes_avail;
			if (!PeekNamedPipe(hInput, 0, 0, 0, &total_bytes_avail, 0))	return true;
			return total_bytes_avail;
			}
		else return _kbhit();
		}

	virtual void checkInput()
		{
		while (InputAvailable())
			{
			char str[128];
			cin.getline(str, 128);

			if (!*str) return;

			if (strieq(trim(str), "stop"))
				{
				flags &= ~(INFINITE_MOVE_TIME | PONDER_SEARCH);
				callback->stop();
				}
			else if (strieq(trim(str), "ponderhit"))
				{
				flags &= ~(INFINITE_MOVE_TIME | PONDER_SEARCH);
				callback->ponderHit();
				}
			}
		}

    void postMoves (const char * bestmove, const char * pondermove)
        {
        while (flags &(INFINITE_MOVE_TIME | PONDER_SEARCH))
            {
            Sleep(10);
            checkInput();
            }
        printf("bestmove %s", bestmove);

        if (pondermove)
            {
            printf(" ponder %s", pondermove);
            }
        printf("\n");
        }

    virtual void postInfo (const int depth, int selective_depth, uint64_t node_count, uint64_t nodes_per_sec,
        uint64_t time, int hash_full)
        {
        printf("info depth %d seldepth %d hashfull %d nodes %llu nps %llu time %llu\n",
			depth, selective_depth, hash_full, node_count, nodes_per_sec, time);
        }

    virtual void postCurrMove (const uint32_t curr_move, int curr_move_number)
        {
        char move_buf[32];

        printf("info currmove %s currmovenumber %d\n", game->moveToString(curr_move, move_buf), curr_move_number);
        }

    virtual void postPV (const int depth, int max_ply, uint64_t node_count, uint64_t nodes_per_second, uint64_t time,
        int hash_full, int score, const char * pv, int node_type)
        {
        char bound[24];

        if (node_type == 4)
            {
            strcpy(bound, "upperbound ");
            }
        else if (node_type == 2)
            {
            strcpy(bound, "lowerbound ");
            }
        else
            {
            bound[0] = 0;
            }

        printf("info depth %d seldepth %d score cp %d %s hashfull %d nodes %llu nps %llu time %llu pv %s\n",
			depth, max_ply, score, bound, hash_full, node_count, nodes_per_second, time, pv);
        }

    virtual int handleInput (const char * params [], int num_params)
        {
        if (num_params < 1)
            {
            return - 1;
            }

        if (strieq(params[0], "uci"))
            {
			printf("id name Tomcat 1.0\n");
			printf("id author Gunnar Harms\n");
			printf("option name Hash type spin default 1024 min 8 max 65536\n");
			printf("option name Ponder type check default true\n");
			printf("option name Threads type spin default 1 min 1 max 64\n");
			printf("option name UCI_Chess960 type check default false\n");
			printf("uciok\n");
            }
        else if (strieq(params[0], "isready"))
            {
            printf("readyok\n");
            }
        else if (strieq(params[0], "ucinewgame"))
            {
            callback->newGame();
            printf("readyok\n");
            }
		else if (strieq(params[0], "setoption"))
		{
			handleSetOption(params, num_params);
		}
        else if (strieq(params[0], "position"))
            {
            handlePosition(params, num_params);
            }
        else if (strieq(params[0], "go"))
            {
            handleGo(params, num_params, callback);
            }
        else if (strieq(params[0], "quit"))
            {
            return 1;
            }
        return 0;
        }

    int handleGo (const char * params [], int num_params, ProtocolListener * callback)
        {
        int wtime = 0;
        int btime = 0;
        int winc = 0;
        int binc = 0;
        int movetime = 0;
        int movestogo = 0;

        flags = 0;

        for ( int param = 1; param < num_params; param++ )
            {
            if (strieq(params[param], "movetime"))
                {
                flags |= FIXED_MOVE_TIME;

                if (++ param < num_params)
                    {
                    movetime = strtol(params[param], NULL, 10);
                    }
                }
            else if (strieq(params[param], "depth"))
                {
                flags |= FIXED_DEPTH;

                if (++ param < num_params)
                    {
                    depth = strtol(params[param], NULL, 10);
                    }
                }
            else if (strieq(params[param], "wtime"))
                {
                if (++ param < num_params)
                    {
                    wtime = strtol(params[param], NULL, 10);
                    }
                }
            else if (strieq(params[param], "movestogo"))
                {
                if (++ param < num_params)
                    {
                    movestogo = strtol(params[param], NULL, 10);
                    }
                }
            else if (strieq(params[param], "btime"))
                {
                if (++ param < num_params)
                    {
                    btime = strtol(params[param], NULL, 10);
                    }
                }
            else if (strieq(params[param], "winc"))
                {
                if (++ param < num_params)
                    {
                    winc = strtol(params[param], NULL, 10);
                    }
                }
            else if (strieq(params[param], "binc"))
                {
                if (++ param < num_params)
                    {
                    binc = strtol(params[param], NULL, 10);
                    }
                }
            else if (strieq(params[param], "infinite"))
                {
                flags |= INFINITE_MOVE_TIME;
                }
            else if (strieq(params[param], "ponder"))
                {
                flags |= PONDER_SEARCH;
                }
            }
        callback->go(wtime, btime, movestogo, winc, binc, movetime);
        return 0;
        }

    int handlePosition (const char * params [], int num_params)
        {
        int param = 1;

        if (num_params < param + 1)
            {
            return - 1;
            }
        char fen[128];

        if (strieq(params[param], "startpos"))
            {
            strcpy(fen, Game::kStartPosition);
            param++;
            }
        else if (strieq(params[param], "fen"))
            {
            if (! FENfromParams(params, num_params, param, fen))
                {
                return - 1;
                }
            param++;
            }
        else
            {
            return - 1;
            }
        callback->setFen(fen);

        if ((num_params > param) && (strieq(params[param++], "moves")))
            {
            while (param < num_params)
                {
                const uint32_t * move = game->pos->stringToMove(params[param++]);

                if (move == 0)
                    {
                    return - 1;
                    }
                game->makeMove(* move, true, true);
                }
            }
        return 0;
        }

    bool handleSetOption (const char * params [], int num_params)
        {
        int param = 1;
        const char * option_name;
        const char * option_value;

        if (parseOptionName(param, params, num_params, & option_name)
            && parseOptionValue(param, params, num_params, & option_value))
            {
            return callback->setOption(option_name, option_value) == 0;
            }
        return false;
        }

    bool parseOptionName (int & param, const char * params [], int num_params, const char * * option_name)
        {
        while (param < num_params)
            {
            if (strieq(params[param++], "name"))
                {
                break;
                }
            }

        if (param < num_params)
            {
            * option_name = params[param++];
            return * option_name != '\0';
            }
        return false;
        }

    bool parseOptionValue (int & param, const char * params [], int num_params, const char * * option_value)
        {
        * option_value = NULL;

        while (param < num_params)
            {
            if (strieq(params[param++], "value"))
                {
                break;
                }
            }

        if (param < num_params)
            {
            * option_value = params[param++];
            return * option_value != NULL;
            }
        return true;
        }
    };