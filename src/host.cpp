#include "common.h"
#include "newport.h"
#include <iostream>
#include "math.h"
#include <fstream>		//needed for file checking
#include <stdlib.h>
#include <string>
#include <vector>		//needed for vectors
#include "newport2.h"		//needed for mySendingPort2 (no ARQ)
#include <sstream>
#include <algorithm> 		//needed to do the find command
#include <cstdlib>

#define advertisementInterval 10
#define lossPercent 0.2

using namespace std;

struct adv
{
    SendingPort *my_adv_port;
};

struct res
{
    LossyReceivingPort *my_res_port;
    mySendingPort *my_req_port;
};

std::vector<int> content; //vector of int type to hold content IDs hosted
int host_id = 1; //setting host ID = 0

//setting up the requesting port; we need to initialize it globally because we need to be able to kill the timer from the receiving thread which is running independently of the main program.

void *advertisement(void *args)
{

    struct adv *sh = (struct adv *)args;

    while(1)
    {

        // send using sh->my_adv_port
        // destination address has already been trained to 10001

        int hops = 0; //setting default hop count from host.

        Packet *adv_packet;
        adv_packet = new Packet();
        adv_packet->setPayloadSize(0); //No payload

        PacketHdr *hdr = adv_packet->accessHeader();

        hdr->setHeaderSize(3); //Need 3 bytes for the header
        hdr->setOctet('2',0); //Advertisement Packet type = 2
        hdr->setOctet((char)hops,2); //# of hops to content = 0

        for (unsigned int i=0;i!=content.size();i++)
        {
            hdr->setOctet((char)content[i],1);
            /*
               cout << "The three octets are " << hdr->getOctet(0)<< ", " << hdr->getOctet(1) << ", " << hdr->getOctet(2) << endl;
               cout << "The payload size is " << adv_packet->getPayloadSize() << endl;
               cout << "The header size is " <<adv_packet->getHeaderSize() << endl;
             */

            sh->my_adv_port->sendPacket(adv_packet);
            cout<<"Advertisement has been sent"<<endl;

        } //closes for

        sleep(advertisementInterval); //do this every 10 seconds.
    } //closes while

    return NULL;    
} //closes void

void *receivedata(void *args)
{
    /*REQ Packet - Type 0
      RES Packet - Type 1
      ADV Packet - Type 2*/

    struct res *sh2 = (struct res *)args;
    FILE *data;
    Packet *q;
    short size;
    while(1)
    {

        q = sh2->my_res_port->receivePacket();

        if (q!= NULL)
        {
            char type = q->accessHeader()->getOctet(0);
            //Receiving response
            if (type == '1')
            {
                sh2->my_req_port->setACKflag(true);
                sh2->my_req_port->timer_.stopTimer();
                char *outputstring[1];
                ofstream outputFile;
                int c_id = (int)q->accessHeader()->getOctet(1); //Get the content ID for which a file needs to be made.

                stringstream ss;
                ss << c_id;
                std::string str = ss.str();
                const char* chr = str.c_str();

                outputFile.open(chr,std::fstream::out |std::fstream::trunc); //create a file with the same name
                outputstring[1] = q->getPayload();//get the payload
                outputFile <<outputstring[1];//write the payload to the output file
                cout<<"Request was fulfilled"<<endl; //acknowledge to the user that we are done writing.

            }
            //Servicing a request
            else if (type == '0')
            {
                int c_id = (int)q->accessHeader()->getOctet(1);
                cout<<"Request Packet just came in for content "<<c_id<<endl;
                //Creating Response Packet
                //char* str;
                //itoa (c_id,str, 10);

                bool contentExists = false;
                for(unsigned int i = 0; i < content.size(); i++)
                {
                    if(content[i] == c_id)
                    {
                        contentExists= true;    
                        break;
                    }
                }

                stringstream ss;
                ss << c_id;
                std::string str = ss.str();
                const char* chr = str.c_str();
                data = fopen(chr,"r"); //open file to be sent

                if (!contentExists)
                {
                    cout<<"This content is not hosted here"<<endl;
                    continue;
                }

                else
                {
                    fseek(data,0,SEEK_END);
                    size = ftell(data);
                    rewind(data);

                    char k[size];
                    fread(k,1,size,data); //Reading data into an array.
                    char *o = &k[0]; //Address for the beginning of data

                    Packet *res_packet;
                    res_packet = new Packet();
                    res_packet->setPayloadSize(size);

                    PacketHdr *reshdr = res_packet->accessHeader();

                    reshdr->setHeaderSize(5);
                    reshdr->setOctet('1',0); //Type 1 for response packet
                    reshdr->setOctet((char)c_id,1); //Setting Content ID that is being sent
                    char foreign_host = q->accessHeader()->getOctet(2);
                    reshdr->setOctet(foreign_host,2);//Setting
                    reshdr->setShortIntegerInfo(size,3);//Setting the size of the file

                    res_packet->fillPayload(size,o); //Adding the payload.
                    sh2->my_req_port->sendPacket(res_packet); //Sending the packet to the destination! BOOYA!
                    cout<<"Request for content "<<c_id<<" has been serviced"<<endl;
                }
            }

/*            else if (type == '2')
            {
                char a = q->accessHeader()->getOctet(0);
                int b = (int)q->accessHeader()->getOctet(1);
                char c = q->accessHeader() ->getOctet(2);
                cout << "This packet is of type "<<a<<" with content ID "<<b<< " with "<<c<< " hops to it"<<endl;
            }
*/
        } //Closes if
    }//Closes while
    return NULL;

}//Closes void


int main(int argc, const char * argv[])
{
    cout<<"I am "<<argv[1]<<endl;

    pthread_t thread; //for advertising
    pthread_t thread2; //for receiving all information

    //create advertising sending port with the destination port included. Then we will asend it to the thread.
    Address *my_adv_addr; //We advertise from here
    Address *my_res_addr; //We receive information from here
    Address *my_req_addr; //We request content from here
    Address *router_addr; //We address the router from here
    mySendingPort *my_req_port; //Requesting port; note the change in type (2 vs no 2)
    mySendingPort2 *my_adv_port;//Sending port for advertisements
    LossyReceivingPort *my_res_port; //Receiving port information

    try{
        //Port number calculation
        int sourceID;
        string source(argv[1]);
        sourceID=atoi(source.substr(1).c_str())+63;
        
        string destination(argv[2]);
        int destinationID;
        if(destination.at(0)=='r'){
            destinationID=atoi(destination.substr(1).c_str())-1;
        }
        else if(destination.at(0)=='h'){
            destinationID=atoi(destination.substr(1).c_str())+63;
        }

        int x,y,zSourceRx,zDestinationRx, zSourceTx; //,zDestinationTx
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


        //Address *my_adv_addr; //We advertise from here
        //Address *my_res_addr; //We receive information from here
        //Address *my_req_addr; //We request content from here
        //Address *router_addr; //We address the router from here

        int destinationPortNum = x*(512)+y*(4)+zDestinationRx+8000;
        int receivingPortNum = x*(512)+y*(4)+zSourceRx+8000;
        int sendingPortNum = x*(512)+y*(4)+zSourceTx+8000;
        //int destinationTxPortNum = x*(512)+y*(4)+zDestinationTx+8000;
        int advPortNum = sourceID+3500;

        cout<<destinationPortNum<<" "<<receivingPortNum<<" "<<sendingPortNum<<endl;

        my_req_addr = new Address("localhost", sendingPortNum);
        my_res_addr = new Address("localhost", receivingPortNum);
        my_adv_addr = new Address("localhost", advPortNum);
        router_addr = new Address("localhost", destinationPortNum); //assume this is R1

        //Initialize requesting port (sending)
        my_req_port = new mySendingPort();
       my_req_port->setAddress(my_req_addr);
      my_req_port->setRemoteAddress(router_addr);
     my_req_port->init();                               //<<<<<PROBLEMM>>>>>>>

        //Initialize advertising port (sending)
        my_adv_port = new mySendingPort2();
        my_adv_port->setAddress(my_adv_addr);
        my_adv_port->setRemoteAddress(router_addr);
        my_adv_port->init();

        //Initialize receiving port (receiving)
        my_res_port = new LossyReceivingPort(lossPercent);
        my_res_port->setAddress(my_res_addr);
        my_res_port->init();
   }

    catch(const char *reason)
    {
        cerr << "Exception:" << reason << endl;
        exit(-1);
    }

    struct adv *sh;
    sh = (struct adv*)malloc(sizeof(struct adv));
    sh->my_adv_port = my_adv_port;


    //creating thread to advertise

    pthread_create(&(thread), 0,&advertisement,sh);

    struct res *sh2;
    sh2 = (struct res*)malloc(sizeof(struct res));
    sh2->my_res_port = my_res_port;
    sh2->my_req_port = my_req_port;

    //creating thread to receive
    pthread_create(&(thread2),0,&receivedata,sh2);

    while(1)
    {

        string input;
        string input2;
        //int hdrSize = 256;
        //get user input on
        cout << "Prompt> ";
        cin >> input >> input2;      

        if(input.compare("add")==0)
        {
            //string src_path = "/home/tua96426/Desktop/";
            //string dest_path = "/home/tua96426/Desktop/src/";
            string src_path = "../";
            string path = "cp " + src_path + input2 + " " + input2;
            const char* content_add = path.c_str();
            system(content_add); //Makes a system call to physically copy file from parent directory to child directory

            //Code checks to see if the file actually exists; else no reason to add to the content table.
            FILE *pfile;
            pfile = fopen(input2.c_str(),"r");
            if (pfile == NULL)
            {
                continue;
            }

            else
            {
                fclose(pfile); //need to close the open file!
                int input2_int = atoi(input2.c_str());

                //Add value to content table if its unique.

                if(std::find(content.begin(),content.end(),input2_int) == content.end())
                {cout<<"You have unique"<<endl;
                    content.push_back(input2_int);
                }

                cout << "This host has "<< content.size() <<" content(s) in its library" <<endl;

                for(unsigned int n=0;n<content.size();n++)
                {
                    cout<<content[n]<<", ";
                }

                printf("\n");
            } //closes else

        } //closes if compare

        if(input.compare("get")==0)
        {
            cout << "You asked to get something" <<endl;

            Packet *req_packet;
            req_packet = new Packet();
            req_packet->setPayloadSize(0); //No Payload

            PacketHdr *rqhdr = req_packet->accessHeader();

            rqhdr->setHeaderSize(3); //Need 3 bytes for the header
            rqhdr->setOctet('0',0); //Request packet = type 0
            rqhdr->setOctet((char)host_id,2); //Setting host id

            int input2_int = atoi(input2.c_str());

            rqhdr->setOctet((char)input2_int,1); //Setting content request message

            my_req_port->sendPacket(req_packet);
            my_req_port->lastPkt_ = req_packet;
            //cout<<"First octet "<<rqhdr->getOctet(0)<<"Second Octet "<<rqhdr->getOctet(1)<<"Third Octet "<<rqhdr->getOctet(2)<<endl;
            cout<<"Request for content "<<input2_int<<" has been sent"<<endl;
            my_req_port->setACKflag(false);
            my_req_port->timer_.startTimer(5);

            while(!my_req_port->isACKed())
            {
                sleep(1);
                if(!my_req_port->isACKed())
                {
                    sleep(2);
                    if(!my_req_port->isACKed())
                    {
                        sleep(3);
                        if(!my_req_port->isACKed())
                        {
                            sleep(10);
                            if(!my_req_port->isACKed())
                            {
                                sleep(12);
                                if(!my_req_port->isACKed())
                                {
                                    cout<<"Giving up.."<<endl;							
                                    my_req_port->setACKflag(true);
                                }	
                            }
                        }
                    }
                } 
                else{continue;}
            }
        }

        if(input.compare("delete")==0)
        {

            cout <<"You asked to delete something" <<endl;

            string path_r = "rm " + input2;
            const char* content_remove = path_r.c_str();
            system(content_remove);

            for( std::vector<int>::iterator iter = content.begin(); iter !=content.end();++iter)
            {

                if(*iter == atoi(input2.c_str()))
                {
                    content.erase(iter);
                    cout<< "Content "<<input2<<" has been removed"<<endl;
                    break;
                }
            }

            for(unsigned int n=0;n<content.size();n++)
            {
                cout<<content[n]<<" ";
            }

            printf("\n");


        }

        if(input.compare("exit")==0)
        {
            cout <<"shutting down host" <<endl;
            return 0;
        }


    } //while close


    return 0;
}
