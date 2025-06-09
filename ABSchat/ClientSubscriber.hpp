#pragma once

#include <string>

//1 - текст
//2 - id
using ClientMessage = std::string;
using IDType = unsigned short;

enum class TypeMessage : unsigned char
{
	message = 0,
	voice,
	command
};

class ClientSubscriber
{
public:
	ClientSubscriber() :
		id_{++countOfListener_}
	{}
	virtual ~ClientSubscriber() = default;

	virtual void sendMessageToYouself(const ClientMessage& message, const TypeMessage type) = 0;
	virtual void update() = 0;

	const IDType getID() const
	{
		return id_;
	}

protected:

	IDType id_{};

private:

	static IDType countOfListener_;

};

IDType ClientSubscriber::countOfListener_ = 0;