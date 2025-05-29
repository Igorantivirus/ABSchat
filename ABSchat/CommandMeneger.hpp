#pragma once

#include <nlohmann/json.hpp>

#include "HTTPClient.hpp"
#include "Service.hpp"

using RowJson = std::map<std::string, std::string>;

using ResultType = std::pair<bool, std::string>;

class CommandMeneger
{
public:

    std::string startCommand(const std::string& commandKey, const std::string& tgId) const
    {
        RowJson param =
        {
            {"code_server", Service::config.API_KEY},
            {"act", "reg"},
            {"code", commandKey},
            {"id_tg", tgId},
        };
        std::string responce = http_.getRequastParam(Service::config.URL_SERVER_BOT, param);

        nlohmann::json json = nlohmann::json::parse(responce);

        return json["id"].empty() ? "" : json["id"].dump();
    }

    std::string renewCommand(const std::string& webId) const
    {
        nlohmann::json json = sendAct("renew", webId);
        if (json.empty())
            Service::log.log("Renew command return empty json");
        return json["code"].empty() ? "false" : json["code"].dump();
    }
    std::string onlineCommand(const std::string& webId) const
    {
        nlohmann::json json = sendAct("online", webId);
        if (json.empty())
            Service::log.log("Online command return empty json");
        return json["code"] != "1" ? (json["online"].dump() + '\n' + json["list"].dump()) : "";
    }
    std::string breakCommand(const std::string& webId) const
    {
        nlohmann::json json = sendAct("breakOut", webId);
        if (json.empty())
            Service::log.log("Break command return empty json");
        return json["code"].empty() ? "false" : json["code"].dump();
    }

private:

	HttpClient http_;

private:

	nlohmann::json sendAct(const std::string& act, const std::string& webId) const
	{
        RowJson param =
        {
            {"code_server", Service::config.API_KEY},
            {"act", act},
            {"id", webId}
        };
        std::string responce = http_.getRequastParam(Service::config.URL_SERVER_BOT, param);

        return nlohmann::json::parse(responce);
	}

};