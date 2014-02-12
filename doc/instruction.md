Instruction Format
==
nop
--
Desciption

    No operation

Format

    nop
    
Arguments

    Nothing
    

move 
--
Description

    Move data to reg

Format 
    
    move dest, sour
    
Arguments

    [0] dest:reg
    [1] sour:any

return 
--
Description

    Return from a function

Format
    
    return reg

Arguments

    [0] value:reg

const 
--
Description
    
    Set a value of dest register  to constant value
    
Format
    
    const dest, value
    
Arguments

    [0] dest :reg
    [1] value:const
    

monitor
--
flags = DVM_FLAG_MONITOR_ENT

Description

    Enter the monitor

flags  = DVM_FLAG_MONITOR_EXT

Description
    
    Exit the monitor
    
Format

    monitor R

Arguments

    [0] R:reg

check-cast
--
Description

    Check if object in the register can cast. Throws java.lang.ClassCastException

Format
    
    check-cast obj, cls

Arguments

   [0] obj: reg
   [1] cls: classpath

throw
--
Description

    Thow a exception 

Format
    
    throw exception

Arguments

    [0] exception: reg

goto
--
Description
    
    Unconditional jump instruction

Format

    goto label
    
Arguments

    [0] lab: label

switch
--
switch flags = DVM_FLAG_SWITCH_SPARSE

Description
    
    Sparse swithch 

Format
    
    switch condition, branchlist
    
Arguments

    [0] conditon: reg
    [1] branchlist: dalvik_sparse_switch_branch_t

    
switch flags = DVM_FLAG_SWITCH_PACKED

Description 
    
    Packed switch. The next instruction would be jumptalbe[condition - begin]
    
Format
    
    swith condition, begin, jumptable

Arguments
    
    [0] condition: integer reg
    [1] begin    : integer constant
    [2] jumptable: label list


    
cmp
--
Description

    Compare two values, and store the result in result register
    
Format

    compare val1, val2

Arguments

    [0] val1: Any
    [1] val2: Any
    
if
--
Description
    
    Conditional branch. The flag indicates when jump to the label
    
Format
    
    if val1, val2, lab
    
Arguments

    [0] val1: Any
    [1] val2: Any
    [2] lab : Label


array, instance
--
Description

    Object Maintainance. The flags indicate operations 
    DVM_FLAG_ARRAY_GET == DVM_FLAG_INSTANCE_GET : Get a member
    DVM_FLAG_ARRAY_PUT == DVM_FLAG_INSTANCE_PUT : Put a member
    DVM_FLAG_INSTANCE_SGET : Get a static member
    DVM_FLAG_INSTANCE_SPUT : Put a static member
    
Format
    
    There are serval formats
    1   array dest, object, index 
    2   instance dest, path, type
    3   instance dest, object, path, type
    
Arguments:

    For type 1:
        [0] dest   : reg
        [1] object : object reg
        [2] index  : integer
    
    For type 2:
        [0] dest    : reg
        [1] path    : const class path
        [2] type    : type descriptor
    
    For type 3:
        [0] dest    : reg
        [1] object  : object reg
        [2] path    : const class path
        [3] type    : type descriptor
    
    
    
invoke
--
Description
    
    Invoke a method. The type is specified by the flags
    
    DVM_FLAG_INVOKE_VIRTUAL         call a virtual function
    DVM_FLAG_INVOKE_SUPER           call closet superclass' virtual method
    DVM_FLAG_INVOKE_DIRECT          call a class method directly
    DVM_FLAG_INVOKE_STATIC          call a static method
    DVM_FLAG_INVOKE_INTERFACE       call a interface method implememnt in the class 
    
    DVM_FLAG_INVOKE_RANGE           the argument table is a range

Format

    ranged          invoke  classpath, methodname, typelist ,begin, end
        call the method passing arguments v(begin), v(begin + 1), ... , v(end)
    unranged        invoke  classpath, methodname, typelist ,arg0, arg1, arg2, arg3, ..., argN
        call the method passing arguments arg0, arg1, arg2,... argN
    
Arguments

    For ranged:
        
        [0] classpath:  const class path
        [1] methodname: const method name
        [2] typelist:   dalvik_type_t[], with terminating NULL
        [3] begin:      integer
        [4] end:        integer
        
    
    For unranged:
        
        [0] classpath
        [1] methodname
        [2] typelist
        [3] arg0
        [4] arg1
        [5] arg2
        .....
        [N+3] argN
        
unop
--
Description

    Arithmetic operations takes signle element
    
    Flags indicates the what method we want to perform
    
Format

    unop dest, sour
    
Argument
    
    [0] dset
    [1] sour
    
binaryop
--
Description
    
    binary opeartors
    
Format
    binaryop dest, sour

new
--
Description 

    create a new instance of the type
    Flag: instance/type
Format
    
    new reg,type
    
Arguments
    
    [0] reg
    [1] type
    

