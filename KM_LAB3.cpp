// KM_LAB3.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define CROW_MAIN
#include "D:\С++ works\KM_LAB3\include/crow.h"
#include <vector>
#include <string>
#include <iostream>
#include <ctime>
#include <mutex>

// Data structure for news
struct News {
    int id;
    std::string headline;
    std::string content;
    std::string date_published;
    std::string author;
};

// Global variables
std::vector<News> news_db;
int current_id = 1;
std::mutex db_mutex;

// Function to get current date in YYYY-MM-DD format
std::string get_current_date() {
    time_t t = time(0);
    struct tm* now = localtime(&t);
    char buf[11]; // Enough for "YYYY-MM-DD\0"
    strftime(buf, sizeof(buf), "%Y-%m-%d", now);
    return std::string(buf);
}

// API initialization
void init_api(crow::SimpleApp& app) {
    // Get all news
    CROW_ROUTE(app, "/news")
        .methods("GET"_method)([]() {
        std::lock_guard<std::mutex> lock(db_mutex); // Protect `news_db`
        crow::json::wvalue response;
        response["news"] = std::vector<crow::json::wvalue>(); // Initialize as a JSON array
        for (const auto& n : news_db) {
            crow::json::wvalue news_item;
            news_item["id"] = n.id;
            news_item["headline"] = n.headline;
            news_item["content"] = n.content;
            news_item["date_published"] = n.date_published;
            news_item["author"] = n.author;
            response["news"] = (std::move(news_item)); // Push into JSON array
        }
        return crow::response(response); // Return the JSON response
            });

    // Get news by id
    CROW_ROUTE(app, "/news/<int>")
        .methods("GET"_method)([](int id) {
        std::lock_guard<std::mutex> lock(db_mutex);
        for (const auto& n : news_db) {
            if (n.id == id) {
                crow::json::wvalue response;
                response["id"] = n.id;
                response["headline"] = n.headline;
                response["content"] = n.content;
                response["date_published"] = n.date_published;
                response["author"] = n.author;
                return crow::response(response);
            }
        }
        return crow::response(404, "News not found");
            });

    // Add new news
    CROW_ROUTE(app, "/news")
        .methods("POST"_method)([](const crow::json::rvalue& req) {
        if (!req.has("headline") || !req.has("content") || !req.has("author")) {
            return crow::response(400, "Missing required fields");
        }

        News new_news;
        new_news.id = current_id++;
        new_news.headline = req["headline"].s();
        new_news.content = req["content"].s();
        new_news.date_published = get_current_date();
        new_news.author = req["author"].s();

        {
            std::lock_guard<std::mutex> lock(db_mutex);
            news_db.push_back(new_news);
        }

        crow::json::wvalue response;
        response["id"] = new_news.id;
        return crow::response(201, response);
            });

    // Update news by id
    CROW_ROUTE(app, "/news/<int>")
        .methods("PUT"_method)([](int id, const crow::json::rvalue& req) {
        if (!req.has("headline") || !req.has("content") || !req.has("author")) {
            return crow::response(400, "Missing required fields");
        }

        std::lock_guard<std::mutex> lock(db_mutex);
        for (auto& n : news_db) {
            if (n.id == id) {
                n.headline = req["headline"].s();
                n.content = req["content"].s();
                n.author = req["author"].s();
                return crow::response(200);
            }
        }
        return crow::response(404, "News not found");
            });

    // Delete news by id
    CROW_ROUTE(app, "/news/<int>")
        .methods("DELETE"_method)([](int id) {
        std::lock_guard<std::mutex> lock(db_mutex);
        auto it = std::remove_if(news_db.begin(), news_db.end(),
            [id](const News& n) { return n.id == id; });
        if (it != news_db.end()) {
            news_db.erase(it, news_db.end());
            return crow::response(200);
        }
        return crow::response(404, "News not found");
            });
}

int main() {
    crow::SimpleApp app;

    init_api(app);

    std::cout << "Server is running on http://localhost:18080" << std::endl;
    app.port(18080).multithreaded().run();

    return 0;
}
