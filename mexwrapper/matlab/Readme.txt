Matlab usage to set/get register values from MSCB

1. place the mex file (mexwrapper), getNodeAdress, and setVal into the active workspace opened in Matlab

2. use getNodeAddress function to retrieve mapping of node addresses

3. use setVal function and pass in register info, mapped node addresses to read/write to a register


See testbench for example.

