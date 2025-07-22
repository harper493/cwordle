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


struct GameState {
    string answer;
    vector<string> guesses;
    bool won = false;
    bool lost = false;
};

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

string pick_random_word() {
    // Stub: always return "CRANE"
    return "CRANE";
}

vector<int> check_guess(const string& guess, const string& answer) {
    // 0: gray, 1: yellow, 2: green
    vector<int> feedback(guess.size(), 0);
    for (size_t i = 0; i < guess.size(); ++i) {
        if (guess[i] == answer[i]) feedback[i] = 2;
        else if (answer.find(guess[i]) != string::npos) feedback[i] = 1;
    }
    return feedback;
}

void add_cors_headers(Http::ResponseWriter& response) {
    response.headers().add<Http::Header::AccessControlAllowOrigin>("*");
    response.headers().add<Http::Header::AccessControlAllowMethods>("POST, GET, OPTIONS");
    response.headers().add<Http::Header::AccessControlAllowHeaders>("Content-Type");
}

int main() {
    Rest::Router router;
    const char* argv[] = {"cwordle"};
    do_options(1, (char**)argv);
    dictionary::init();
    cwordle *the_game = NULL;
    // Register /reveal route before any catch-all or wildcard routes
    router.post("/reveal", [&](const Rest::Request& req, Http::ResponseWriter response) {
        std::cerr << "Reveal endpoint hit" << std::endl;
        json body;
        try {
            body = json::parse(req.body());
        } catch (...) {
            add_cors_headers(response);
            response.send(Http::Code::Bad_Request, R"({\"error\":\"Invalid JSON\"})", MIME(Application, Json));
            return Rest::Route::Result::Ok;
        }
        if (!body.is_object() || !body.count("game_id")) {
            add_cors_headers(response);
            response.send(Http::Code::Bad_Request, R"({\"error\":\"Missing game_id\"})", MIME(Application, Json));
            return Rest::Route::Result::Ok;
        }
        string game_id = body["game_id"].get<std::string>();
        cwordle* game = nullptr;
        {
            lock_guard<mutex> lock(games_mutex);
            auto it = games.find(game_id);
            if (it == games.end()) {
                add_cors_headers(response);
                response.send(Http::Code::Not_Found, R"({\"error\":\"No such game\"})", MIME(Application, Json));
                return Rest::Route::Result::Ok;
            }
            game = it->second;
        }
        json res = { {"word", game->get_current_word().str()} };
        add_cors_headers(response);
        response.send(Http::Code::Ok, res.dump(), MIME(Application, Json));
        return Rest::Route::Result::Ok;
    });
    router.post("/start", [&](const Rest::Request&, Http::ResponseWriter response) {
        string game_id = generate_game_id();
        {
            lock_guard<mutex> lock(games_mutex);
            the_game = new cwordle(the_dictionary);
            games[game_id] = the_game;
        }
        the_game->new_word();
        json res = { {"game_id", game_id}, {"length", word_length} };
        add_cors_headers(response);
        response.send(Http::Code::Ok, res.dump(), MIME(Application, Json));
        return Rest::Route::Result::Ok;
    });
    router.post("/guess", [&](const Rest::Request& req, Http::ResponseWriter response) {
        if (!the_game) {
            json res = { {"error", "No game in progress"} };
            add_cors_headers(response);
            response.send(Http::Code::Bad_Request, res.dump(), MIME(Application, Json));
            return Rest::Route::Result::Ok;
        }
        json body;
        try {
            body = json::parse(req.body());
        } catch (...) {
            add_cors_headers(response);
            response.send(Http::Code::Bad_Request, R"({\"error\":\"Invalid JSON\"})", MIME(Application, Json));
            return Rest::Route::Result::Ok;
        }
        if (!body.is_object() || !body.count("game_id") || !body.count("guess")) {
            add_cors_headers(response);
            response.send(Http::Code::Bad_Request, R"({\"error\":\"Invalid request\"})", MIME(Application, Json));
            return Rest::Route::Result::Ok;
        }
        string game_id = body["game_id"].get<std::string>();
        string guess = boost::algorithm::to_lower_copy(body["guess"].get<std::string>());
        {
            lock_guard<mutex> lock(games_mutex);
            auto it = games.find(game_id);
            if (it == games.end()) {
                add_cors_headers(response);
                response.send(Http::Code::Not_Found, R"({\"error\":\"No such game\"})", MIME(Application, Json));
                return Rest::Route::Result::Ok;
            }
            the_game = it->second;
        }
        if (the_game->is_over()) {
            add_cors_headers(response);
            response.send(Http::Code::Ok, R"({\"error\":\"Game over\"})", MIME(Application, Json));
            return Rest::Route::Result::Ok;
        }
        wordle_word wguess(guess); // Construct wordle_word from guess string
        auto mr = the_game->try_word(wguess);
        vector<int> fb;
        for (auto ch : mr.str()) {
            fb.emplace_back(lexical_cast<int>(string(1, ch)));
        }
        json res = {
            {"feedback", fb},
            {"won", the_game->is_won()},
            {"lost", the_game->is_lost()},
            {"guesses", the_game->get_guesses()},
            {"remaining", the_game->remaining().size()}
        };
        if (the_game->is_lost()) {
            res["the_word"] = the_game->get_current_word().str();
        }
        add_cors_headers(response);
        response.send(Http::Code::Ok, res.dump(), MIME(Application, Json));
        return Rest::Route::Result::Ok;
    });
    router.post("/best", [&](const Rest::Request& req, Http::ResponseWriter response) {
        json body;
        try {
            body = json::parse(req.body());
        } catch (...) {
            add_cors_headers(response);
            response.send(Http::Code::Bad_Request, R"({\"error\":\"Invalid JSON\"})", MIME(Application, Json));
            return Rest::Route::Result::Ok;
        }
        if (!body.is_object() || !body.count("game_id")) {
            add_cors_headers(response);
            response.send(Http::Code::Bad_Request, R"({\"error\":\"Missing game_id\"})", MIME(Application, Json));
            return Rest::Route::Result::Ok;
        }
        string game_id = body["game_id"].get<std::string>();
        cwordle* game = nullptr;
        {
            lock_guard<mutex> lock(games_mutex);
            auto it = games.find(game_id);
            if (it == games.end()) {
                add_cors_headers(response);
                response.send(Http::Code::Not_Found, R"({\"error\":\"No such game\"})", MIME(Application, Json));
                return Rest::Route::Result::Ok;
            }
            game = it->second;
        }
        std::vector<std::string> words;
        if (!(game->is_over() || game->size()==0)) {            
            auto best_list = game->best(5);
            for (const auto& r : best_list) {
                if (r.key) words.push_back(string(r.key->str()));
            }
        }
        json res = { {"best", words} };
        add_cors_headers(response);
        response.send(Http::Code::Ok, res.dump(), MIME(Application, Json));
        return Rest::Route::Result::Ok;
    });
    // Register catch-all OPTIONS route after specific routes
    router.options("/*", [&](const Rest::Request&, Http::ResponseWriter response) {
        add_cors_headers(response);
        response.send(Http::Code::No_Content);
        return Rest::Route::Result::Ok;
    });
    router.get("/status", [&](const Rest::Request& req, Http::ResponseWriter response) {
        if (!the_game) {
            json res = { {"error", "No game in progress"} };
            add_cors_headers(response);
            response.send(Http::Code::Bad_Request, res.dump(), MIME(Application, Json));
            return Rest::Route::Result::Ok;
        }
        std::string game_id = "";
        auto opt = req.query().get("game_id");
        if (opt.has_value()) game_id = *opt;
        {
            lock_guard<mutex> lock(games_mutex);
            auto it = games.find(game_id);
            if (it == games.end()) {
                add_cors_headers(response);
                response.send(Http::Code::Not_Found, R"({\"error\":\"No such game\"})", MIME(Application, Json));
                return Rest::Route::Result::Ok;
            }
            the_game = it->second;
        }
        int word_length = the_game->get_current_word().size();
        json res = {
            {"guesses", the_game->size()},
            {"won", the_game->is_won()},
            {"lost", the_game->is_lost()},
            {"length", word_length}
        };
        if (the_game->is_over()) {
            res["answer"] = the_game->get_current_word().str();
        }
        if (the_game->is_lost()) {
            res["the_word"] = the_game->get_current_word().str();
        }
        add_cors_headers(response);
        response.send(Http::Code::Ok, res.dump(), MIME(Application, Json));
        return Rest::Route::Result::Ok;
    });
    while (true) {
        try {
            auto server = std::make_unique<Http::Endpoint>(Address(Ipv4::any(), Port(18080)));
            auto opts = Http::Endpoint::options().threads(4);
            server->init(opts);
            server->setHandler(router.handler());
            server->serve();
            break; // If serve() exits normally, break the loop
        } catch (const std::runtime_error& e) {
            std::cerr << "[web_server] Failed to bind/start: " << e.what() << ". Retrying in 5 seconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
} 
