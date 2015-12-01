// Copyright 2014 MongoDB Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "catch.hpp"
#include "helpers.hpp"

#include <mongocxx/private/libmongoc.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/database.hpp>

using namespace mongocxx;

namespace {

// Clean up the given collection if it exists
void clear_collection(const database& database, bsoncxx::stdx::string_view collection_name) {
    if (database.has_collection(collection_name)) {
        database[collection_name].drop();
    }
}

}  // namespace

TEST_CASE("A default constructed database is false-ish", "[database]") {
    database d;
    REQUIRE(!d);
}

TEST_CASE("database copy", "[database]") {
    client mongodb_client{uri{}};

    std::string dbname{"foo"};
    std::string dbname2{"bar"};
    database db = mongodb_client[dbname];

    database db2{db};
    database db3 = mongodb_client[dbname2];
    db3 = db;

    REQUIRE(db2.name() == stdx::string_view{dbname});
    REQUIRE(db3.name() == stdx::string_view{dbname});
}

TEST_CASE("A database", "[database]") {
    stdx::string_view database_name{"database"};
    MOCK_CLIENT
    MOCK_DATABASE
    client mongo_client{uri{}};

    SECTION("is created by a client") {
        bool called = false;
        get_database->interpose([&](mongoc_client_t* client, const char* d_name) {
            called = true;
            REQUIRE(database_name == stdx::string_view{d_name});
            return nullptr;
        });

        database obtained_database = mongo_client[database_name];
        REQUIRE(obtained_database);
        REQUIRE(called);
        REQUIRE(obtained_database.name() == database_name);
    }

    SECTION("cleans up its underlying mongoc database on destruction") {
        bool destroy_called = false;

        database_destroy->interpose([&](mongoc_database_t* client) { destroy_called = true; });

        {
            database database = mongo_client["database"];
            REQUIRE(!destroy_called);
        }

        REQUIRE(destroy_called);
    }

    SECTION("is dropped") {
        bool drop_called = false;

        database_drop->interpose([&](mongoc_database_t* database, bson_error_t* error) { 
            drop_called = true; 
            return true;
        });

        database database = mongo_client["database"];
        REQUIRE(!drop_called);
        database.drop();
        REQUIRE(drop_called);
    }

    SECTION("throws an exception when dropping causes an error") {
        database_drop->interpose([&](mongoc_database_t* database, bson_error_t* error) { return false; });

        database database = mongo_client["database"];
        REQUIRE_THROWS(database.drop());
    }

    SECTION("supports move operations") {
        bool destroy_called = false;
        database_destroy->interpose([&](mongoc_database_t* client) { destroy_called = true; });

        {
            client mongo_client{uri{}};
            database a = mongo_client[database_name];

            database b{std::move(a)};
            REQUIRE(!destroy_called);

            database c = std::move(b);
            REQUIRE(!destroy_called);
        }
        REQUIRE(destroy_called);
    }

    SECTION("has a read preferences which may be set and obtained") {
        bool destroy_called = false;
        database_destroy->interpose([&](mongoc_database_t* client) { destroy_called = true; });

        database mongo_database(mongo_client["database"]);
        read_preference preference{read_preference::read_mode::k_secondary_preferred};

        auto deleter = [](mongoc_read_prefs_t* var) { mongoc_read_prefs_destroy(var); };
        std::unique_ptr<mongoc_read_prefs_t, decltype(deleter)> saved_preference(nullptr, deleter);

        bool called = false;
        database_set_preference->interpose([&](mongoc_database_t* db,
                                               const mongoc_read_prefs_t* read_prefs) {
            called = true;
            saved_preference.reset(mongoc_read_prefs_copy(read_prefs));
            REQUIRE(
                mongoc_read_prefs_get_mode(read_prefs) ==
                static_cast<mongoc_read_mode_t>(read_preference::read_mode::k_secondary_preferred));
        });

        database_get_preference->interpose([&](const mongoc_database_t* client) {
            return saved_preference.get();
        }).forever();

        mongo_database.read_preference(std::move(preference));
        REQUIRE(called);

        REQUIRE(read_preference::read_mode::k_secondary_preferred ==
                mongo_database.read_preference().mode());
    }

    SECTION("has a write concern which may be set and obtained") {
        bool destroy_called = false;
        database_destroy->interpose([&](mongoc_database_t* client) { destroy_called = true; });

        database mongo_database(mongo_client[database_name]);
        write_concern concern;
        concern.majority(std::chrono::milliseconds(100));

        mongoc_write_concern_t* underlying_wc;

        bool set_called = false;
        database_set_concern->interpose(
            [&](mongoc_database_t* client, const mongoc_write_concern_t* concern) {
                set_called = true;
                underlying_wc = mongoc_write_concern_copy(concern);
            });

        bool get_called = false;
        database_get_concern->interpose([&](const mongoc_database_t* client) {
            get_called = true;
            return underlying_wc;
        });

        mongo_database.write_concern(concern);
        REQUIRE(set_called);

        MOCK_CONCERN
        bool copy_called = false;
        concern_copy->interpose([&](const mongoc_write_concern_t* concern) {
            copy_called = true;
            return mongoc_write_concern_copy(underlying_wc);
        });

        REQUIRE(concern.majority() == mongo_database.write_concern().majority());

        REQUIRE(get_called);
        REQUIRE(copy_called);
    }

    SECTION("may create a collection") {
        MOCK_COLLECTION
        // dummy_collection is the name the mocked collection_get_name returns
        stdx::string_view collection_name{"dummy_collection"};
        database database = mongo_client[database_name];
        collection obtained_collection = database[collection_name];
        REQUIRE(obtained_collection.name() == collection_name);
    }
}

TEST_CASE("Database integration tests", "[database]") {
    client mongo_client{uri{}};
    bsoncxx::stdx::string_view database_name{"database"};
    database database = mongo_client[database_name];
    bsoncxx::stdx::string_view collection_name{"collection"};

    SECTION("A database may create a collection via create_collection") {
        SECTION("without any options") {
            clear_collection(database, collection_name);

            collection obtained_collection = database.create_collection(collection_name);
            REQUIRE(obtained_collection.name() == collection_name);
        }

        SECTION("with options") {
            clear_collection(database, collection_name);

            options::create_collection opts;
            opts.capped(true);
            opts.size(256);
            opts.max(100);
            opts.no_padding(false);

            collection obtained_collection = database.create_collection(collection_name, opts);
            REQUIRE(obtained_collection.name() == collection_name);
        }

        SECTION("but raises exception when collection already exists") {
            clear_collection(database, collection_name);

            database.create_collection(collection_name);

            REQUIRE_THROWS(database.create_collection(collection_name));
        }

        clear_collection(database, collection_name);
    }

    SECTION("A database may be dropped") {
        clear_collection(database, collection_name);

        database.create_collection(collection_name);
        REQUIRE(database.has_collection(collection_name));
        database.drop();
        REQUIRE(!database.has_collection(collection_name));
    }
}