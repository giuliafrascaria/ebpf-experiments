#!/bin/bash
#test verifier complexity for 5 different prog lengths

for j in {8..10..2}
do
    #for i in {1..12300..4095}
        #do
            for k in {1..1}
            do

                sudo su -c "echo 1 > /proc/sys/vm/drop_caches"
                sudo su -c "echo 2 > /proc/sys/vm/drop_caches"
                sudo su -c "echo 3 > /proc/sys/vm/drop_caches"
                taskset -c 0 sudo ./helperbench write$j 1000
                #taskset -c 0 sudo ./helperbench read$i
        		#taskset -c 0 sudo ./endtoend filter25 sum25 $i
            done
        #done
            
        sleep 1
done
