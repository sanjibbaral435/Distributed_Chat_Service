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
#include <map>
#include <iterator>
#include <unistd.h>
#define ONE_SEC 9000000
#define CLIENT_PORT 5000

#include "fb.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;

using hw2::LogRequest;
using hw2::LogResponse;
using google::protobuf::Timestamp;
using google::protobuf::Duration;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
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
using hw2::WorkerMapReply;

std::map<std::string, int> portToRankMap;

std::map<std::string,int> workerMap;

std::string port;
std::string masterIp="128.194.143.213"; // To be changed
int rank;
std::string leader;
bool isLeader=false;

void RunServer(std::string);
void workerMapUpdate(std::string, int, std::string);
void workerHeartBeats();

int getRankForNewNode()
{
	int max=0;
	std::map<std::string, int>::iterator it;
	for(it=portToRankMap.begin();it!=portToRankMap.end();it++)
	{
		if(max<it->second && it->second!=INT_MAX)
			max=it->second;
	}
	return max+1;
}

std::string getAliveProcess(std::string deadProc)
{
	std::string ip = deadProc.substr(0,deadProc.length()-5);
	std::map<std::string, int>::iterator it;
	for(it=workerMap.begin();it!=workerMap.end();it++)
	{
		std::string temp=it->first;
		if (temp.find(ip) != std::string::npos) {
			std::cout<<"Getting client DB from target: "<<it->first<<std::endl;
    		return it->first;
		}
	}
	return "";
}

int getRankForNewWorkerNode()
{
	int max=0;
	std::map<std::string, int>::iterator it;
	for(it=workerMap.begin();it!=workerMap.end();it++)
	{
		if(max<it->second)
			max=it->second;
	}
	return max+1;
}

std::string getPrimaryWorkerAddr()
{
	std::map<std::string, int>::iterator it;
	int minimum=INT_MAX;
	std::string result;
	for(it = workerMap.begin();it!=workerMap.end();it++)
	{
		if(it->second<minimum)
		{
			minimum = it->second;
			result=it->first;
		}
	}
	//std::cout<<"Returning: "<<result<<std::endl;
	return result;
}

std::string getFromLogFile(std::string filename){
    std::ifstream ifs(filename);
  	std::string content( (std::istreambuf_iterator<char>(ifs) ),
                       (std::istreambuf_iterator<char>()    ) );
		return content;
}


std::string getLeader()
{
	std::string temp;
	if(portToRankMap["5254"] > portToRankMap["5255"])
		temp="5255";
	else
		temp="5254";

	if(portToRankMap[temp] > portToRankMap["5256"])
		return "5256";
	else
		return temp;
}

void listenTo(std::string port)
{
	RunServer(port);
}


void restart(std::string thisPort)
{
	std::string prevLeader=leader;
	portToRankMap[leader]=INT_MAX;
	leader = getLeader();
	//std::cout<<"thisPort="<<thisPort<<" leader="<<leader<<std::endl;
	if(!thisPort.compare(leader))
	{
		std::thread restarter([](){
			std::cout<<"this is new leader"<<std::endl;
			listenTo("5000");
		});

		std::thread restarter2([](){
			listenTo("6000");
		});

		 std::thread workerThread([thisPort](){
			  if(leader.compare(thisPort)==0)
			  {
				workerHeartBeats();
			  }
		 });

		std::string command="x-terminal-emulator -e ./master -p ";
		command.append(prevLeader);
		std::string temp = " -r ";
		command.append(temp);
		std::string temp2 = std::to_string(getRankForNewNode());
		command.append(temp2);
		system(command.c_str());
		//std::cout<<"Command is: "<<command<<std::endl;
		restarter.detach();
		restarter2.detach();
		workerThread.detach();
	}

}

void reset(std::string destPort, std::string thisPort)
{
	if(!thisPort.compare(leader))
	{//this is the new master
		//listenToClient("5000");
		std::string command="x-terminal-emulator -e ./master -p ";
		command.append(destPort);
		std::string temp = " -r ";
		command.append(temp);
		std::string temp2 = std::to_string(getRankForNewNode());
		command.append(temp2);
		system(command.c_str());
	}

}
void  bringUpRemoteProc(std::string);

class WorkerClient {
 public:
  WorkerClient(std::shared_ptr<Channel> channel)
      : stub_(MessengerServer::NewStub(channel)) {}

	void getClientDB(ClientDB* clientDb)
	{
		Request request;
		ClientContext context;

		Status status = stub_->getClientDbFromHere(&context, request, clientDb);

		//return &clientDb;
	}

	void replicateMessage(std::string username,std::string message,std::string time)
	{
 		Request request;
 		Reply reply;
 		ClientContext context;

 		request.set_username(username);
 		request.add_arguments(message);
 		request.add_arguments(time);

 		Status status = stub_->writeToFile(&context,request,&reply);

		//writeToFile
	}


	void bringUpProc(std::string address)
	{
		ClientContext context;
		MasterRequest request;
		Reply reply;

		request.set_addr(address);
		request.set_rank(getRankForNewWorkerNode());
		Status status = stub_->bringUpLocalProc(&context,request,&reply);
	}

 	void heartBeat(std::string addr)
 	{
		RankRequest request;
		RankResponse response;

		ClientContext context;
		Status status = stub_->heartBeat(&context,request,&response);
		std::thread temp([status,&addr](){
			if(!status.ok())
			{
				usleep(10000000);
				workerMap.erase(addr);
				bringUpRemoteProc(addr);

			}
		});
		temp.join();
 	}

 	void Join(std::string username,std::string username2)
 	{
 		Request request;
 		Reply reply;
 		ClientContext context;

 		request.set_username(username);
 		request.add_arguments(username2);
 		request.add_arguments("master");

 		Status status = stub_->Join(&context,request,&reply);
 	}

 	void Leave(std::string username,std::string username2)
 	{
 		Request request;
 		Reply reply;
 		ClientContext context;

 		request.set_username(username);
 		request.add_arguments(username2);
 		request.add_arguments("master");

 		Status status = stub_->Leave(&context,request,&reply);
 	}

 	void Login(std::string username)
 	{
 		Request request;
 		Reply reply;
 		ClientContext context;

 		request.set_username(username);
 		request.add_arguments("master");
 		Status status = stub_->Login(&context,request,&reply);
 	}


 private:
  std::unique_ptr<MessengerServer::Stub> stub_;
};

std::map<std::string, WorkerClient*> workerStubMap;

void  bringUpRemoteProc(std::string deadProcaddress)
{
	std::map<std::string, WorkerClient*>::iterator it;
	workerStubMap.erase(deadProcaddress);
	std::string aliveProcAddr = getAliveProcess(deadProcaddress);
	//std::cout<<"iterating over workerStubMap"<<std::endl;
	//std::cout<<"Alive Proc Addr: "<<aliveProcAddr <<std::endl;
	workerStubMap.erase(deadProcaddress);
	for(it=workerStubMap.begin();it!=workerStubMap.end();it++)
	{
		//std::cout<<"addr: "<<it->first<<std::endl;
		if(aliveProcAddr.compare(it->first)==0)
		{
			WorkerClient* workerclient = it->second;
			workerclient->bringUpProc(deadProcaddress);
		}
	}
}

class MessengerClient {
 public:
  MessengerClient(std::shared_ptr<Channel> channel)
      : stub_(MasterServer::NewStub(channel)) {}

  MessengerClient(){}

void getWorkerMap()
{
	MasterRequest request;
	WorkerMapReply reply;
	ClientContext context;

	Status status = stub_->getWorkerMapFromOtherMaster(&context,request,&reply);
	if(status.ok())
	{
		int mapSize=reply.size();
		std::cout<<"WorkerMap size: "<<reply.size()<<" values received:"<<std::endl;
		for(int i=0;i<mapSize;i++)
		{
			//std::cout<<"getWorkerMap  "<<reply.workerip(i)<<"  "<<reply.workerrank(i)<<std::endl;
			workerMap.insert(std::pair<std::string, int>(reply.workerip(i),reply.workerrank(i)));
			//std::cout<<"Reply: "<<reply.workerip(i)<<"  "<<reply.workerrank(i)<<std::endl;
		}
	}
}

 void addMapEntry(std::string addr,int rank)
 {
	//std::cout<<"addMapEntry "<<addr<<" "<<rank<<std::endl;
	MasterRequest request;
	MasterReply response;

    ClientContext context;

    request.set_addr(addr);
    request.set_rank(rank);

	Status status = stub_->setMapEntry(&context,request,&response);
 }

 void setRank(std::string port)
 {
	//std::cout<<"Setting rank of: "<<port<<std::endl;
	RankRequest request;
	RankResponse response;

    ClientContext context;
	Status status;
	request.set_port(port);
	//std::cout<<"Calling getRank"<<std::endl;
	status = stub_->getRank(&context, request, &response);
	//std::cout<<"Done Calling getRank"<<std::endl;
	if(status.ok())
		portToRankMap[port]=response.rank();
	else
		std::cout<<"Port: "<<port<<" not active"<<std::endl;
 }

 void masterToMasterHeartBeat(std::string destPort, std::string thisPort)
 {
	MasterRequest request;
	MasterReply response;

    ClientContext context;

    	Status status = stub_->heartBeat(&context, request, &response);
    	if(!status.ok())
    	{
    		      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;

    		portToRankMap[destPort]=INT_MAX;
    		if(leader.compare(destPort)){
    			std::cout<<"Reset master"<<std::endl;
    			reset(destPort,thisPort);
    		}
    		else{
    			std::cout<<"Restart master"<<std::endl;
    			restart(thisPort);

    		}
    		usleep(1000000);
    		setRank(destPort);
    	}

 }

 private:
  std::unique_ptr<MasterServer::Stub> stub_;

};

  MessengerClient *messenger1= new MessengerClient(grpc::CreateChannel(
        "128.194.143.213:5254", grpc::InsecureChannelCredentials())); ;
  MessengerClient *messenger2= new MessengerClient(grpc::CreateChannel(
        "128.194.143.213:5255", grpc::InsecureChannelCredentials())); ;
  MessengerClient *messenger3= new MessengerClient(grpc::CreateChannel(
        "128.194.143.213:5256", grpc::InsecureChannelCredentials())); ;

			WorkerClient *worker= new WorkerClient(grpc::CreateChannel(
			"128.194.143.213:7000", grpc::InsecureChannelCredentials()));

std::string getTargetAddrForDB()
{
	std::map<std::string, int>::iterator it;
	for(it=workerMap.begin();it!=workerMap.end();it++)
	{
		std::cout<<it->first<<"  "<<it->second<<std::endl;
	}
	for(it=workerMap.begin();it!=workerMap.end();it++)
	{
		std::string temp=it->first;
		if (temp.find(masterIp) != std::string::npos) {
			std::cout<<"Getting client DB from target: "<<it->first<<std::endl;
    		return it->first;
		}
	}
	//return("127.0.0.1:7001");
}

class MessengerServiceImpl final : public MasterServer::Service {

  Status getWorkerMapFromOtherMaster(ServerContext* context, const MasterRequest* request,WorkerMapReply* reply) override{
  	std::map<std::string, int>::iterator it;
  	int i=0;
  	if(!workerMap.empty()){
	  	for(it=workerMap.begin();it!=workerMap.end();it++)
	  	{
	  		reply->add_workerip(it->first);
	  		reply->add_workerrank(it->second);
	  		//std::cout<<"Added to worker: "<<it->first<<"  "<<it->second<<std::endl;
	  		i++;
	  	}
	}
  	reply->set_size(i);

  	return Status::OK;
  }

  Status initClient_DB(ServerContext* context, const Request* request,ClientDB* reply) override{
  	//std::string username=request->username();
  	std::cout<<"In master initClient_DB"<<std::endl;
  	std::string worker_addr=request->arguments(0);
    std::string targetAddr = getTargetAddrForDB();

  	std::map<std::string, WorkerClient*>::iterator it = workerStubMap.find(targetAddr);

  	if(it!=workerStubMap.end()){
  		WorkerClient* target_worker = it->second;

  		target_worker->getClientDB(reply);
  	}
	else{
		//reply = new ClientDB();
		reply->set_clientdbsize(0);
	}
	std::cout<<"Returning status from initClient_DB"<<std::endl;
	return Status::OK;
  }

  Status replicateMessage(ServerContext* context, const Request* request,Reply* reply) override{
  	//std::cout<<"In master replicateMessage"<<std::endl;
  	std::string username=request->username();
  	std::string message=request->arguments(0);
  	std::string time=request->arguments(1);
		std::string thisAddr=request->arguments(2);

  	std::map<std::string, WorkerClient*>::iterator it;

  	for(it=workerStubMap.begin(); it!=workerStubMap.end(); it++)
  	{
  		WorkerClient* worker = it->second;
  		if(thisAddr.compare(it->first))
  		{
  			worker->replicateMessage(username,message,time);
  		}
  	}

  	return Status::OK;

  }

  Status getWorker(ServerContext* context, const Request* request,Reply* reply) override{
  	std::string temp=getPrimaryWorkerAddr();
  	std::cout<<"Primary worker: "<<temp<<std::endl;
  	reply->set_msg(temp);
  	return Status::OK;
  }

  Status join(ServerContext* context, const Request* request, Reply* reply) override {
  	std::string username=request->username();
  	std::string username2=request->arguments(0);
  	std::string thisAddr=request->arguments(1);

  	std::map<std::string, WorkerClient*>::iterator it;

  	for(it=workerStubMap.begin(); it!=workerStubMap.end(); it++)
  	{
  		WorkerClient* worker = it->second;
  		if(thisAddr.compare(it->first))
  		{
  			worker->Join(username,username2);
  		}
  	}

  	return Status::OK;
  }

  Status leave(ServerContext* context, const Request* request, Reply* reply) override {
  	std::string username=request->username();
  	std::string username2=request->arguments(0);
  	std::string thisAddr=request->arguments(1);

  	std::map<std::string, WorkerClient*>::iterator it;

  	for(it=workerStubMap.begin(); it!=workerStubMap.end(); it++)
  	{
  		WorkerClient* worker = it->second;
  		if(thisAddr.compare(it->first))
  		{
  			worker->Leave(username,username2);
  		}
  	}

  	return Status::OK;
  }

  Status login(ServerContext* context, const Request* request, Reply* reply) override {
  	std::string username = request->username();
  	std::string thisAddr = request->arguments(0);

  	std::map<std::string, WorkerClient*>::iterator it;

  	for(it=workerStubMap.begin(); it!=workerStubMap.end(); it++)
  	{
  		WorkerClient* worker = it->second;
  		if(thisAddr.compare(it->first))
  		{
  			//std::cout<<"Sending login to: "<<it->first<<std::endl;
  			worker->Login(username);
  			//std::cout<<"Done Sending login to: "<<it->first<<std::endl;
  		}
  	}

  	return Status::OK;

  }

  //Sends the list of total rooms and joined rooms to the client
  Status heartBeat(ServerContext* context, const MasterRequest* request, MasterReply* reply) override {
    return Status::OK;
  }

  Status getRank(ServerContext* context, const RankRequest* request, RankResponse* reply) override {
  		reply->set_rank(rank);
  		return Status::OK;
  }

  Status setMapEntry(ServerContext* context, const MasterRequest* request, MasterReply* reply) override {

		std::string addr = request->addr();
		int rank = request->rank();
		std::cout<<"Set Map Entry -> "<<addr<<" "<<rank<<std::endl;
		std::map<std::string, int>::iterator it = workerMap.find(addr);
		if(it!=workerMap.end())
			it->second=rank;
		else
			workerMap.insert(std::pair<std::string,int>(addr,rank));

		for(it=workerMap.begin();it!=workerMap.end();it++)
			std::cout<<it->first<<" "<<it->second<<std::endl;
		return Status::OK;
  }

  Status setMapByWorker(ServerContext* context, const MasterRequest* request, MasterReply* reply) override {
  		std::cout<<"setMapByWorker "<<request->addr()<<" "<<request->rank()<<std::endl;
		std::string addr = request->addr();
		int rank = request->rank();
		bool joinedFirstTime=false;
		if(workerStubMap.empty())
			joinedFirstTime=true;
		else if(workerStubMap.find(addr)==workerStubMap.end())
			joinedFirstTime=true;
		if(port.compare("5254")){std::cout<<port<<" 5254"<<std::endl;
			messenger1->addMapEntry(addr,rank);}

		if(port.compare("5255")){std::cout<<port<<" 5255"<<std::endl;
			messenger2->addMapEntry(addr,rank);}

		if(port.compare("5256")){std::cout<<port<<" 5256"<<std::endl;
			messenger3->addMapEntry(addr,rank);}

		std::map<std::string, int>::iterator it = workerMap.find(addr);
		if(it!=workerMap.end())
			it->second=rank;
		else
			workerMap.insert(std::pair<std::string,int>(addr,rank));

			if(workerStubMap.find(addr)==workerStubMap.end())
			{
				WorkerClient *worker= new WorkerClient(grpc::CreateChannel(
				addr, grpc::InsecureChannelCredentials()));
				workerStubMap.insert(std::pair<std::string, WorkerClient*>(addr,worker));
			}
		std::thread updateMap;
		if(joinedFirstTime){
	  		updateMap = std::thread([addr,rank](){
			std::cout<<"Done Creating workerClient"<<std::endl;
			std::map<std::string, WorkerClient*>::iterator it = workerStubMap.find(addr);
			WorkerClient* worker = it->second;
			while(1)
			{
				usleep(500000);
				worker->heartBeat(addr);
			}
  			//workerMapUpdate(request->addr(), request->rank(), port);
  		});
  		updateMap.detach();
  		}
  		return Status::OK;
  }

  Status getLogData(ServerContext* context, const LogRequest* request, LogResponse* reply) override {
			std::cout<<"getLogData "<<std::endl;
			std::string aliveProcess = getTargetAddrForDB();
			std::string clientfilename = aliveProcess+request->clientname()+".txt"; //set the worker process file name in the master machine
			std::string followersfilename = aliveProcess+request->clientname()+"following.txt";
			reply->set_clientfile(getFromLogFile(clientfilename));
			reply->set_followersfile(getFromLogFile(followersfilename));
  		return Status::OK;
  }

};

void initRanks()
{
 	std::cout<<"Init.."<<std::endl;
 	usleep(ONE_SEC);
	if(portToRankMap["5254"]==INT_MAX)
	{
	 	MessengerClient *messenger = new MessengerClient(grpc::CreateChannel(
        "128.194.143.213:5254", grpc::InsecureChannelCredentials()));
		messenger->setRank("5254");
	}

	if(portToRankMap["5255"]==INT_MAX)
	{
	 	MessengerClient *messenger = new MessengerClient(grpc::CreateChannel(
        "128.194.143.213:5255", grpc::InsecureChannelCredentials()));
		messenger->setRank("5255");
	}

	if(portToRankMap["5256"]==INT_MAX)
	{
	 	MessengerClient *messenger = new MessengerClient(grpc::CreateChannel(
        "128.194.143.213:5256", grpc::InsecureChannelCredentials()));
		messenger->setRank("5256");
	}

}

void workerHeartBeats()
{
	std::cout<<"In workerHeartBeats"<<std::endl;
	std::map<std::string, int>::iterator it;
	//std::map<std::string, WorkerClient*> stubToAddrMap;
	std::thread workerThreads[7];
	int i=0;
	bool temp=false;
	if(!workerMap.empty()){
		for(it=workerMap.begin();it!=workerMap.end();it++,i++)
		{
				std::string tempStr = it->first;
				//std::cout<<"In for loop"<<std::endl;
				WorkerClient *worker= new WorkerClient(grpc::CreateChannel(
				tempStr, grpc::InsecureChannelCredentials()));

				workerStubMap[it->first] = worker;
				int rank2 = workerMap[it->first];
				workerThreads[i]=std::thread([tempStr,rank2,worker](){
					while(1)
					{
						usleep(5000000);
						worker->heartBeat(tempStr);
					}
				});
		}

		for(i=0;i<7;i++)
			workerThreads[i].join();

	}

}

void workerMapUpdate(std::string addr, int rank, std::string thisPort)
{
	//std::cout<<"WorkerMapUpdate "<<addr<<" "<<rank<<std::endl;
	//std::cout<<"In workerMapUpdate"<<std::endl;
	if(thisPort.compare("5254")){std::cout<<thisPort<<" 5254"<<std::endl;
		messenger1->addMapEntry(addr,rank);}

	if(thisPort.compare("5255")){std::cout<<thisPort<<" 5255"<<std::endl;
		messenger2->addMapEntry(addr,rank);}

	if(thisPort.compare("5256")){std::cout<<thisPort<<" 5256"<<std::endl;
		messenger3->addMapEntry(addr,rank);}

	std::map<std::string, int>::iterator it = workerMap.find(addr);
	if(it!=workerMap.end())
		it->second=rank;
	else
		workerMap.insert(std::pair<std::string,int>(addr,rank));

	//std::cout<<"Creating workerClient"<<std::endl;
	WorkerClient *worker= new WorkerClient(grpc::CreateChannel(
	addr, grpc::InsecureChannelCredentials()));
	//std::cout<<"Done Creating workerClient"<<std::endl;
	std::thread workerThread([&addr,&rank,worker](){
		while(1)
		{
			usleep(5000000);
			worker->heartBeat(addr);

		}
	});
  workerThread.detach();
}

void RunServer(std::string port_no) {
  std::string server_address = "128.194.143.213:"+port_no;
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


  int opt = 0;

  while ((opt = getopt(argc, argv, "p:r:")) != -1){
    switch(opt) {
      case 'p':
          port = optarg;break;
      case 'r':
      	  rank = std::stoi(optarg);break;
      default:
	  std::cerr << "Invalid Command Line Argument\n";
    }
  }
  std::string thisPort = port;
  std::thread runner([thisPort](){
  RunServer(thisPort);
  });

  std::string s1="5254";
  portToRankMap.insert(std::pair<std::string,int>(s1,INT_MAX));
  std::string s2="5255";
  portToRankMap.insert(std::pair<std::string,int>(s2,INT_MAX));
  std::string s3="5256";
  portToRankMap.insert(std::pair<std::string,int>(s3,INT_MAX));

//  std::cout<<"Assigning rank: "<<rank<<std::endl;
  portToRankMap[port]=rank;
//  std::cout<<"Assigned rank: "<<rank<<std::endl;

  initRanks();
  leader = getLeader();
  std::thread clientListener([thisPort](){
  if(!thisPort.compare(leader))
	  {
			  listenTo("5000");
	  }
  });

  std::thread WorkerListener([thisPort](){
  if(!thisPort.compare(leader))
	  {
			  listenTo("6000");
	  }
  });

  std::cout<<"5254: "<<portToRankMap["5254"]<<std::endl;
  std::cout<<"5255: "<<portToRankMap["5255"]<<std::endl;
  std::cout<<"5256: "<<portToRankMap["5256"]<<std::endl;

  MessengerClient *messenger1= new MessengerClient(grpc::CreateChannel(
        "128.194.143.213:5254", grpc::InsecureChannelCredentials())); ;
  MessengerClient *messenger2= new MessengerClient(grpc::CreateChannel(
        "128.194.143.213:5255", grpc::InsecureChannelCredentials())); ;
  MessengerClient *messenger3= new MessengerClient(grpc::CreateChannel(
        "128.194.143.213:5256", grpc::InsecureChannelCredentials())); ;

  if(leader.compare(thisPort)!=0){
	  if(!leader.compare("5254"))
	  {
	  	std::cout<<"Getting workerMap from 5254"<<std::endl;
	  	messenger1->getWorkerMap();
	  }
	  else if(!leader.compare("5255"))
	  {
	  	std::cout<<"Getting workerMap from 5255"<<std::endl;
	  	messenger2->getWorkerMap();
	  }
	  else{
	  	  	std::cout<<"Getting workerMap from 5256"<<std::endl;
	  	messenger3->getWorkerMap();
	  }
	  }

  std::thread heartbeat([messenger1,messenger2,messenger3,thisPort]{
  	while(1)
  	{
		usleep(5000000);
		if(port.compare("5254"))
			messenger1->masterToMasterHeartBeat("5254",port);
		if(port.compare("5255"))
			messenger2->masterToMasterHeartBeat("5255",port);
		if(port.compare("5256"))
			messenger3->masterToMasterHeartBeat("5256",port);

  	}
  });

  //std::cout<<getPrimaryWorker()<<std::endl;
  WorkerListener.join();
  clientListener.join();
  heartbeat.join();
  runner.join();
  return 0;
}
