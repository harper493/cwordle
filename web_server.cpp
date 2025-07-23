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
#include "cwordle.h"
#include "types.h"

using namespace Pistache;
using json = nlohmann::json;
using namespace std;

unordered_map<string, cwordle*> games;
mutex games_mutex;

string generate_game_id() {
    static random_device rd;
    static mt19937 gen(rd());
    static uniform_int_distribution<> dis(0, 15);
    string id;
    for (int i = 0; i < 16; ++i) {
        id += "0123456789abcdef"[dis(gen)];
    }
    return id;
}

void add_cors_headers(Http::ResponseWriter& response) {
    response.headers().add<Http::Header::AccessControlAllowOrigin>("*");
    response.headers().add<Http::Header::AccessControlAllowMethods>("POST, GET, OPTIONS");
    response.headers().add<Http::Header::AccessControlAllowHeaders>("Content-Type");
}

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

/************************************************************************
 * validate_request - ensure that the request is valid, extract the
 * game_id and find the corresponding game, also check that all
 * required fields are present. Throw a ReqestException if there is an
 * error, otherwise return the JSON body and the game pointer.
 ***********************************************************************/
    
pair<json,cwordle*> validate_request(const Rest::Request& req, const vector<string> &required_content)
{
    json body;
    cwordle *game = NULL;
    try {
        body = json::parse(req.body());
    } catch (...) {
        throw RequestException("Invalid JSON");
    }
    if (!body.is_object() || !body.count("game_id")) {
        throw RequestException("Missing fields");
    }
    for (const string &f : required_content) {
        if (!body.count(f)) {
            throw RequestException("Missing fields");
        }
    }
    string game_id = body["game_id"].get<std::string>();
    {
        lock_guard<mutex> lock(games_mutex);
        auto it = games.find(game_id);
        if (it == games.end()) {
            throw RequestException("No such game");
        }
        game = it->second;
    }
    return make_pair(body, game);
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
        
    router.post("/start", [&](const Rest::Request&, Http::ResponseWriter response)
    {
        cwordle *game = NULL;
        string game_id = generate_game_id();
        {
            lock_guard<mutex> lock(games_mutex);
            game = new cwordle(the_dictionary);
            games[game_id] = game;
        }
        game->new_word();
        json res = { {"game_id", game_id}, {"length", word_length} };
        send_good_response(response, res);
        return Rest::Route::Result::Ok;
    });

    /************************************************************************
     * Handle /reveal endpoint
     ***********************************************************************/
    
    router.post("/reveal", [&](const Rest::Request& req, Http::ResponseWriter response)
    {
        try {
            json body;
            cwordle *game;
            tie(body, game) = validate_request(req, {});
            json res = { {"word", game->get_current_word().str()} };
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
            json body;
            cwordle *the_game;
            tie(body, the_game) = validate_request(req, {"guess"});
            string guess = boost::algorithm::to_lower_copy(body["guess"].get<std::string>());
            if (the_game->is_over()) {
                throw RequestException("Game over");
            }
            // Validate the guess
            if (!the_game->is_valid_word(guess)) {
                throw RequestException("Invalid word");
            }
            auto mr = the_game->try_word(wordle_word(guess));
            vector<int> fb;
            for (auto ch : mr.str()) {
                fb.emplace_back(lexical_cast<int>(string(1, ch)));
            }
            // Prepare remaining words list
            std::vector<std::string> rem_words;
            const auto& rem = the_game->remaining();
            json res = {
                {"feedback", fb},
                {"won", the_game->is_won()},
                {"lost", the_game->is_lost()},
                {"guesses", the_game->get_guesses()},
                {"remaining", the_game->remaining().size()},
                {"remaining_words", rem.to_string_vector(20)}
            };
            if (the_game->is_lost()) {
                res["the_word"] = the_game->get_current_word().str();
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
            json body;
            cwordle *game;
            tie(body, game) = validate_request(req, {"guess"});
            string guess = boost::algorithm::to_lower_copy(body["guess"].get<std::string>());
            std::vector<int> explore_state = body["explore_state"].get<std::vector<int>>();
            // Validate the guess
            if (!game->is_valid_word(guess)) {
                throw RequestException("Invalid word");
            }
            // Convert explore_state to a string for match_result
            std::string match_str;
            for (int v : explore_state) {
                match_str += char('0' + v);
            }
            wordle_word::match_result mr(match_str);
            game->set_result(wordle_word(guess), mr);
            // Prepare feedback for the frontend
            std::vector<int> fb;
            for (auto ch : match_str) {
                fb.emplace_back(ch - '0');
            }
            const auto& rem = game->remaining();
            json res = {
                {"feedback", std::vector<std::vector<int>>{fb}},
                {"won", game->is_won()},
                {"lost", game->is_lost()},
                {"guesses", game->get_guesses()},
                {"remaining", game->remaining().size()},
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
            json body;
            cwordle *game;
            tie(body, game) = validate_request(req, {});
            std::vector<std::string> words;
            if (!(game->is_over() || game->size()==0)) {            
                auto best_list = game->best(5);
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
            json body;
            cwordle *game;
            tie(body, game) = validate_request(req, {});
            std::vector<std::string> words;
            if (!(game->is_over() || game->size()==0)) {            
                auto best_list = game->best(5);
                for (const auto& r : best_list) {
                    if (r.key) words.push_back(string(r.key->str()));
                }
            }
            json res = {
                {"guesses", game->size()},
                {"won", game->is_won()},
                {"lost", game->is_lost()},
                {"length", game->get_current_word().size()}
            };
            if (game->is_over()) {
                res["answer"] = game->get_current_word().str();
            }
            if (game->is_lost()) {
                res["the_word"] = game->get_current_word().str();
            }
            send_good_response(response, res);
            return Rest::Route::Result::Ok;
        } catch (const RequestException &exc) {
            send_error_response(response, exc.what());
            return Rest::Route::Result::Ok;
        }
    });

    /************************************************************************
     * Catch-all for unknown endpoints
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
