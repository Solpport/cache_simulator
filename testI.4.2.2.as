        sw 0 1 addr0    
        sw 0 2 addr1   
        sw 0 3 addr2      
        sw 0 4 addr3     
        sw 0 5 addr0      
        sw 0 6 addr1    
        lw 0 7 addr2      
        lw 0 8 addr3      
        halt
addr0   .fill 0
addr1   .fill 0
addr2   .fill 0
addr3   .fill 0