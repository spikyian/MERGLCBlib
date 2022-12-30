# MERGLCBlib
C library for MERGLCB.
Uses XC8.
Has CAN transport functionality using the PIC18 ECAN peripheral.

## The module application
An MERGLCB module using this library needs to consider the following application responsibilities:

### Inputs
   - Monitoring input pins, 
   - Perform behaviour based upon module NV settings,
   - Save input state according to type of input in RAM
   - Send events to the transport layer according to behaviour requirements

### Outputs
   - Receive messages from the transport layer or receive actions from the action queue
   - Use the NV settings to determine the behaviour
   - Save state into non volatile memory e.g. EEPROM
   - Make changes to the output pin state

### On power on
   - Restore output state using the information stored in non volatile memory
   - Initialise input state based upon current input pin state

### Regular poll
   - Update and perform time based behaviour

## A module designer needs to:
 1. Determine which services the module will use.
 2. If the module will use NVs then:
     - Define the NV usage and allocation,
     - Define the memory allocation (type and address) for the NVs,
     - Define the default value (factory reset settings) of the NVs,
     - Determine whether any validation of NV settings is required.
 3. If the module has a CAN interface:
     - Decide where in NVM the CANID is to be stored,
     - Decide how much memory can be used for transmit and receive buffers.
 4. If the module is to support event teaching:
     - Decide the number of events and number of EVs per event,
     - Define the event EV usage and allocation,
     - Define the memory allocation (type and address) for the EVs,
 5. If the module also supports produced events then:
     - If the concept of Happenings is to be used then defined the size of the Happening identifier,
     - Provide a function to provide the current event state given a Happening
 6. If the module also supports consumed events then:
     - If Actions concept is to be used then the size of the Action queue should be defined.
 7. All modules also need:
     - The address and type of NVM where the module's node number is to be stored,
     - The address and type of NVM where the mode is to be stored,
     - Define the module type name, module ID and version,
     - The number of LEDs used to display module state,
     - A macro to obtain the push button state,
     - A macro to set up the ports for the LEDs and push button.

## Other information
   - The module mode is available using the uint8_t mode global variable.
   - The module's node number is available as Word nn global variable.
   - A module may define a function to process MERGLCB messages before being handled by the library.
   - A module may also define a function to process MERGLCB messages if not handled by the library. 
  
  
