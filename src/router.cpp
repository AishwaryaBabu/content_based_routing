#include<iostream>
#include"common.h"
#include"newport.h"
#include<vector>
#include<list>

#define rtTimeToExpire 100
#define prtTimeToExpire 100
#define timerWrap 6000
#define sleepDelay 1
using namespace std;

//Routing table - content id + receiving port(interface) + #hops + time to expire
//Connections table - 2D vector mapping destination address to sending port+"receiving port"(interface)//For broadcast
//Pending request table - Requested id + host id + receiving port(interface) + time to expire
static int globalTimer=0;
static vector<vector<int> >connectionsList;
static vector<vector<int> >routingTable;
static vector<vector<int> >pendingRequestTable;


struct cShared{
    short receivingPortNum;
    LossyReceivingPort* fwdRecvPort;
    mySendingPort* fwdSendPort;
    //  int max;
};

void Display2DVector(vector<vector<int> > nameOfVector)
{
    for(unsigned int i = 0; i < nameOfVector.size(); i++)
    {
        for(unsigned int j = 0; j < nameOfVector[0].size(); j++)
            cout<<nameOfVector[i][j]<<" ";  
        cout<<endl;
    }  
}

void CreateConnectionsList(int argc, char* argv[])
{
    //Implement formula and build table using config details
    int numberofPorts = argc-2;
    string source(argv[1]);

    //Sets source details to self
    int sourceID;
        sourceID=atoi(source.substr(1).c_str())-1;
    for(int i=0;i<numberofPorts;i++)
    {
        vector<int> ports;
        string destination(argv[i+2]);
        int destinationID;
        if(destination.at(0)=='r'){
            destinationID=atoi(destination.substr(1).c_str())-1;
        }
        else if(destination.at(0)=='h'){
            destinationID=atoi(destination.substr(1).c_str())+63;
        }
        int x,y,zSourceRx,zDestinationRx, zSourceTx;
        if(sourceID<destinationID){
            x=sourceID;
            y=destinationID;
            zSourceRx=0;
            zDestinationRx=1;
            zSourceTx=2;
//            zDestinationTx=3;
            
        }
        else{
            y=sourceID;
            x=destinationID;
            zSourceTx=0;
//            zDestinationTx=1;
            zSourceRx=2;
            zDestinationRx=3;
        }

        int destinationPortNum = x*(512)+y*(4)+zDestinationRx+8000;
        int receivingPortNum = x*(512)+y*(4)+zSourceRx+8000;
        int sendingPortNum = x*(512)+y*(4)+zSourceTx+8000;
        ports.push_back(receivingPortNum);
        ports.push_back(destinationPortNum);
        ports.push_back(sendingPortNum);
        connectionsList.push_back(ports);
    }
}

int SearchConnectionsTable(int receivingPortNum)
{
    for(unsigned int i = 0; i < connectionsList.size(); i++)
    {
        if(receivingPortNum == connectionsList[i][0])
            return connectionsList[i][1];
    }
    return -1;
}

void AddRoutingTableEntry(int contentId, int recPortNum, int numHops)
{
    int dstPortNum = SearchConnectionsTable(recPortNum);
    //Using config information build routing table
    if(routingTable.size()==0)
    {
        vector<int> routingRow;
        routingRow.push_back(contentId);
        routingRow.push_back(dstPortNum);
        routingRow.push_back(numHops);
        routingRow.push_back(rtTimeToExpire);      //Time to expire
        routingTable.push_back(routingRow);
    }
    else
    {
        bool contentExists=false;
        for(unsigned int i = 0; i < routingTable.size(); i++)
        {
            if(routingTable[i][0]==contentId)
            {
                contentExists=true;
                if(numHops < routingTable[i][2])
                {
                    routingTable[i][1]=dstPortNum;
                    routingTable[i][2]=numHops;
                    routingTable[i][3]=rtTimeToExpire;      //Time to expire;
                }
                break;
            }
        }
        if(!contentExists)
        {
            vector<int> routingRow;
            routingRow.push_back(contentId);
            routingRow.push_back(dstPortNum);
            routingRow.push_back(numHops);
            routingRow.push_back(rtTimeToExpire);//Time to expire;
            routingTable.push_back(routingRow);
        }
    }

}

void DeleteRoutingTableEntry(int contentId)
{
    for(unsigned int i = 0; i < routingTable.size(); i++)
    {
        if(routingTable[i][0]==contentId)
        {
            routingTable.erase(routingTable.begin()+i);
            break;
        }
    }
}

int SearchRoutingTable(int requestedContentId)
{
    for(unsigned int i = 0; i < routingTable.size(); i++)
    {
        if(requestedContentId == routingTable[i][0])
        {
            return routingTable[i][1]; //Returns receiving port number   
        }
    }
    return -1;

}
void UpdatePendingRequestTable(int requestedContentId, int requestingHostId, int receivingPort)
{
    vector<int> pendingRequestRow;
    pendingRequestRow.push_back(requestedContentId);
    pendingRequestRow.push_back(requestingHostId);

    //Maintain destination Port numbers at PRT  
    int destPort = SearchConnectionsTable(receivingPort);
    pendingRequestRow.push_back(destPort);
    pendingRequestRow.push_back(globalTimer+prtTimeToExpire); // Time to expire

    pendingRequestTable.push_back(pendingRequestRow);
}

void UpdatePendingRequestTableTTL()
{
	for(unsigned int i=0; i<pendingRequestTable.size(); i++)
	{
		pendingRequestTable[i][2] = pendingRequestTable[i][2] - timerWrap;
	}
}

void DeletePendingRequestTableEntry(int requestedContentId, int requestingHostId)
{
	for(unsigned int i=0; i<pendingRequestTable.size(); i++)
	{
		if((pendingRequestTable[i][0] == requestedContentId)&&(pendingRequestTable[i][1] == requestingHostId))
		{
			pendingRequestTable.erase(pendingRequestTable.begin()+i);
			break;
		}
	}

}

void DeletePendingRequestTableExpired(int currentTime)
{
	for(unsigned int i=1; i<=pendingRequestTable.size(); i++)
	{
		if(pendingRequestTable[i-1][2] == currentTime)
		{
			pendingRequestTable.erase(pendingRequestTable.begin()+i);
			i--; // to ensure the deletion of 0th entry
		}
	}

}

int SearchPendingRequestTable(int contentId, int hostId)
{
    for(unsigned int i = 0; i < pendingRequestTable.size(); i++)
    {
        vector<int> currentEntry = pendingRequestTable[i];
        if(contentId==currentEntry[0] && hostId==currentEntry[1])
            return currentEntry[2];
    }
    return -1;
}

void PendingRequestTimer()
{
	while(1)
	{
		sleep(sleepDelay);
		if(globalTimer>=timerWrap)
		{
			globalTimer=globalTimer-timerWrap;
			UpdatePendingRequestTableTTL();
		}
		globalTimer++;
		DeletePendingRequestTableExpired(globalTimer);
	}
}

void* NodeRecProc(void* arg)
{
    cout<<"Created thread"<<endl;
    struct cShared *sh = (struct cShared *)arg;
    Packet* recvPacket;

    //Packet received : needs to be checked for appropriate forwarding and editing of table
    while(true)
    {
        recvPacket = sh->fwdRecvPort->receivePacket();
        if(recvPacket != NULL)
        {
//            sh->fwdSendPort->sendPacket(recvPacket);
            //Request Packet
            if(recvPacket->accessHeader()->getOctet(0) == '0')
            {
                int requestedContentId = int(recvPacket->accessHeader()->getOctet(1));
                int requestingHostId = int(recvPacket->accessHeader()->getOctet(2)); 
                int receivingPortNum = sh->receivingPortNum;

                //Look up routing table based on content id and Forward to appropriate next hop
                int nextHopPortNum = SearchRoutingTable(requestedContentId);
                //                int nextHopDestPortNum = SearchConnectionsTable(nextHopRecvPortNum);
                Address* dstAddr = new Address("localhost", nextHopPortNum);
                sh->fwdSendPort->setRemoteAddress(dstAddr);
                sh->fwdSendPort->sendPacket(recvPacket);
                delete(dstAddr);

                //Make entry in pending request table                 
                UpdatePendingRequestTable(requestedContentId, requestingHostId, receivingPortNum); //PRT converts receiving port to dest port
            }
            //Response Packet
            else if(recvPacket->accessHeader()->getOctet(0) == '1')
            {
                int requestedContentId = int(recvPacket->accessHeader()->getOctet(1));
                int requestingHostId = int(recvPacket->accessHeader()->getOctet(2)); 
                int dstPortNum = SearchPendingRequestTable(requestedContentId, requestingHostId);
                //Search for appropriate destination address in connections table
                if(dstPortNum > 0)
                {
                    Address* dstAddr = new Address("localhost", dstPortNum);
                    sh->fwdSendPort->setRemoteAddress(dstAddr);
                    sh->fwdSendPort->sendPacket(recvPacket);
                    delete(dstAddr);

                //Delete from pending request table entry
                DeletePendingRequestTableEntry(requestedContentId, requestingHostId);
                }

            }
            //Announcement
            else if(recvPacket->accessHeader()->getOctet(0) == '2')
            {
                int receivedContentId = int(recvPacket->accessHeader()->getOctet(1));
                int receivingPortNum = sh->receivingPortNum;
                int numHops = int(recvPacket->accessHeader()->getOctet(2));

                AddRoutingTableEntry(receivedContentId, receivingPortNum, numHops); //Takes care of updating timer and comparing num Hops
                //Routing table converts receiving port num to appropriate dest port num

                //Increment number of hops
                numHops++;
                recvPacket->accessHeader()->setOctet(char(numHops), 2);

                //Forward to all other ports
                int destPortNumToSkip = SearchConnectionsTable(receivingPortNum);
                //BroadcastPacket(destPortNumToSkip, recvPacket);
                for(unsigned int i = 0; i < routingTable.size(); i++)
                {
                    int destPort = routingTable[i][1];
                    if(destPortNumToSkip != destPort)
                    {
                        Address* dstAddr = new Address("localhost", destPort);
                        sh->fwdSendPort->setRemoteAddress(dstAddr);
                        sh->fwdSendPort->sendPacket(recvPacket);
                        delete(dstAddr);
                    }
                }
            }
        }
    }
    return NULL;
}

void StartNodeThread(pthread_t* thread, vector<int>& ports)
{

    //setup ports numbers
    Address* recvAddr;  //receive from port corresponding to node2 
    Address* sendAddr; // sending port corresponding to node1
//    Address* dstAddr;  //address of node1 //NEEDS TO GO
    mySendingPort* sendPort; //sending port corr to send_addr
    LossyReceivingPort* recvPort; //receiving port corr to recvAddr;

    try{
        recvAddr = new Address("localhost", ports[0]);
        sendAddr = new Address("localhost", ports[2]);
//        dstAddr =  new Address("localhost", ports[2]); //NEEDS TO GO and edit common.cpp line 380 to get rid of assertion

        recvPort = new LossyReceivingPort(0.0);
        recvPort->setAddress(recvAddr);

        sendPort = new mySendingPort();
        sendPort->setAddress(sendAddr);
//        sendPort->setRemoteAddress(dstAddr); //NEEDS TO GO 

        sendPort->init();
        recvPort->init();

    } catch(const char *reason ){
        cerr << "Exception:" << reason << endl;
        return;
    }

    //pthread_create() - with sender
    struct cShared *sh;
    sh = (struct cShared*)malloc(sizeof(struct cShared));
    sh->fwdRecvPort = recvPort;
    sh->fwdSendPort = sendPort;
    sh->receivingPortNum = ports[0];
    //    sh->max = n;
    //    pthread_t thread;
    pthread_create(thread, 0, &NodeRecProc, sh);
    //    pthread_join(thread, NULL);
}


int main(int argc, char* argv[])
{

    //sender 4000
    //receiver localhost 4001 

    CreateConnectionsList(argc, argv);
    Display2DVector(connectionsList);

    int N = connectionsList.size();

    pthread_t threads[N];

    for(int i = 0; i < N; i++)
    {
        StartNodeThread(&(threads[i]), connectionsList[i]);
    }
    PendingRequestTimer();
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    //void startReceiverThread()

    return 0;
}
