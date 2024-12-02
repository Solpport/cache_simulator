        lw 0 1 addr     
start   add 1 1 2       
        sw 0 2 0        
        beq 1 3 done    
        beq 0 0 start   
done    halt
addr    .fill 0