nop

move 
    dest:reg
    sour:any

return 
    value:reg

const 
    value:const

monitor flags = DVM_FLAG_MONITOR_ENT
    v1:reg

monitor flags = DVM_FLAG_MONITOR_EXT
    v1:reg

check-cast
   obj: reg
   cls: classpath

throw
    exp: reg

goto
    lab: label

switch flags = DVM_FLAG_SWITCH_SPARSE
    
switch flags = DVM_FLAG_SWITCH_PACKED
    cond: reg
    
