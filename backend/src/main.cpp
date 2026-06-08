#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

#include "core/logging/Logger.hpp"
#include "infrastructure/config/EnvironmentConfig.hpp"
#include "infrastructure/database/DatabaseManager.hpp"

auto main() -> int {
    try {
        auto config = Nyx::Infrastructure::Config::EnvironmentConfig();

        Nyx::Core::Logger::initialize(config.log_level());

        spdlog::info("Starting Nyx backend v0.1.0");
        spdlog::info(
            "Configuring server on port {} "
            "with {} threads",
            config.server_port(),
            config.server_threads()
        );

        trantor::Logger::setLogLevel(trantor::Logger::kFatal);

        Nyx::Infrastructure::Database::DatabaseManager::initialize(config);

        const auto front_url = config.frontend_url();

        drogon::app()
            .setLogLevel(trantor::Logger::kFatal)
            .addListener("0.0.0.0", config.server_port())
            .setThreadNum(config.server_threads())
            .setClientMaxBodySize(52428800)
            .registerBeginningAdvice([] {
                spdlog::info("Nyx backend is running");
            })
            .registerPreRoutingAdvice(
                [front_url](
                    const drogon::HttpRequestPtr& req,
                    drogon::AdviceCallback&& callback,
                    drogon::AdviceChainCallback&& chain
                ) {
                    if (req->method() == drogon::Options) {
                        auto resp = drogon::HttpResponse::newHttpResponse();
                        resp->addHeader(
                            "Access-Control-Allow-Origin", front_url
                        );
                        resp->addHeader(
                            "Access-Control-Allow-Methods",
                            "GET, POST, PUT, PATCH, DELETE, OPTIONS"
                        );
                        resp->addHeader(
                            "Access-Control-Allow-Headers",
                            "Content-Type, Authorization, X-CSRF-Token"
                        );
                        resp->addHeader(
                            "Access-Control-Allow-Credentials", "true"
                        );
                        resp->setStatusCode(drogon::k204NoContent);
                        callback(resp);
                        return;
                    }
                    chain();
                }
            )
            .registerPostHandlingAdvice(
                [front_url](
                    const drogon::HttpRequestPtr&,
                    const drogon::HttpResponsePtr& resp
                ) {
                    resp->addHeader(
                        "Access-Control-Allow-Origin", front_url
                    );
                    resp->addHeader(
                        "Access-Control-Allow-Credentials", "true"
                    );
                }
            )
            .run();
    } catch (const std::exception& exception) {
        spdlog::critical("Fatal error during startup: {}", exception.what());
        return 1;
    }

    return 0;
}
