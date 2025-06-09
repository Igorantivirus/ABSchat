#pragma once

#include <vector>

#include "ClientSubscriber.hpp"
#include "Service.hpp"

class ClientBrocker
{
public:
	ClientBrocker()
	{
		Service::init("configWS.json");
	}

	void addSub(ClientSubscriber* sub)
	{
		if(sub != nullptr)
			subers_.push_back(sub);
	}

	void sendMessage(const int64_t senderId, const ClientMessage& message, const TypeMessage type)
	{
		for (auto& i : subers_)
			if (i->getID() != senderId)
				i->sendMessageToYouself(message, type);
	}

	void start()
	{
		while (true)
		{
			for (auto& i : subers_)
				i->update();
		}
	}

private:

	std::vector<ClientSubscriber*> subers_;

};