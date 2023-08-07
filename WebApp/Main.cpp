#ifndef _CRT_SECURE_NO_WARNINGS
  #define _CRT_SECURE_NO_WARNINGS
#endif // !_CRT_SECURE_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN
#define SQLITE_ORM_OMITS_CODECVT

#include <cpp-httplib-0.13.3/httplib.h>
#include <mustache-4.1/mustache.hpp>
#include <sqlite3/sqlite3.h>
#include <sqlite_orm/sqlite_orm.h>

#include <assert.h>
#include <stdlib.h>
#include <memory>
#include <format>
#define ASSERT assert

template<typename T>
using Ref = std::shared_ptr<T>;

namespace Models
{
    struct User
    {
        int64_t     id = 0ll;
        std::string Name = {};
        int64_t     Age  = 0ll;
    };
}

namespace Utils
{
    static int Rand()             noexcept;
    static int Rand(int a, int b) noexcept;
}


int main()
{
    namespace sqorm = sqlite_orm;

    srand((uint32_t)time(nullptr));

    httplib::Server svr;
    
    auto storage = sqorm::make_storage("Company.sqlite",
        sqorm::make_table("Users",
            sqorm::make_column("id",   &Models::User::id, sqorm::primary_key().autoincrement()),
            sqorm::make_column("Name", &Models::User::Name),
            sqorm::make_column("Age",  &Models::User::Age)
        )
    );
    storage.sync_schema();

	// Route for the root URL "/"
    svr.Get("/", [](const httplib::Request& req, httplib::Response& res)
	{
        res.set_content("Hello, World!", "text/plain");
    }); 
    
	// Route for "/hello" with query parameters
    svr.Get("/add", [&](const httplib::Request& req, httplib::Response& res)
	{
        const std::string Name = req.get_param_value("name");
        int id = storage.insert(Models::User
        {
            .id   = -1ll,
            .Name = Name,
            .Age  = Utils::Rand(18, 40)
        });

        const std::string Message = std::format("Hello, User[{}, {}, {}]!", id, Name, -1);
        res.set_content(Message, "text/plain");
    });
    
	// Route for "/users/{id}" with a path parameter
    svr.Get("/user/:id", [&](const httplib::Request& req, httplib::Response& res)
	{
        static const char* s_Template = R"(
<html>
    <h1>Users</h1>
    <h2>Name: {{name}}</h2>
    <h2>Age : {{age}}</h2>
</html>
)";

        const std::string& id = req.path_params.at("id");
        int iId = strtol(id.c_str(), nullptr, 10);

        Models::User user = storage.get<Models::User>(iId);
        const std::string html = kainjow::mustache::mustache{ s_Template }.render(kainjow::mustache::object
        {
            { "name", user.Name                },
            { "age",  std::to_string(user.Age) },
        });

        res.set_content(html, "text/html");
    }); 
    
	// Route for "/users/all" with a path parameter
    svr.Get("/users/all", [&](const httplib::Request& req, httplib::Response& res)
	{
        namespace km = kainjow::mustache;

        static const char* s_Template = R"(
<html>
    <h1>Users</h1>
    {{#users}}
    <ol>{{name}}, {{age}}</ol> {{/users}}
</html>
)";
        
        km::data users{ km::data::type::list };
        for (const Models::User& user : storage.get_all<Models::User>())
        {
            users << km::object{
                { "name", user.Name                },
                { "age",  std::to_string(user.Age) }
            };
        }
        const std::string html = km::mustache{ s_Template }.render({ "users", users });

        res.set_content(html, "text/html");
    }); 
    
	// Route for "/admin/quit"
    svr.Get("/admin/quit", [&svr](const httplib::Request& req, httplib::Response& res)
	{
		svr.stop();
    }); 
    
	// Start the server on port 8080
    printf("Server started...\nhttp://127.0.0.1:8080/\n");
    svr.listen("localhost", 8080);
    
	return 0;
}


static int Utils::Rand() noexcept
{
    return ::rand();
}

static int Utils::Rand(int a, int b) noexcept
{
    const int iRange = b - a;
    return a + (::rand() % iRange);
}


