#include <thread>

class Worker
    {
    public:

    void start (Game * master, Hash * transt, PawnHash * pawnt)
        {
        game_ = new Game();
        game_->copy(master);
        see_ = new See(game_);
        eval_ = new Eval(* game_, pawnt);
        search_ = new Search(game_, eval_, see_, transt);
        thread_ = new std::thread(& Search::run, search_);
        }

    void stop ()
        {
        search_->stop();
        thread_->join();
        delete thread_;
        delete search_;
        delete eval_;
        delete see_;
        delete game_;
        }
    private:
    Game * game_;
    Eval * eval_;
    See * see_;
    Search * search_;
    std::thread * thread_;
    };