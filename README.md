content_based_routing
=====================

Topology File format

m (Number of hosts)
n (Number of routers)
h1 {Connected router}
h2 ----------------------
-------------------------
-------------------------
hm-----------------------
r1 {Connected components}
r2-----------------------
-------------------------
-------------------------
rn-----------------------

Example:

3
7
h1 r5
h2 r6
h3 r3
r1 r2 r3
r2 r1 r4 r5
r3 h3 r1 r6 r7
r4 r2
r5 h1 r2
r6 h2 r3
r7 r3

How to setup network?

1. 'make' the .cpp files
2. Execute 'chmod +x topologySetup.sh' on terminal
3. Execute 'bash topologySetup.sh' on terminal
4. This opens terminals for each host and router, creates directories for each host with name     Host_{host number}

How to use host terminal?

add {content ID} - This command adds file name with content ID in the current directory to Host_{content ID}.An entry is added in to content table and is available for advertisement.

Ex: add 1

get {content ID} - On this command host sends a request packet to the edge router for content.

Ex: get 4

delete {content ID} - On this command file name with content ID is deleted from  Host_{content ID} and removed from content table. This content is nomore advertised.



