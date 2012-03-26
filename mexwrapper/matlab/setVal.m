function result = setVal(nodeType, nodeAddr, regName, instance, val)
%Set or get the value of the selected register
%Usage 1 - write: setVal(nodeType, nodeAddr, 'regName'/OffsetAddr,instance #, val)
%Usage 2 - read: result = setVal(nodeType, nodeAddr, 'regName', instance #)
%nodeType and nodeAddr can be updated by getNodeAddress function
%List of register names
%FE node:
%     'V_board1'
%     'V_board2'
%     'T_int1'
%     'T_int2'
%     'T_board'
%     'I_fpga'
%     'I_left'
%     'I_right'
%     'tx_count'
%     'rx_count'
%     'err_cntl'
%     'x_select'
%     'x_read'
%     'x_write'
%ADC node:
%     'Gain_U_D'
%     'Gain_Set'
%     'ADC_Reg'
%     'ADC_Writ'
%     'ADC_Read'
%Channel Node:
%     'fsynenab'
%     'fhitenab'
%     'fnowrap'
%     'fhitclr'
%     'fmonclr'
%     'fhitgen'
%     'fmissclr'
%     'fhitsper'
%     'fhitmon'
%     'fthresh'
%     'fchannel'
%     'fhitcnt'
%     'fmisscnt'
%     'fsyntime'
%     'fmaxhits'
%     'fcntlreg'
%
% Update this list of neccessary
regMap = {'FE',0,'V_board1';...
          'FE',1,'V_board2';...
          'FE',2,'T_int1';...
          'FE',3,'T_int2';...
          'FE',4,'T_board';...
          'FE',5,'I_fpga';...
          'FE',6,'I_left';...
          'FE',7,'I_right';...
          'FE',8,'tx_count';...
          'FE',9,'rx_count';...
          'FE',10,'err_cntl';...
          'FE',11,'x_select';...
          'FE',12,'x_read';...
          'FE',13,'x_write';...
          'ADC',0,'Gain_U_D';...
          'ADC',1,'Gain_Set';...
          'ADC',2,'ADC_Reg';...
          'ADC',3,'ADC_Writ';...
          'ADC',4,'ADC_Read';...
          'CHAN',0,'fsynenab';...
          'CHAN',1,'fhitenab';...
          'CHAN',2,'fnowrap';...
          'CHAN',3,'fhitclr';...
          'CHAN',4,'fmonclr';...
          'CHAN',5,'fhitgen';...
          'CHAN',6,'fmissclr';...
          'CHAN',7,'fhitsper';...
          'CHAN',8,'fhitmon';...
          'CHAN',9,'fthresh';...
          'CHAN',10,'fchannel';...
          'CHAN',11,'fhitcnt';...
          'CHAN',12,'fmisscnt';...
          'CHAN',13,'fsyntime';...
          'CHAN',14,'fmaxhits';...
          'CHAN',15,'fcntlreg'};

%organize arrays of node addresses for different node types      
arraySize = size(nodeType,2);

FEBRD = zeros(0);
FEADC = zeros(0);
FECHAN = zeros(0);

%node types: 1 <-> FE, 2 <-> ADC, 3 <-> CHANNEL
for i = 1:arraySize
    if(nodeType(i) == 1)
        FEBRD = [FEBRD, nodeAddr(i)];
    elseif(nodeType(i) == 2)
        FEADC = [FEADC, nodeAddr(i)];
    elseif(nodeType(i) == 3)
        FECHAN = [FECHAN, nodeAddr(i)];
    end
end

%flag for matching register name
regMapped = 0;
%obtained register address from regMap
regAddr = 0;

%check if regName is character array
if(ischar(regName))
    %search for matching register name and get register address
    for i = 1:size(regMap,1)
       if(strcmp(regMap{i,3},regName) == 1)
          regAddr = regMap{i,2};
          regType = regMap{i,1};
          regMapped = 1;
          break; 
       end
    end
%just assign register address if regName input is a number    
elseif(isfinite(regName))
    writeNodeAddr = regName;
    regMapped = 1;
end

%error if no matching register found
if(regMapped == 0)
    error('register %s not found',regName);
end


%check if instance number of node is valid for regName is char array
%if valid, get the node address
if(ischar(regName))
    if(strcmp(regType,'FE') == 1)
        if(instance < 1 || instance > size(FEBRD,2))
           error('invalid FE number: %d', instance); 
        else
           writeNodeAddr = FEBRD(instance);
        end
    elseif(strcmp(regType,'ADC') == 1)
        if(instance < 1 || instance > size(FEADC,2))
           error('invalid ADC number: %d', instance);
        else
           writeNodeAddr = FEADC(instance);
        end
    else
        if(instance < 1 || instance > size(FECHAN,2))
           error('invalid Channel number: %d', instance);
        else
           writeNodeAddr = FECHAN(instance);
        end
    end
end

%write value if function has 5 inputs or read if 4 inputs
if (nargin == 5)
    %write through MSCB 
    mexwrapper('192.168.2.2',writeNodeAddr, 'w', regAddr, val);
else
    %read through MSCB 
    [name, val] = mexwrapper('192.168.2.2',writeNodeAddr, 'r', regAddr);
    result = val;
end