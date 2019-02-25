/*
 *
 * Copyright 2015, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

 /*
Kaustubh Kapileshwar (UIN: 926001578)
Sanjib Kumar Baral (UIN: 127001115)
 */
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <time.h>
#include <unistd.h>
#include <grpc++/grpc++.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/select.h>
#include <termios.h>
#include <stropts.h>
#include <sys/ioctl.h>
#include <grpc++/grpc++.h>


#include "fb.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using hw2::Message;
using hw2::ListReply;
using hw2::Request;
using hw2::Reply;
using hw2::MessengerServer;
using hw2::MasterServer;

//Helper function used to create a Message object given a username and message
Message MakeMessage(const std::string& username, const std::string& msg) {
  Message m;
  m.set_username(username);
  m.set_msg(msg);
  google::protobuf::Timestamp* timestamp = new google::protobuf::Timestamp();
  timestamp->set_seconds(time(NULL));
  timestamp->set_nanos(0);
  m.set_allocated_timestamp(timestamp);
  return m;
}

bool firstChat = true;
int _kbhit() {
    static const int STDIN = 0;
    static bool initialized = false;

    if (! initialized) {
        // Use termios to turn off line buffering
        termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = true;
    }

    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}

class MasterClient {
 public:
  MasterClient(std::shared_ptr<Channel> channel)
      : stub_(MasterServer::NewStub(channel)) {}

  std::string getPrimaryServer()
  {
  	std::cout<<"Getting primary server:"<<std::endl;
  	Request request;
  	Reply reply;
  	ClientContext context;

  	Status status = stub_->getWorker(&context, request, &reply);
  	if(status.ok())
  		std::cout<<"Reply received: "<<reply.msg()<<std::endl;
  		return reply.msg();
  }

  private:
  std::unique_ptr<MasterServer::Stub> stub_;
};


class MessengerClient {
 public:
  MessengerClient(std::shared_ptr<Channel> channel)
      : stub_(MessengerServer::NewStub(channel)) {}

  //Calls the List stub function and prints out room names
  std::string List(const std::string& username){
    //Data being sent to the server
    Request request;
    request.set_username(username);

    //Container for the data from the server
    ListReply list_reply;

    //Context for the client
    unsigned int client_connection_timeout = 5;
    std::chrono::system_clock::time_point deadline =
    std::chrono::system_clock::now() + std::chrono::seconds(client_connection_timeout);
    ClientContext context;
    //context.set_deadline(deadline);
    Status status = stub_->List(&context, request, &list_reply);

    //Loop through list_reply.all_rooms and list_reply.joined_rooms
    //Print out the name of each room
    if(status.ok()){
        std::cout << "All Rooms: \n";
        for(std::string s : list_reply.all_rooms()){
	  std::cout << s << std::endl;
        }
        std::cout << "Following: \n";
        for(std::string s : list_reply.joined_rooms()){
          std::cout << s << std::endl;;
        }
    }
    else{
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
             return "getNewPrimary";
    }
    return "success";
  }

  //Calls the Join stub function and makes user1 follow user2
  std::string Join(const std::string& username1, const std::string& username2){
    Request request;
    //username1 is the person joining the chatroom
    request.set_username(username1);
    //username2 is the name of the room we're joining
    request.add_arguments(username2);
    request.add_arguments("client");
    Reply reply;

    ClientContext context;

    Status status = stub_->Join(&context, request, &reply);

    if(status.ok()){
      std::cout << reply.msg() << std::endl;
    }
    else{
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      std::cout << "RPC failed\n";
      return "getNewPrimary";
    }
    return "success";
  }

  //Calls the Leave stub function and makes user1 no longer follow user2
  std::string Leave(const std::string& username1, const std::string& username2){
    Request request;

    request.set_username(username1);
    request.add_arguments(username2);
    request.add_arguments("client");

    Reply reply;

    ClientContext context;

    Status status = stub_->Leave(&context, request, &reply);

    if(status.ok()){
      std::cout << reply.msg() << std::endl;
    }
    else{
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      std::cout << "RPC failed\n";

      return "getNewPrimary";
    }
    return "success";
  }

  //Called when a client is run
  std::string Login(const std::string& username){
    Request request;

    request.set_username(username);
    request.add_arguments("client");

    Reply reply;

    ClientContext context;

    Status status = stub_->Login(&context, request, &reply);

    if(status.ok()){
      return "success";
    }
    else{
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "getNewPrimary";
    }
    return "success";
  }

  //Calls the Chat stub function which uses a bidirectional RPC to communicate
  void Chat (const std::string& username, const std::string& messages, const std::string& usec) {
    ClientContext context;
	bool ended=false;
    std::shared_ptr<ClientReaderWriter<Message, Message>> stream(
	stub_->Chat(&context));

    //Thread used to read chat messages and send them to the server
    std::thread writer([username, messages, usec, stream,&ended]() {
	  if(usec == "n") {
	    std::string input;
	    if(firstChat)
        	input = "Set Stream";
        else
        	input = "Reset Stream";
        Message m = MakeMessage(username, input);
        stream->Write(m);
        std::cout << "Enter chat messages: \n";

		while(!ended){
  //std::cout<<"Is available: " << std::cin.rdbuf()->in_avail() << std::endl;
			if(_kbhit()){
			  getline(std::cin, input);
			  std::cout<<"Sending Message: "<<input<<std::endl;
			  m = MakeMessage(username, input);
			  stream->Write(m);
			}
		}
		std::cout<<"Write stream broken"<<std::endl;
		 stream->WritesDone();
    }
	  else {
    		std::string input = "Set Stream";
        Message m = MakeMessage(username, input);
        stream->Write(m);
    		int msgs = stoi(messages);
    		int u = stoi(usec);
    		time_t start, end;
        std::cout << "Enter chat messages: \n";
    		time(&start);
        for(int i=0; i<msgs; i++) {
          input = "hello" + std::to_string(i);
    		  m = MakeMessage(username, input);
          stream->Write(m);
		  std::cout << input << '\n';
		  usleep(u);
        }
		time(&end);
		std::cout << "Elapsed time: " << (double)difftime(end,start) << std::endl;
        stream->WritesDone();
	  }
	});

    //Thread used to display chat messages from users that this client follows
    std::thread reader([username, stream,&ended,&writer]() {
       Message m;
       while(stream->Read(&m)){
	     	std::cout << m.username() << " -- " << m.msg() << std::endl;
       }
       ended=true;
       std::cout<<"Stream broken"<<std::endl;
    });

    //Wait for the threads to finish
    writer.join();
    reader.join();

  }

 private:
  std::unique_ptr<MessengerServer::Stub> stub_;
};

MasterClient *messenger2 = new MasterClient(grpc::CreateChannel(
        "128.194.143.213:5000", grpc::InsecureChannelCredentials()));

MessengerClient* messenger;
std::map<std::string, MessengerClient*> workerToConnectionMap;

void getNewWorker()
{
	MessengerClient* tempMessenger;
	std::string login_info=messenger2->getPrimaryServer();
	std::cout<<"New primary server is: "<<login_info<<std::endl;
/*	if(workerToConnectionMap.empty())
	{
		std::cout<<"workerConnectionMap is empty"<<std::endl;
		tempMessenger = new MessengerClient(grpc::CreateChannel(
        login_info, grpc::InsecureChannelCredentials()));
        workerToConnectionMap.insert(std::pair<std::string, MessengerClient*>(login_info,tempMessenger));
        messenger = tempMessenger;
    }
    else
    {
    	std::map<std::string, MessengerClient*>::iterator it;
    	it=workerToConnectionMap.find(login_info);
    	if(it!=workerToConnectionMap.end())
    	{
    		messenger=it->second;
    		std::cout<<"Connection exists in workerConnectionMap"<<std::endl;
    	}
    	else
    	{
    		std::cout<<"Connection Does not exist in workerConnectionMap"<<std::endl;
			tempMessenger = new MessengerClient(grpc::CreateChannel(
		    login_info, grpc::InsecureChannelCredentials()));
		    workerToConnectionMap.insert(std::pair<std::string, MessengerClient*>(login_info,tempMessenger));
		    messenger = tempMessenger;
    	}
    }*/
    messenger = new MessengerClient(grpc::CreateChannel(
        login_info, grpc::InsecureChannelCredentials()));
}

//Parses user input while the client is in Command Mode
//Returns 0 if an invalid command was entered
//Returns 1 when the user has switched to Chat Mode


int parse_input(MessengerClient* messenger, std::string username, std::string input){
  //Splits the input on spaces, since it is of the form: COMMAND <TARGET_USER>
  std::size_t index = input.find_first_of(" ");
  if(index!=std::string::npos){
    std::string cmd = input.substr(0, index);
    if(input.length()==index+1){
      std::cout << "Invalid Input -- No Arguments Given\n";
      return 0;
    }
    std::string argument = input.substr(index+1, (input.length()-index));
    if(cmd == "JOIN"){
		std::string response = messenger->Join(username, argument);
		std::cout<<"JOIN response: "<<response<<std::endl;
		while(response != "success")
		{
			getNewWorker();
			usleep(1000);
			response = messenger->Join(username, argument);
		}

    }
    else if(cmd == "LEAVE"){
		std::string response = messenger->Leave(username, argument);
		std::cout<<"LEAVE response: "<<response<<std::endl;
		while(response != "success")
		{
			getNewWorker();
			usleep(1000);
			response = messenger->Leave(username, argument);
		}

     }
    else{
      std::cout << "Invalid Command\n";
      return 0;
    }
  }
  else{
    if(input == "LIST"){
		std::string response = messenger->List(username);
		std::cout<<"LIST response: "<<response<<std::endl;
		while(response != "success")
		{
			getNewWorker();
			usleep(1000);
			if(messenger==NULL)
				std::cout<<"Messenger is null"<<std::endl;
			else
				std::cout<<"Messenger not null"<<std::endl;
			response = messenger->List(username);
		}
    }
    else if(input == "CHAT"){
      //Switch to chat mode
      return 1;
    }
    else{
      std::cout << "Invalid Command\n";
      return 0;
    }
  }
  return 0;
}

int main(int argc, char** argv) {
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint (in this case,
  // localhost at port 50051). We indicate that the channel isn't authenticated
  // (use of InsecureChannelCredentials()).

  std::string hostname = "localhost";
  std::string username = "default";
  std::string port = "3055";
  std::string messages = "10000";
  std::string usec = "n";
  int opt = 0;
  while ((opt = getopt(argc, argv, "h:u:p:m:t:")) != -1){
    switch(opt) {
      case 'h':
	  hostname = optarg;break;
      case 'u':
          username = optarg;break;
      case 'p':
          port = optarg;break;
	  case 'm':
		  messages = optarg;break;
	  case 't':
		  usec = optarg;break;
      default:
	  std::cerr << "Invalid Command Line Argument\n";
    }
  }

  std::string login_info = hostname + ":" + port;

  //Create the messenger client with the login info
  login_info="128.194.143.213:5000";
/*  MasterClient *messenger2 = new MasterClient(grpc::CreateChannel(
        login_info, grpc::InsecureChannelCredentials())); */
  //Call the login stub function
  		std::string response;
  		if(messenger==NULL)
  			std::cout<<"messenger null"<<std::endl;
  		else
  			std::cout<<"Messenger not null"<<std::endl;
  		getNewWorker();
  		response = messenger->Login(username);

  		while(response != "success"){
  			//getNewWorker();
			usleep(100000);
  			response = messenger->Login(username);
  		}
		//If the username already exists, exit the client
		if(response == "Invalid Username"){
			std::cout << "Invalid Username -- please log in with a different username \n";
			return 0;
		}
		else{
			std::cout << response << std::endl;

    std::cout << "Enter commands: \n";
    std::string input;
    //While loop that parses all of the command input
    while(getline(std::cin, input)){
      //If we have switched to chat mode, parse_input returns 1
      if(parse_input(messenger, username, input) == 1)
	break;
    }
    //Once chat mode is enabled, call Chat stub function and read input
    while(1){
    	getNewWorker();
    	messenger->Chat(username, messages, usec);
    	firstChat=false;
    	usleep(11000000);
    }
  }
  return 0;
}
