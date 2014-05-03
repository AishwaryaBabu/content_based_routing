#include<iostream>
#include"common.h"
#include"newport.h"
#include<vector>
#include<list>

#define rtTimeToExpire 30
#define prtTimeToExpire 30
#define timerWrap 6000
#define sleepDelay 1
#define lossPercent 0.05
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
    cout<<endl;  
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
            zSourceTx=3;
            //            zDestinationTx=2;
            zSourceRx=1;
            zDestinationRx=0;
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
    numHops+=1;
//    int dstPortNum = SearchConnectionsTable(recPortNum);
    //Using config information build routing table
    if(routingTable.size()==0)
    {
        vector<int> routingRow;
        routingRow.push_back(contentId);
        routingRow.push_back(recPortNum);
        routingRow.push_back(numHops);
        routingRow.push_back(globalTimer+rtTimeToExpire);      //Time to expire
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
                if(numHops <= routingTable[i][2])
                {
                    routingTable[i][1]=recPortNum;
                    routingTable[i][2]=numHops;
                    routingTable[i][3]=globalTimer+rtTimeToExpire;      //Time to expire;
                }
                break;
            }
        }
        if(!contentExists)
        {
            vector<int> routingRow;
            routingRow.push_back(contentId);
            routingRow.push_back(recPortNum);
            routingRow.push_back(numHops);
            routingRow.push_back(globalTimer+rtTimeToExpire);//Time to expire;
            routingTable.push_back(routingRow);
        }
    }

    cout<<"Ad received - Routing Table:"<<endl;   
    Display2DVector(routingTable);
}

void UpdateRoutingTableEntryTTL()
{
    for(unsigned int i=0; i<routingTable.size(); i++)
    {
        routingTable[i][3] = routingTable[i][3] - timerWrap;
    }

}

void DeleteRoutingTableEntry(int row)
{
            routingTable.erase(routingTable.begin()+row);
            cout<<"Deleted entry - Routing Tab:"<<endl;
            Display2DVector(routingTable);

}

void CheckRoutingTableEntryExpired(int currentTime)
{
    for(unsigned int i=1; i<=routingTable.size(); i++)
    {
        if(routingTable[i-1][3] == currentTime)
        {
            DeleteRoutingTableEntry(i-1);
            //routingTable.erase(routingTable.begin()+i-1);
            i--; // to ensure the deletion of 0th entry
        }
    }

}

int getReceivingPort(int requestedContentId)
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

int getNumberHops(int requestedContentId)
{
    for(unsigned int i = 0; i < routingTable.size(); i++)
    {
        if(requestedContentId == routingTable[i][0])
        {
            return routingTable[i][2]; //Returns number of hops   
        }
    }
    return -1;
}

bool contentIdExists(int requestedContentId)
{
    for(unsigned int i = 0; i < routingTable.size(); i++)
    {
        if(requestedContentId == routingTable[i][0])
            return true;
    }
    return false;
}

void UpdatePendingRequestTable(int requestedContentId, int requestingHostId, int receivingPort)
{
    vector<int> pendingRequestRow;
    pendingRequestRow.push_back(requestedContentId);
    pendingRequestRow.push_back(requestingHostId);

    //Maintain destination Port numbers at PRT  
    int destPort = SearchConnectionsTable(receivingPort);

    bool contentExists = false;
    for(unsigned int i = 0; i < pendingRequestTable.size(); i++)
    {   
        if((requestedContentId == pendingRequestTable[i][0]) && (requestingHostId == pendingRequestTable[i][1]))
        {
            contentExists = true;
            pendingRequestTable[i][3] = globalTimer+ prtTimeToExpire;
            break;
        }
    }

    if(!contentExists)
    {
        pendingRequestRow.push_back(destPort);
        pendingRequestRow.push_back(globalTimer+prtTimeToExpire); // Time to expire
        pendingRequestTable.push_back(pendingRequestRow);

    }
    cout<<"Request recd - Pending Req Tab:"<<endl;
    Display2DVector(pendingRequestTable);
}

void UpdatePendingRequestTableTTL()
{
    for(unsigned int i=0; i<pendingRequestTable.size(); i++)
    {
        pendingRequestTable[i][3] = pendingRequestTable[i][3] - timerWrap;
    }
    cout<<"Updated TTL - Pending Request Table"<<endl;
    Display2DVector(pendingRequestTable);
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
    cout<<"Deleted entry - PRT:"<<endl;
    if(pendingRequestTable.size()==0)
        cout<<"No pending req"<<endl;
    else
        Display2DVector(pendingRequestTable);

}

void CheckPendingRequestTableExpired(int currentTime)
{
    for(unsigned int i=1; i<=pendingRequestTable.size(); i++)
    {
        if(pendingRequestTable[i-1][3] == currentTime)
        {
            int contentID = pendingRequestTable[i-1][0];
            int hostID = pendingRequestTable[i-1][1];
            DeletePendingRequestTableEntry(contentID, hostID);
            //            pendingRequestTable.erase(pendingRequestTable.begin()+i-1);
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

void ExpiryTimer()
{
    while(1)
    {
        sleep(sleepDelay);
        if(globalTimer>=timerWrap)
        {
            globalTimer=globalTimer-timerWrap;
            UpdatePendingRequestTableTTL();
            UpdateRoutingTableEntryTTL();
        }
        globalTimer++;
        CheckPendingRequestTableExpired(globalTimer);
        CheckRoutingTableEntryExpired(globalTimer);
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
                
                int nextHopRecvPortNum = getReceivingPort(requestedContentId);
                int nextHopDestPortNum = SearchConnectionsTable(nextHopRecvPortNum);
                Address* dstAddr = new Address("localhost", nextHopDestPortNum);
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

                int currentHops = getNumberHops(receivedContentId);
                int currentReceivingPort = getReceivingPort(receivedContentId);
                
                bool noEntry = !(contentIdExists(receivedContentId));
//                if(currentReceivingPort==-1)
//                    noEntry = true;

                bool forwardFlag = false;
                if((receivingPortNum == currentReceivingPort))
                {
                    if(numHops <= (currentHops-1))
                    {
                        forwardFlag = true;
                    }
                }
                else
                { 
                    if(numHops < (currentHops-1))
                    {
                        forwardFlag = true;
                    }
                }
                
            
                if(forwardFlag || noEntry)
                {
                    AddRoutingTableEntry(receivedContentId, receivingPortNum, numHops); //Takes care of updating timer and comparing num Hops
                    //Routing table converts receiving port num to appropriate dest port num
                    //Increment number of hops
                    numHops++;
                    recvPacket->accessHeader()->setOctet(char(numHops), 2);

                    //Forward to all other ports
                    int destPortNumToSkip = SearchConnectionsTable(receivingPortNum);
                    //BroadcastPacket(destPortNumToSkip, recvPacket);
                    for(unsigned int i = 0; i < connectionsList.size(); i++)
                    {
                        int destPort = connectionsList[i][1];
                        if(destPortNumToSkip != destPort)
                        {
                            //                        cout<<destPort<<endl;
                            Address* dstAddr = new Address("localhost", destPort);
                            sh->fwdSendPort->setRemoteAddress(dstAddr);
                            sh->fwdSendPort->sendPacket(recvPacket);
                            delete(dstAddr);
                        }
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

        recvPort = new LossyReceivingPort(lossPercent);
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

    cout<<"I am "<<argv[1]<<endl;
    //sender 4000
    //receiver localhost 4001 

    CreateConnectionsList(argc, argv);
    //    Display2DVector(connectionsList);

    int N = connectionsList.size();

    pthread_t threads[N];

    for(int i = 0; i < N; i++)
    {
        StartNodeThread(&(threads[i]), connectionsList[i]);
    }
    ExpiryTimer();
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    //void startReceiverThread()

    return 0;
}
