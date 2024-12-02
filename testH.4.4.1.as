        sw 0 1 addr0    
        sw 0 2 addr4     
        sw 0 3 addr8     
        sw 0 4 addr12    
        sw 0 5 addr0    
        sw 0 6 addr16    
        lw 0 7 addr0     
        halt
addr0   .fill 0
addr4   .fill 0
addr8   .fill 0
addr12  .fill 0
addr16  .fill 0
