#include <pistache/endpoint.h>
#include <pistache/router.h>
#include <pistache/http.h>
#include <pistache/net.h>
#include "json.hpp"
#include <random>
#include <mutex>
#include <unordered_map>
#include <string>
#include <boost/algorithm/string.hpp>
#include <vector>
#include <ctime>
#include "cwordle.h"
#include "types.h"
#include "formatted.h"

using namespace Pistache;
using json = nlohmann::json;
using namespace std;

struct game_info;

random_device rd;
mt19937 gen(rd());
uniform_int_distribution<U32> dis(0, 1<<30);

unordered_map<U32, game_info*> games;
std::set<game_info*> old_games;
mutex games_mutex;

time_t last_purge = time(nullptr);

class RequestException : public std::exception
{
private:
    string message;
public:
    RequestException(const string_view &msg)
        : message(msg) { };
    const char *what() const noexcept
    {
        return message.c_str();
    }
};

struct game_info
{
    U32 id;
    cwordle *game;
    time_t timestamp;
    mutex my_mutex;

    game_info(cwordle *g)
        : id(dis(gen)), game(g), timestamp(time(nullptr))
    {        
    }
    void set_timestamp()
    {
        timestamp = time(nullptr);
    }
    int age() const
    {
        return time(nullptr) - timestamp;
    }
};

struct request_info
{
    json body;
    game_info *my_game_info = NULL;
    cwordle *game = NULL;
    ~request_info()
    {
        if (my_game_info) {
            my_game_info->my_mutex.unlock();
        }
    }
    /************************************************************************
     * build - ensure that the request is valid, extract the
     * game_id and find the corresponding game, also check that all
     * required fields are present. Throw a ReqestException if there is an
     * error, otherwise save the JSON body and the game pointer.
     ***********************************************************************/
    
    void build(const Rest::Request& req, const vector<string> &required_content, bool game_required = true)
    {
        try {
            body = json::parse(req.body().empty() ? "{}" : req.body());
        } catch (...) {
            throw RequestException("Invalid JSON");
        }
        if (!body.is_object()
            || (game_required && !body.count("game_id"))) {
            throw RequestException("Missing fields");
        }
        for (const string &f : required_content) {
            if (!body.count(f)) {
                throw RequestException("Missing fields");
            }
        }
        if (body.count("game_id")) {
            string game_id = body["game_id"].get<std::string>();
            U32 game_id_n = 0;
            try {
                game_id_n = lexical_cast<U32>(game_id);
            } catch (...) {
                throw RequestException("No such game");
            }
            lock_guard<mutex> lock(games_mutex);
            auto it = games.find(game_id_n);
            if (it == games.end()) {
                if (game_required) {
                    throw RequestException("No such game");
                }
            } else {
                my_game_info = it->second;
                my_game_info->set_timestamp();
                game = my_game_info->game;
                my_game_info->my_mutex.lock();
            }
        }
    }
};

/************************************************************************
 * add_cors_headers - add CORS headers to the response
 ***********************************************************************/

void add_cors_headers(Http::ResponseWriter& response) {
    response.headers().add<Http::Header::AccessControlAllowOrigin>("*");
    response.headers().add<Http::Header::AccessControlAllowMethods>("POST, GET, OPTIONS");
    response.headers().add<Http::Header::AccessControlAllowHeaders>("Content-Type");
}

void send_good_response(Http::ResponseWriter &response, const json &res)
{
    add_cors_headers(response);
    response.send(Http::Code::Ok, res.dump(), MIME(Application, Json));
}

void send_error_response(Http::ResponseWriter &response, const string &err)
{
    add_cors_headers(response);
    response.send(Http::Code::Bad_Request,
                  "{\"error\":\"" + err + "\"}",
                  MIME(Application, Json));
}

/************************************************************************
 * purge_games - called periodically to get rid of completed or abandoned games
 * and very old games
 ***********************************************************************/

const int purge_delay_over = 1*60;  // seconds
const int purge_delay_abandoned = 10; // seconds
const int purge_delay_active = 24*60*60; // seconds
const int min_purge_interval = 10; // seconds
const int purge_delete_wait = 10; // seconds

void purge_games()
{
    if (time(nullptr) - last_purge > min_purge_interval) {
        last_purge = time(nullptr);
        vector<game_info*> to_erase;
        lock_guard<mutex> lock(games_mutex);
    
        for (auto it : games) {
            auto *gi = it.second;
            if ((gi->game->is_abandoned() && gi->age() > purge_delay_abandoned)
                || (gi->game->is_over() && gi->age() > purge_delay_over)
                || gi->age() > purge_delay_active) {
                if (gi->my_mutex.try_lock()) {
                    to_erase.push_back(gi);
                }
            }
        }
        for (auto gi : to_erase) {
            // std::cout << "purge: erasing game " << gi->id << "\n";
            games.erase(gi->id);
            gi->set_timestamp();
            old_games.insert(gi);
        }
        vector<game_info*> to_delete;
        for (auto it : old_games) {
            if (it->age() > purge_delete_wait) {
                to_delete.push_back(it);
            }
        }
        for (auto gi : to_delete) {
            // std::cout << "purge: deleting game " << gi->id << "\n";
            old_games.erase(gi);
            delete gi->game;
            gi->my_mutex.unlock();
            delete gi;
        }
    }
}

/************************************************************************
 * main processing loop with rest endpoints
 ***********************************************************************/

int main() {
    Rest::Router router;
    const char* argv[] = {"cwordle"};
    do_options(1, (char**)argv);
    dictionary::init();

    /************************************************************************
     * Handle /start endpoint
     ***********************************************************************/
        
    router.post("/start", [&](const Rest::Request& req, Http::ResponseWriter response)
    {
        purge_games();
        request_info ri;
        ri.build(req, {}, false);
        if (ri.game) {
            ri.game->abandon();
        }
        cwordle *new_game = NULL;
        game_info *gi = NULL;
        {
            lock_guard<mutex> lock(games_mutex);
            new_game = new cwordle(the_dictionary);
            gi = new game_info(new_game);
            games[gi->id] = gi;
        }
        new_game->new_word();
        json res = { {"game_id", lexical_cast<string>(gi->id)}, {"length", word_length} };
        send_good_response(response, res);
        return Rest::Route::Result::Ok;
    });

    /************************************************************************
     * Handle /reveal endpoint
     ***********************************************************************/
    
    router.post("/reveal", [&](const Rest::Request& req, Http::ResponseWriter response)
    {
        try {
            request_info ri;
            ri.build(req, {});
            json res = { {"word", ri.game->get_current_word().str()} };
            send_good_response(response, res);
            return Rest::Route::Result::Ok;
        } catch (const RequestException &exc) {
            send_error_response(response, exc.what());
            return Rest::Route::Result::Ok;
        }
    });

    /************************************************************************
     * Handle /guess endpoint
     ***********************************************************************/

    router.post("/guess", [&](const Rest::Request& req, Http::ResponseWriter response)
    {
        try {
            request_info ri;
            ri.build(req, {"guess"});
            string guess = boost::algorithm::to_lower_copy(ri.body["guess"].get<std::string>());
            if (ri.game->is_over()) {
                throw RequestException("Game over");
            }
            // Validate the guess
            if (!ri.game->is_valid_word(guess)) {
                throw RequestException("Invalid word");
            }
            auto mr = ri.game->try_word(wordle_word(guess));
            vector<int> fb;
            for (auto ch : mr.str()) {
                fb.emplace_back(lexical_cast<int>(string(1, ch)));
            }
            // Prepare remaining words list
            std::vector<std::string> rem_words;
            const auto& rem = ri.game->remaining();
            json res = {
                {"feedback", fb},
                {"won", ri.game->is_won()},
                {"lost", ri.game->is_lost()},
                {"guesses", ri.game->get_guesses()},
                {"remaining", ri.game->remaining().size()},
                {"remaining_words", rem.to_string_vector(20)}
            };
            if (ri.game->is_lost()) {
                res["the_word"] = ri.game->get_current_word().str();
            }
            send_good_response(response, res);
            return Rest::Route::Result::Ok;
        } catch (const RequestException &exc) {
            send_error_response(response, exc.what());
            return Rest::Route::Result::Ok;
        }
    });

    /************************************************************************
     * Handle /explore endpoint
     ***********************************************************************/
        
    router.post("/explore", [&](const Rest::Request& req, Http::ResponseWriter response)
    {
        try {
            request_info ri;
            ri.build(req, {"guess", "explore_state"});
            string guess = boost::algorithm::to_lower_copy(ri.body["guess"].get<std::string>());
            std::vector<int> explore_state = ri.body["explore_state"].get<std::vector<int>>();
            // Validate the guess
            if (!ri.game->is_valid_word(guess)) {
                throw RequestException("Invalid word");
            }
            // Convert explore_state to a string for match_result
            std::string match_str;
            for (int v : explore_state) {
                match_str += char('0' + v);
            }
            wordle_word::match_result mr(match_str);
            ri.game->set_result(wordle_word(guess), mr);
            // Prepare feedback for the frontend
            std::vector<int> fb;
            for (auto ch : match_str) {
                fb.emplace_back(ch - '0');
            }
            const auto& rem = ri.game->remaining();
            json res = {
                {"feedback", std::vector<std::vector<int>>{fb}},
                {"won", ri.game->is_won()},
                {"lost", ri.game->is_lost()},
                {"guesses", ri.game->get_guesses()},
                {"remaining", ri.game->remaining().size()},
                {"remaining_words", rem.to_string_vector(20) }
            };
            send_good_response(response, res);
            return Rest::Route::Result::Ok;
        } catch (const RequestException &exc) {
            send_error_response(response, exc.what());
            return Rest::Route::Result::Ok;
        }
    });

    /************************************************************************
     * Handle /best endpoint
     ***********************************************************************/
    
    router.post("/best", [&](const Rest::Request& req, Http::ResponseWriter response)
    {
        try {
            request_info ri;
            ri.build(req, {});
            std::vector<std::string> words;
            if (!(ri.game->is_over() || ri.game->size()==0)) {            
                auto best_list = ri.game->best(5);
                for (const auto& r : best_list) {
                    if (r.key) words.push_back(string(r.key->str()));
                }
            }
            json res = { {"best", words} };
            send_good_response(response, res);
            return Rest::Route::Result::Ok;
        } catch (const RequestException &exc) {
            send_error_response(response, exc.what());
            return Rest::Route::Result::Ok;
        }
    });
        
    /************************************************************************
     * Handle /status endpoint
     ***********************************************************************/
    
    router.get("/status", [&](const Rest::Request& req, Http::ResponseWriter response)
    {
        try {
            request_info ri;
            ri.build(req, {});
            std::vector<std::string> words;
            if (!(ri.game->is_over() || ri.game->size()==0)) {            
                auto best_list = ri.game->best(5);
                for (const auto& r : best_list) {
                    if (r.key) words.push_back(string(r.key->str()));
                }
            }
            json res = {
                {"guesses", ri.game->size()},
                {"won", ri.game->is_won()},
                {"lost", ri.game->is_lost()},
                {"length", ri.game->get_current_word().size()}
            };
            if (ri.game->is_over()) {
                res["answer"] = ri.game->get_current_word().str();
            }
            if (ri.game->is_lost()) {
                res["the_word"] = ri.game->get_current_word().str();
            }
            send_good_response(response, res);
            return Rest::Route::Result::Ok;
        } catch (const RequestException &exc) {
            send_error_response(response, exc.what());
            return Rest::Route::Result::Ok;
        }
    });

    /************************************************************************
     * Catch-all for unknown endpoints (including OPTIONS)
     ***********************************************************************/
    
    router.options("/*", [&](const Rest::Request&, Http::ResponseWriter response) {
        add_cors_headers(response);
        response.send(Http::Code::No_Content);
        return Rest::Route::Result::Ok;
    });

    /************************************************************************
     * Main loop
     ***********************************************************************/
    
    while (true) {
        try {
            auto server = std::make_unique<Http::Endpoint>(Address(Ipv4::any(), Port(18080)));
            auto opts = Http::Endpoint::options().threads(4);
            server->init(opts);
            server->setHandler(router.handler());
            server->serve();
            break; // If serve() exits normally, break the loop
        } catch (const std::runtime_error& e) {
            std::cerr << "[web_server] Failed to bind/start: " << e.what()
                      << ". Retrying in 5 seconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
} 
