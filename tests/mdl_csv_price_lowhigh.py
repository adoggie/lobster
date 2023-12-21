
import os
import zmq
import time 
import fire

def lowhigh(filename):     
    
    minp = 999999999
    maxp = 0 
    with open(filename, "r") as file:
        lines = file.readlines()
        
        for line in lines:            
            line = line.strip() 
            fs = line.split(",")
            p = float(fs[7])
            if p < minp and p !=0:
                minp = p
            if p > maxp:
                maxp = p
            
    print(filename,":",minp,maxp)

if __name__ == '__main__':
    fire.Fire()
    
