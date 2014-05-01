#!/bin/bash

#while IFS=: read number_of_hosts number_of_routers line1 line2 line3 line4 line5 #reads column wise
#do
#    echo "NumHosts: $number_of_hosts\n
#NumRouters : $number_of_routers\n"
#echo "$line1 $line2 $line3 $line3 $line4 $line5"
#done < topology

numHosts=$(head -n 1 topology)
echo "$numHosts"

i=0
ARRAY=
while read LINE
do
#ARRAY+="$LINE"
   echo "$LINE"
done < topology
#for LINE in "{$ARRAY[@]}"
#do
#echo "$LINE"
#done




#!!!!!!!!!!!!! TRIAL !!!!!!!!!!!!!!!!
#mkdir "Host_0" 
#cp host ./"Host_0"
#cd ./"Host_0"
#gnome-terminal -x ./host localhost 5001
#cd ../
#
#mkdir "Host_1" 
#cp host ./"Host_1"
#cd ./"Host_1"
#gnome-terminal -x ./host localhost 10001 
#cd ../
#
#gnome-terminal -x ./router r1 h1 r2


##!!!!!!!!!!!!   NOTES   !!!!!!!!!!!!!
#
##for(int i = 0; i < numHosts)
##{
##mkdir "Host_$i" 
##cp host ./"Host_$i"
##cd ./"Host_$i"
##gnome-terminal -x ./host args 
##cd ../
##}
#
#for(int i = 0; i < numRouters; i++)
#{
#gnome-terminal -x ./router args 
#}

#    mkdir "host_$LINE"
#   rm -r "host_$LINE"

#to comment: :num1,num2s/^/#/
#to uncomment: num1,num2s/^#//
