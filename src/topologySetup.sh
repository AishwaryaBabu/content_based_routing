##\file topologySetup.sh

#!/bin/bash
#while IFS=: read number_of_hosts number_of_routers line1 line2 line3 line4 line5 #reads column wise
#do
#    echo "NumHosts: $number_of_hosts\n
#NumRouters : $number_of_routers\n"
#echo "$line1 $line2 $line3 $line3 $line4 $line5"
#done < topology

filename=topology0
rm -r Host_*
numHosts=$(sed -n '1p' < $filename)
numRouters=$(sed -n '2p' < $filename)
#echo "$numHosts"
#echo "$numRouters"

#to setup hosts
for ((i=3; i < numHosts+3; i++))
do
args=$(sed -n $i'p' < $filename)
j=$(($i-2))
mkdir "Host_$j" 
cp host ./"Host_$j"
cd ./"Host_$j"
gnome-terminal --geometry=35x20 --window-with-profile=host --title=Host$j -x ./host $args  
cd ../
done


#to set up routers
for ((i=3+numHosts; i < 3+numHosts+numRouters; i++))
do
args=$(sed -n $i'p' < $filename)
j=$(($i-2-numHosts))
gnome-terminal --geometry=35x20 --window-with-profile=router --title=Router$j -x ./router $args 
#setterm -term linux -back blue -fore white -clear
done


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

#i=0
#ARRAY=
#head -1 topology |
#while read LINE
#do
#ARRAY+="$LINE"
#   echo "$LINE"
#done
#for LINE in "{$ARRAY[@]}"
#do
#echo "$LINE"
#done




#!!!!!!!!!!!!! TRIAL !!!!!!!!!!!!!!!!
#mkdir "Host_1" 
#cp host ./"Host_1"
#cd ./"Host_1"
#gnome-terminal -x ./host h1 r1
#cd ../
#
#mkdir "Host_2" 
#cp host ./"Host_2"
#cd ./"Host_2"
#gnome-terminal -x ./host h2 r2 
#cd ../
#
#mkdir "Host_3" 
#cp host ./"Host_3"
#cd ./"Host_3"
#gnome-terminal -x ./host h3 r1 
#cd ../
#
#
#gnome-terminal -x ./router r1 h1 h3 r2
#gnome-terminal -x ./router r2 h2 r1 

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
