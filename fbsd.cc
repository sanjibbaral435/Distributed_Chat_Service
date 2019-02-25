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

#include <ctime>

#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/duration.pb.h>
#include<thread>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <google/protobuf/util/time_util.h>
#include <grpc++/grpc++.h>

#include "fb.grpc.pb.h"

using namespace std;
using hw2::LogRequest;
using hw2::LogResponse;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;

using google::protobuf::Timestamp;
using google::protobuf::Duration;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using hw2::Message;
using hw2::ListReply;
using hw2::Request;
using hw2::Reply;
using hw2::MessengerServer;
using hw2::RankRequest;
using hw2::RankResponse;
using grpc::Status;
using hw2::MasterRequest;
using hw2::MasterReply;
using hw2::MasterServer;
using hw2::Message;
using hw2::ListReply;
using hw2::Request;
using hw2::Reply;
using hw2::MessengerServer;
using hw2::RankRequest;
using hw2::RankResponse;
using hw2::ClientProto;
using hw2::ClientDB;
using grpc::Status;


//Client struct that holds a user's username, followers, and users they follow
struct Client {
  std::string username;
  bool connected = true;
  int following_file_size = 0;
  std::vector<Client*> client_followers;
  std::vector<Client*> client_following;
  ServerReaderWriter<Message, Message>* stream = 0;
  bool operator==(const Client& c1) const{
    return (username == c1.username);
  }
};

//Vector that stores every client that has been created
std::vector<Client> client_db;

std::string thisAddr;

//Helper function used to find a Client object given its username
int find_user(std::string username){
  int index = 0;
  for(Client c : client_db){
    if(c.username == username)
      return index;
    index++;
  }
  return -1;
}

void internalJoin(std::string username1, std::string username2){

  	  int join_index = find_user(username2);
      Client *user1 = &client_db[find_user(username1)];
      Client *user2 = &client_db[join_index];
      user1->client_following.push_back(user2);
      user2->client_followers.push_back(user1);

}


void SaveToFile(std::string filename, std::string msg){
  FILE *file;

  //std::string filename = "Sample.txt";
  if( remove(filename.c_str()) != 0 )
    perror( "Error deleting file" );

  //cout<<"Saving chat to file: "<<filename<<endl;
  //cout<<"Msg Content: "<<msg<<endl;
    file = fopen(filename.c_str(), "w");
  if (file == NULL) {
    return;
  }
  else
  {
    fputs(msg.c_str(), file);
    fputs("\n", file);
    fclose(file);
  }
}


class WorkerClient {
 public:
  WorkerClient(std::shared_ptr<Channel> channel)
      : stub_(MasterServer::NewStub(channel)) {}

 	void initClient_DB(ClientDB* reply)
 	{
 		Request request;
 		//ClientDB reply;
 		ClientContext context;

 		//request.set_username(username);
 		request.add_arguments(thisAddr);

 		Status status = stub_->initClient_DB(&context, request, reply);

 		//return &reply;
 	}

 	void replicateMessage(std::string username, std::string message,std::string time)
 	{
 		Request request;
 		Reply reply;
 		ClientContext context;
 		std::cout<<"in replicateMessage"<<std::endl;
 		request.set_username(username);
 		request.add_arguments(message);
 		request.add_arguments(time);
 		request.add_arguments(thisAddr);

 		Status status = stub_->replicateMessage(&context, request, &reply);

 	}

 	void setRankInMaster(std::string addr, int rank)
 	{
 		MasterRequest request;
 		MasterReply response;
 		ClientContext context;

 		request.set_addr(addr);
 		request.set_rank(rank);
 		std::cout<<request.addr()<<" "<<request.rank()<<std::endl;
 		Status status=stub_->setMapByWorker(&context, request, &response);
 		//std::cout<<status.ok()<<std::endl;

	    LogRequest logrequest;
		LogResponse logresponse;
		//std::cout<<"Fetching the log file from the master machine"<<std::endl;

		//todo before iterating first copy all the inmemory values from the alive server
		for(Client c : client_db){
		  //std::cout<<"In for Loop: "<<c.username <<std::endl;
		  logrequest.set_clientname(c.username);
		  ClientContext context2;
		  Status status1=stub_->getLogData(&context2, logrequest, &logresponse);
		  SaveToFile(addr+c.username+".txt", logresponse.clientfile());
		  //iterate over all client names and ge
		  SaveToFile(addr+c.username+"following.txt", logresponse.followersfile());
		}

 	}

 	void join(std::string username,std::string username2)
 	{
 		Request request;
 		Reply reply;
 		ClientContext context;

 		request.set_username(username);
 		request.add_arguments(username2);
 		request.add_arguments(thisAddr);
 		std::cout<<"Sending join to Master"<<std::endl;
 		Status status = stub_->join(&context,request,&reply);
 	}

 	void leave(std::string username,std::string username2)
 	{
 		Request request;
 		Reply reply;
 		ClientContext context;

 		request.set_username(username);
 		request.add_arguments(username2);
 		request.add_arguments(thisAddr);
 		std::cout<<"Sending leave to Master"<<std::endl;
 		Status status = stub_->leave(&context,request,&reply);
 	}

 	void login(std::string username)
 	{
 		Request request;
 		Reply reply;
 		ClientContext context;

 		request.set_username(username);
 		request.add_arguments(thisAddr);
 		std::cout<<"Sending login to Master"<<std::endl;
 		Status status = stub_->login(&context,request,&reply);
 	}

 private:
  std::unique_ptr<MasterServer::Stub> stub_;
};

  WorkerClient* workerClient = new WorkerClient(grpc::CreateChannel(
        "128.194.143.213:6000", grpc::InsecureChannelCredentials()));

// Logic and data behind the server's behavior.
class MessengerServiceImpl final : public MessengerServer::Service {

  Status bringUpLocalProc(ServerContext* context, const MasterRequest* request, Reply* reply) override {
  	std::string address = request->addr();
  	int thisRank = request->rank();
  	std::thread newProcCreate([address, thisRank](){
	  std::string command="x-terminal-emulator -e ./fbsd -p ";
		command.append(address);
		std::string temp = " -r ";
		command.append(temp);
		std::string temp2 = std::to_string(thisRank);
		command.append(temp2);
		system(command.c_str());
  	});
  	newProcCreate.detach();
	return Status::OK;
  }

  Status getClientDbFromHere(ServerContext* context, const Request* request, ClientDB* clientdb) override {
  	if(client_db.empty())
  	{
  		clientdb->set_clientdbsize(0);
  		return Status::OK;
  	}
  	int clientCount=0;
  	std::vector<Client>::iterator it;
  	for(it = client_db.begin();it!=client_db.end();it++)
  	{
  		ClientProto* client = clientdb->add_client();
  		client->set_username(it->username);
  		client->set_connected(it->connected);
  		client->set_following_file_size(it->following_file_size);

  		std::vector<Client*>::iterator inner_it;
  		int count=0;
  		for(inner_it=it->client_following.begin();inner_it!=it->client_following.end();inner_it++)
  		{
  			client->add_following((*inner_it)->username);
  			count++;
  		}

  		client->set_followers_size(count);
  		clientCount++;
  	}
  	clientdb->set_clientdbsize(clientCount);
  	return Status::OK;
  }

  //Sends the list of total rooms and joined rooms to the client
  Status List(ServerContext* context, const Request* request, ListReply* list_reply) override {
    std::cout<<"In list on fbsd"<<std::endl;
    std::string temp;
    if(client_db.empty())
    	std::cout<<"Client_db empty"<<std::endl;
    else
    	std::cout<<"Client_db not empty"<<std::endl;
    Client user = client_db[find_user(request->username())];
    int index = 0;
    for(Client c : client_db){
      list_reply->add_all_rooms(c.username);
    }
    std::vector<Client*>::const_iterator it;
    for(it = user.client_following.begin(); it!=user.client_following.end(); it++){
      list_reply->add_joined_rooms((*it)->username);
    }
    return Status::OK;
  }

	Status heartBeat(ServerContext* context, const RankRequest* request, RankResponse* reply) override {
		return Status::OK;
	}

  //Sets user1 as following user2
  Status Join(ServerContext* context, const Request* request, Reply* reply) override {
    std::string username1 = request->username();
    std::string username2 = request->arguments(0);
    std::string caller = request->arguments(1);
    if(caller.compare("client")==0)
    	workerClient->join(username1,username2);
    int join_index = find_user(username2);
    //If you try to join a non-existent client or yourself, send failure message
    if(join_index < 0 || username1 == username2)
      reply->set_msg("Join Failed -- Invalid Username");
    else{
      Client *user1 = &client_db[find_user(username1)];
      Client *user2 = &client_db[join_index];
      //If user1 is following user2, send failure message
      if(std::find(user1->client_following.begin(), user1->client_following.end(), user2) != user1->client_following.end()){
	reply->set_msg("Join Failed -- Already Following User");
        return Status::OK;
      }
      user1->client_following.push_back(user2);
      user2->client_followers.push_back(user1);
      reply->set_msg("Join Successful");
    }
    return Status::OK;
  }


  //Sets user1 as no longer following user2
  Status Leave(ServerContext* context, const Request* request, Reply* reply) override {
    std::string username1 = request->username();
    std::string username2 = request->arguments(0);

    std::string caller = request->arguments(1);
    if(caller.compare("client")==0)
    	workerClient->leave(username1,username2);

    int leave_index = find_user(username2);
    //If you try to leave a non-existent client or yourself, send failure message
    if(leave_index < 0 || username1 == username2)
      reply->set_msg("Leave Failed -- Invalid Username");
    else{
      Client *user1 = &client_db[find_user(username1)];
      Client *user2 = &client_db[leave_index];
      //If user1 isn't following user2, send failure message
      if(std::find(user1->client_following.begin(), user1->client_following.end(), user2) == user1->client_following.end()){
	reply->set_msg("Leave Failed -- Not Following User");
        return Status::OK;
      }
      // find the user2 in user1 following and remove
      user1->client_following.erase(find(user1->client_following.begin(), user1->client_following.end(), user2));
      // find the user1 in user2 followers and remove
      user2->client_followers.erase(find(user2->client_followers.begin(), user2->client_followers.end(), user1));
      reply->set_msg("Leave Successful");
    }
    return Status::OK;
  }

  //Called when the client startd and checks whether their username is taken or not
  Status Login(ServerContext* context, const Request* request, Reply* reply) override {
    Client c;
    std::string username = request->username();
    std::string caller = request->arguments(0);

    if(caller.compare("client")==0)
	    workerClient->login(username);
    int user_index = find_user(username);
    if(user_index < 0){
      c.username = username;
      client_db.push_back(c);
      reply->set_msg("Login Successful!");
    }
    else{
      Client *user = &client_db[user_index];
      if(user->connected)
        reply->set_msg("Invalid Username");
      else{
        std::string msg = "Welcome Back " + user->username;
	reply->set_msg(msg);
        user->connected = true;
      }
    }
    return Status::OK;
  }

  Status Chat(ServerContext* context,
		ServerReaderWriter<Message, Message>* stream) override {
    Message message;
    Client *c;
    //Read messages until the client disconnects
    while(stream->Read(&message)) {
      google::protobuf::Timestamp temptime = message.timestamp();
      std::string time = google::protobuf::util::TimeUtil::ToString(temptime);
	  std::cout<<"timestamp: "<<time<<std::endl;
	  std::cout<<message.username()<<"  "<<message.msg()<<std::endl;
      workerClient->replicateMessage(message.username(),message.msg(),time);
      std::string username = message.username();
      int user_index = find_user(username);
      c = &client_db[user_index];
      //Write the current message to "username.txt"
      std::string filename = username+thisAddr+".txt";
      std::ofstream user_file(filename,std::ios::app|std::ios::out|std::ios::in);
      std::string fileinput = time+" :: "+message.username()+":"+message.msg()+"\n";
      //"Set Stream" is the default message from the client to initialize the stream
      if(message.msg() != "Set Stream" && message.msg() != "Reset Stream")
        user_file << fileinput;
      //If message = "Set Stream", print the first 20 chats from the people you follow
      else{
        if(c->stream==0)
      	  c->stream = stream;
        std::string line;
        std::vector<std::string> newest_twenty;
        std::ifstream in(username+thisAddr+"following.txt");
        int count = 0;
        //Read the last up-to-20 lines (newest 20 messages) from userfollowing.txt
        while(getline(in, line)){
          if(c->following_file_size > 20){
	           if(count < c->following_file_size-20){
               count++;
               continue;
             }
          }
          newest_twenty.push_back(line);
        }
        Message new_msg;
 	      //Send the newest messages to the client to be displayed
      	if(message.msg() != "Reset Stream"){
      		for(int i = 0; i<newest_twenty.size(); i++){
      		  new_msg.set_msg(newest_twenty[i]);
      		  stream->Write(new_msg);
      		}
        }
        continue;
      }
      //Send the message to each follower's stream
      std::vector<Client*>::const_iterator it;
      for(it = c->client_followers.begin(); it!=c->client_followers.end(); it++){
        Client *temp_client = *it;
      	if(temp_client->stream!=0 && temp_client->connected)
	      temp_client->stream->Write(message);
        //For each of the current user's followers, put the message in their following.txt file
        std::string temp_username = temp_client->username;
        std::string temp_file = temp_username + thisAddr+"following.txt";
	      std::ofstream following_file(temp_file,std::ios::app|std::ios::out|std::ios::in);
	      following_file << fileinput;
        temp_client->following_file_size++;
	      std::ofstream user_file(temp_username + thisAddr + ".txt",std::ios::app|std::ios::out|std::ios::in);
        user_file << fileinput;
      }
    }
    //If the client disconnected from Chat Mode, set connected to false
    c->connected = false;
    return Status::OK;
  }

	Status writeToFile(ServerContext* context, const Request* request, Reply* reply) override {
		  std::string username = request->username();
		  std::string msg = request->arguments(0);
		  std::string time = request->arguments(1);
		  int user_index = find_user(username);
		  Client* c = &client_db[user_index];
		  //Write the current message to "username.txt"
		  std::string filename = username+thisAddr+".txt";
		  std::ofstream user_file(filename,std::ios::app|std::ios::out|std::ios::in);
		  std::string fileinput = time+" :: "+username+":"+msg+"\n";
		  //"Set Stream" is the default message from the client to initialize the stream
		  if(msg != "Set Stream")
		    user_file << fileinput;
		  user_file.close();

	      std::vector<Client*>::const_iterator it;
		  for(it = c->client_followers.begin(); it!=c->client_followers.end(); it++){
		    Client *temp_client = *it;
		    //For each of the current user's followers, put the message in their following.txt file
		    std::string temp_username = temp_client->username;
		    std::string temp_file = temp_username + thisAddr+"following.txt";
			std::ofstream following_file(temp_file,std::ios::app|std::ios::out|std::ios::in);
			following_file << fileinput;
		    temp_client->following_file_size++;
			std::ofstream user_file(temp_username + thisAddr + ".txt",std::ios::app|std::ios::out|std::ios::in);
		    user_file << fileinput;
		  }
		return Status::OK;
	}


};

void RunServer(std::string address) {
  std::string server_address = address;
  MessengerServiceImpl service;

  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {

  std::string addr = "3055";
  int rank;
  int opt = 0;
  while ((opt = getopt(argc, argv, "p:r:")) != -1){
    switch(opt) {
      case 'p':
      	  std::cout<<"IP:PORT "<<optarg<<std::endl;
          addr = optarg;break;
      case 'r':
      	  rank = std::stoi(optarg);break;
      default:
	  std::cerr << "Invalid Command Line Argument\n";
    }
  }
  thisAddr=addr;
  std::thread writer([addr](){
  	RunServer(addr);
  });

/*  WorkerClient* workerClient = new WorkerClient(grpc::CreateChannel(
        "127.0.0.1:6000", grpc::InsecureChannelCredentials())); */
  int i,j;
  ClientDB* reply = new ClientDB;
  workerClient->initClient_DB(reply);
  std::cout<<reply->clientdbsize()<<std::endl;
   		for(i=0;i<reply->clientdbsize();i++)
 		{
 			Client c;
 			c.username=reply->client(i).username();
 			c.connected=reply->client(i).connected();
 			c.following_file_size=reply->client(i).following_file_size();
 			client_db.push_back(c);
 			std::cout<<c.username<<std::endl;
 		}

 		for(i=0;i<reply->clientdbsize();i++)
 		{
 			for(j=0;j<reply->client(i).followers_size();j++)
 			{
 				std::string joinTo=reply->client(i).following(j);
 				internalJoin(reply->client(i).username(),joinTo);
 			}
 		}

  /*
  std::cout<<"\nPrinting Client DB"<<std::endl;
  for(Client c : client_db)
  {
  	std::cout<<c.username<<std::endl;
  }

  std::cout<<addr<<" "<<rank<<std::endl;
  */
  usleep(500);
  workerClient->setRankInMaster(addr,rank);
  //std::cout<<"Done Setting rank in Master."<<rank<<std::endl;

  writer.join();
  return 0;
}
