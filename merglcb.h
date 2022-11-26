#ifndef _MERGLCB_H_
#define _MERGLCB_H_

//
// MERGLCB Service IDs
//
#define SERVICE_ID_MNS      1
#define SERVICE_ID_NV       2
#define SERVICE_ID_CAN      3
#define SERVICE_ID_TEACH    4
#define SERVICE_ID_PRODUCER 5
#define SERVICE_ID_CONSUMER 6
#define SERVICE_ID_EVENTACK 9
#define SERVICE_ID_BOOT     10



// 
// MERGLCB opcodes list
// 
// Packets with no data bytes
// 
#define OPC_ACK	0x00	// General ack
#define OPC_NAK	0x01	// General nak
#define OPC_HLT	0x02	// Bus Halt
#define OPC_BON	0x03	// Bus on
#define OPC_TOF	0x04	// Track off
#define OPC_TON	0x05	// Track on
#define OPC_ESTOP	0x06	// Track stopped
#define OPC_ARST	0x07	// System reset
#define OPC_RTOF	0x08	// Request track off
#define OPC_RTON	0x09	// Request track on
#define OPC_RESTP	0x0a	// Request emergency stop all
#define OPC_RSTAT	0x0c	// Request node status
#define OPC_QNN	0x0d	// Query nodes
// 
#define OPC_RQNP	0x10	// Read node parameters
#define OPC_RQMN	0x11	// Request name of module type
// 
// Packets with 1 data byte
// 
#define OPC_KLOC	0x21	// Release engine by handle
#define OPC_QLOC	0x22	// Query engine by handle
#define OPC_DKEEP	0x23	// Keep alive for cab
// 
#define OPC_DBG1	0x30	// Debug message with 1 status byte
#define OPC_EXTC	0x3F	// Extended opcode
// 
// Packets with 2 data bytes
// 
#define OPC_RLOC	0x40	// Request session for loco
#define OPC_QCON	0x41	// Query consist
#define OPC_SNN	0x42	// Set node number
#define OPC_ALOC	0X43	// Allocate loco (used to allocate to a shuttle in cancmd)
// 
#define OPC_STMOD	0x44	// Set Throttle mode
#define OPC_PCON	0x45	// Consist loco
#define OPC_KCON	0x46	// De-consist loco
#define OPC_DSPD	0x47	// Loco speed/dir
#define OPC_DFLG	0x48	// Set engine flags
#define OPC_DFNON	0x49	// Loco function on
#define OPC_DFNOF	0x4A	// Loco function off
#define OPC_SSTAT	0x4C	// Service mode status
#define OPC_NNRSM	0x4F	// Reset to manufacturer's defaults
// 
#define OPC_RQNN	0x50	// Request Node number in setup mode
#define OPC_NNREL	0x51	// Node number release
#define OPC_NNACK	0x52	// Node number acknowledge
#define OPC_NNLRN	0x53	// Set learn mode
#define OPC_NNULN	0x54	// Release learn mode
#define OPC_NNCLR	0x55	// Clear all events
#define OPC_NNEVN	0x56	// Read available event slots
#define OPC_NERD	0x57	// Read all stored events
#define OPC_RQEVN	0x58	// Read number of stored events
#define OPC_WRACK	0x59	// Write acknowledge
#define OPC_RQDAT	0x5A	// Request node data event
#define OPC_RQDDS	0x5B	// Request short data frame
#define OPC_BOOT	0x5C	// Put node into boot mode
#define OPC_ENUM	0x5D	// Force can_id self enumeration
#define OPC_NNRST	0x5E	// Reset node (as in restart)
#define OPC_EXTC1	0x5F	// Extended opcode with 1 data byte
// 
// Packets with 3 data bytes
// 
#define OPC_DFUN	0x60	// Set engine functions
#define OPC_GLOC	0x61	// Get loco (with support for steal/share)
#define OPC_ERR	0x63	// Command station error
#define OPC_CMDERR	0x6F	// Errors from nodes during config
// 
#define OPC_EVNLF	0x70	// Event slots left response
#define OPC_NVRD	0x71	// Request read of node variable
#define OPC_NENRD	0x72	// Request read stored event by index
#define OPC_RQNPN	0x73	// Request read module parameters
#define OPC_NUMEV	0x74	// Number of events stored response
#define OPC_CANID	0x75	// Set canid
#define OPC_EXTC2	0x7F	// Extended opcode with 2 data bytes
// 
// Packets with 4 data bytes
// 
#define OPC_RDCC3	0x80	// 3 byte DCC packet
#define OPC_WCVO	0x82	// Write CV byte Ops mode by handle
#define OPC_WCVB	0x83	// Write CV bit Ops mode by handle
#define OPC_QCVS	0x84	// Read CV
#define OPC_PCVS	0x85	// Report CV
#define OPC_NVSETRD 0x8E    // Set and read back NV
// 
#define OPC_ACON	0x90	// on event
#define OPC_ACOF	0x91	// off event
#define OPC_AREQ	0x92	// Accessory Request event
#define OPC_ARON	0x93	// Accessory response event on
#define OPC_AROF	0x94	// Accessory response event off
#define OPC_EVULN	0x95	// Unlearn event
#define OPC_NVSET	0x96	// Set a node variable
#define OPC_NVANS	0x97	// Node variable value response
#define OPC_ASON	0x98	// Short event on
#define OPC_ASOF	0x99	// Short event off
#define OPC_ASRQ	0x9A	// Short Request event
#define OPC_PARAN	0x9B	// Single node parameter response
#define OPC_REVAL	0x9C	// Request read of event variable
#define OPC_ARSON	0x9D	// Accessory short response on event
#define OPC_ARSOF	0x9E	// Accessory short response off event
#define OPC_EXTC3	0x9F	// Extended opcode with 3 data bytes
// 
// Packets with 5 data bytes
// 
#define OPC_RDCC4	0xA0	// 4 byte DCC packet
#define OPC_WCVS	0xA2	// Write CV service mode
#define OPC_GRSP    0xAF    // Generic response
// 
#define OPC_ACON1	0xB0	// On event with one data byte
#define OPC_ACOF1	0xB1	// Off event with one data byte
#define OPC_REQEV	0xB2	// Read event variable in learn mode
#define OPC_ARON1	0xB3	// Accessory on response (1 data byte)
#define OPC_AROF1	0xB4	// Accessory off response (1 data byte)
#define OPC_NEVAL	0xB5	// Event variable by index read response
#define OPC_PNN     0xB6	// Response to QNN
#define OPC_ASON1	0xB8	// Accessory short on with 1 data byte
#define OPC_ASOF1	0xB9	// Accessory short off with 1 data byte
#define OPC_ARSON1	0xBD	// Short response event on with one data byte
#define OPC_ARSOF1	0xBE	// Short response event off with one data byte
#define OPC_EXTC4	0xBF	// Extended opcode with 4 data bytes
// 
// Packets with 6 data bytes
// 
#define OPC_RDCC5	0xC0	// 5 byte DCC packet
#define OPC_WCVOA	0xC1	// Write CV ops mode by address
#define OPC_CABDAT	0xC2	// Cab data (cab signalling)
#define OPC_FCLK	0xCF	// Fast clock
// 
#define OPC_ACON2	0xD0	// On event with two data bytes
#define OPC_ACOF2	0xD1	// Off event with two data bytes
#define OPC_EVLRN	0xd2	// Teach event
#define OPC_EVANS	0xd3	// Event variable read response in learn mode
#define OPC_ARON2	0xD4	// Accessory on response
#define OPC_AROF2	0xD5	// Accessory off response
#define OPC_ASON2	0xD8	// Accessory short on with 2 data bytes
#define OPC_ASOF2	0xD9	// Accessory short off with 2 data bytes
#define OPC_ARSON2	0xDD	// Short response event on with two data bytes
#define OPC_ARSOF2	0xDE	// Short response event off with two data bytes
#define OPC_EXTC5	0xDF	// Extended opcode with 5 data bytes
// 
// Packets with 7 data bytes
// 
#define OPC_RDCC6	0xE0	// 6 byte DCC packets
#define OPC_PLOC	0xE1	// Loco session report
#define OPC_NAME	0xE2	// Module name response
#define OPC_STAT	0xE3	// Command station status report
#define OPC_DTXC	0xE9	// CBUS long message packet
#define OPC_PARAMS	0xEF	// Node parameters response
// 
#define OPC_ACON3	0xF0	// On event with 3 data bytes
#define OPC_ACOF3	0xF1	// Off event with 3 data bytes
#define OPC_ENRSP	0xF2	// Read node events response
#define OPC_ARON3	0xF3	// Accessory on response
#define OPC_AROF3	0xF4	// Accessory off response
#define OPC_EVLRNI	0xF5	// Teach event using event indexing
#define OPC_ACDAT	0xF6	// Accessory data event: 5 bytes of node data (eg: RFID)
#define OPC_ARDAT	0xF7	// Accessory data response
#define OPC_ASON3	0xF8	// Accessory short on with 3 data bytes
#define OPC_ASOF3	0xF9	// Accessory short off with 3 data bytes
#define OPC_DDES	0xFA	// Short data frame aka device data event (device id plus 5 data bytes)
#define OPC_DDRS	0xFB	// Short data frame response aka device data response
#define OPC_DDWS	0xFC	// Device Data Write Short
#define OPC_ARSON3	0xFD	// Short response event on with 3 data bytes
#define OPC_ARSOF3	0xFE	// Short response event off with 3 data bytes
#define OPC_EXTC6	0xFF	// Extended opcode with 6 data byes

#define OPC_RDGN    0x87
#define OPC_RQSD    0x78
#define OPC_MODE    0x76
#define OPC_GSTOP   0x12
#define OPC_SQU     0x4E
#define OPC_SD      0x8C
#define OPC_ESD     0xE7



// MANUFACTURER
#define MANU_MERG	165
// MODULE ID
#define MTYP_MERGLCB    0xFC

//
// Parameters
//
#define PAR_NUM 	0	// Number of parameters
#define PAR_MANU	1	// Manufacturer id
#define PAR_MINVER	2	// Minor version letter
#define PAR_MTYP	3	// Module type code
#define PAR_EVTNUM	4	// Number of events supported
#define PAR_EVNUM	5	// Event variables per event
#define PAR_NVNUM	6	// Number of Node variables
#define PAR_MAJVER	7	// Major version number
#define PAR_FLAGS	8	// Node flags
#define PAR_CPUID	9	// Processor type
#define PAR_BUSTYPE	10	// Bus type
#define PAR_LOAD1	11	// load address, 4 bytes
#define PAR_LOAD2	12
#define PAR_LOAD3	13
#define PAR_LOAD4	14
#define PAR_CPUMID	15	// CPU manufacturer's id as read from the chip config space, 4 bytes (note - read from cpu at runtime, so not included in checksum)
#define PAR_CPUMAN	19	// CPU manufacturer code
#define PAR_BETA	20	// Beta revision (numeric), or 0 if release

// 
// BUS type that module is connected to
// 
#define PB_CAN	1	// 
#define PB_ETH	2	// 
#define PB_MIWI	3	// 

// 
// Error codes for OPC_CMDERR
// 
#define CMDERR_INV_CMD	1	// 
#define CMDERR_NOT_LRN	2	// 
#define CMDERR_NOT_SETUP	3	// 
#define CMDERR_TOO_MANY_EVENTS	4	// 
#define CMDERR_NO_EV	5	// 
#define CMDERR_INV_EV_IDX	6	// 
#define CMDERR_INVALID_EVENT	7	// 
#define CMDERR_INV_EN_IDX	8	// now reserved
#define CMDERR_INV_PARAM_IDX	9	// 
#define CMDERR_INV_NV_IDX	10	// 
#define CMDERR_INV_EV_VALUE	11	// 
#define CMDERR_INV_NV_VALUE	12	// 

//
// GRSP codes
//
#define GRSP_OK                 0
#define GRSP_UNKNOWN_NVM_TYPE   254

//
// Modes
//
#define MODE_UNINITIALISED      0
#define MODE_SETUP      1
#define MODE_NORMAL     2
#define MODE_LEARN      3
#define MODE_EVENT_ACK  4
#define MODE_BOOT       5
#define MODE_BOOT2      6


// NVM types
#define EEPROM_NVM_TYPE     1
#define FLASH_NVM_TYPE      2

int readNVM(uint8_t type, unsigned int index);
uint8_t writeNVM(uint8_t type, unsigned int index, uint8_t value);

// XC8 doesn't support function overloading nor varargs
void sendMessage0(uint8_t opc);
void sendMessage1(uint8_t opc, uint8_t data1);
void sendMessage2(uint8_t opc, uint8_t data1, uint8_t data2);
void sendMessage3(uint8_t opc, uint8_t data1, uint8_t data2, uint8_t data3);
void sendMessage4(uint8_t opc, uint8_t data1, uint8_t data2, uint8_t data3, uint8_t data4);
void sendMessage5(uint8_t opc, uint8_t data1, uint8_t data2, uint8_t data3, uint8_t data4, uint8_t data5);
void sendMessage6(uint8_t opc, uint8_t data1, uint8_t data2, uint8_t data3, uint8_t data4, uint8_t data5, uint8_t data6);
void sendMessage7(uint8_t opc, uint8_t data1, uint8_t data2, uint8_t data3, uint8_t data4, uint8_t data5, uint8_t data6, uint8_t data7);
void sendMessage(uint8_t opc, uint8_t len, uint8_t data1, uint8_t data2, uint8_t data3, uint8_t data4, uint8_t data5, uint8_t data6, uint8_t data7);


typedef struct Message {
    uint8_t opc;
    uint8_t len;
    uint8_t bytes[7];
} Message;

typedef union Word {
    struct {
        uint8_t hi;
        uint8_t lo;
    } bytes;
    uint16_t word;
} Word;

typedef struct Service {
    uint8_t serviceNo;
    uint8_t version;
    void (* factoryReset)(void);
    void (* powerUp)(void);
    uint8_t (* processMessage)(Message * m);
    void (* poll)(void);
    void (* highIsr)(void);
    void (* lowIsr)(void);
    //void modes();
    //void statusCodes();
    //void diagnostics();
} Service;

extern Service * services[];

extern void factoryReset(void);
extern void powerUp(void);
extern void processMessage(Message *);
extern void poll(void);
extern void highIsr(void);
extern void lowIsr(void);
extern Service * findService(uint8_t id);
extern uint8_t have(uint8_t id);

extern Message * getReceiveBuffer(void);

extern uint8_t APP_isSuitableTimeToWriteFlash(void);


/////////////////////////////////////////////////////
// TickTime
/////////////////////////////////////////////////////
#if defined(CPUF18K) || defined(_PIC18)
    /* this section is based on the Timer 0 module of the PIC18 family */

//   Prescaler is now calculated from clock MHz in the init routine - which assumes clock is an exact number of MHz
//    #define ONE_SECOND (((DWORD)CLOCK_FREQ/1000 * 62500) / (SYMBOL_TO_TICK_RATE / 1000))
//    /* SYMBOLS_TO_TICKS to only be used with input (a) as a constant, otherwise you will blow up the code */
//    #define SYMBOLS_TO_TICKS(a) (((DWORD)CLOCK_FREQ/100000) * a / ((DWORD)SYMBOL_TO_TICK_RATE/100000))
//    #define TICKS_TO_SYMBOLS(a) (((DWORD)SYMBOL_TO_TICK_RATE/100000) * a / ((DWORD)CLOCK_FREQ/100000))


    #define TMR_IF          INTCONbits.TMR0IF
    #define TMR_IE          INTCONbits.TMR0IE
    #define TMR_IP          INTCON2bits.TMR0IP
    #define TMR_ON          T0CONbits.TMR0ON
    #define TMR_CON         T0CON
    #define TMR_L           TMR0L
    #define TMR_H           TMR0H
    
    
#elif defined(__dsPIC30F__) || defined(__dsPIC33F__) || defined(__PIC24F__) || defined(__PIC24FK__) || defined(__PIC24H__)
    /* this section is based on the Timer 2/3 module of the dsPIC33/PIC24 family */
    #if(CLOCK_FREQ <= 125000)
        #define CLOCK_DIVIDER 1
        #define CLOCK_DIVIDER_SETTING 0x0000 /* no prescalar */
        #define SYMBOL_TO_TICK_RATE 125000
    #elif(CLOCK_FREQ <= 1000000)
        #define CLOCK_DIVIDER 8
        #define CLOCK_DIVIDER_SETTING 0x0010
        #define SYMBOL_TO_TICK_RATE 1000000
    #elif(CLOCK_FREQ <= 8000000)
        #define CLOCK_DIVIDER 64
        #define CLOCK_DIVIDER_SETTING 0x0020
        #define SYMBOL_TO_TICK_RATE 8000000
    #else
        #define CLOCK_DIVIDER 256
        #define CLOCK_DIVIDER_SETTING 0x0030
        #define SYMBOL_TO_TICK_RATE 32000000
    #endif

    #define ONE_SECOND (((DWORD)CLOCK_FREQ/1000 * 62500) / ((DWORD)SYMBOL_TO_TICK_RATE / 1000))
    /* SYMBOLS_TO_TICKS to only be used with input (a) as a constant, otherwise you will blow up the code */
    #define SYMBOLS_TO_TICKS(a) (((DWORD)CLOCK_FREQ/10000 * a ) / ((DWORD)SYMBOL_TO_TICK_RATE / 10000))
    #define TICKS_TO_SYMBOLS(a) (((DWORD)SYMBOL_TO_TICK_RATE/10000) * a / ((DWORD)CLOCK_FREQ/10000))
#elif defined(__PIC32MX__)
    /* this section is based on the Timer 2/3 module of the PIC32MX family */
    #define INSTR_FREQ  (CLOCK_FREQ/4)
    #if(INSTR_FREQ <= 125000)
        #define CLOCK_DIVIDER 1
        #define CLOCK_DIVIDER_SETTING 0x0000 /* no prescalar */
        #define SYMBOL_TO_TICK_RATE 125000
    #elif(INSTR_FREQ <= 1000000)
        #define CLOCK_DIVIDER 8
        #define CLOCK_DIVIDER_SETTING 0x0030
        #define SYMBOL_TO_TICK_RATE 1000000
    #elif(INSTR_FREQ <= 8000000)
        #define CLOCK_DIVIDER 64
        #define CLOCK_DIVIDER_SETTING 0x0060
        #define SYMBOL_TO_TICK_RATE 8000000
    #elif(INSTR_FREQ <= 16000000)
        #define CLOCK_DIVIDER 256
        #define CLOCK_DIVIDER_SETTING 0x0070
        #define SYMBOL_TO_TICK_RATE INSTR_FREQ
    #else
        #define CLOCK_DIVIDER 256
        #define CLOCK_DIVIDER_SETTING 0x70
        #define SYMBOL_TO_TICK_RATE INSTR_FREQ
    #endif

    #define ONE_SECOND (((DWORD)INSTR_FREQ/1000 * 62500) / (SYMBOL_TO_TICK_RATE / 1000))
    /* SYMBOLS_TO_TICKS to only be used with input (a) as a constant, otherwise you will blow up the code */
    #define SYMBOLS_TO_TICKS(a) (((DWORD)(INSTR_FREQ/100000) * a) / (SYMBOL_TO_TICK_RATE / 100000))
    #define TICKS_TO_SYMBOLS(a) (((DWORD)SYMBOL_TO_TICK_RATE/100000) * a / ((DWORD)CLOCK_FREQ/100000))
#else
    //#error "Unsupported processor.  New timing definitions required for proper operation"
#endif

#define HUNDRED_MICRO_SECOND 6                      // 6 ticks is 96us - approx 100us as close as possible with 16uS resolution
#define ONE_SECOND          62500                   // Ticks per second at 16uS per tick
#define TWO_SECOND          (ONE_SECOND*2)
#define FIVE_SECOND         (ONE_SECOND*5)
#define TEN_SECOND          (ONE_SECOND*10)
#define HALF_SECOND         (ONE_SECOND/2)
#define HALF_MILLI_SECOND   (ONE_SECOND/2000)
#define ONE_MILI_SECOND     (ONE_SECOND/1000)
#define HUNDRED_MILI_SECOND (ONE_SECOND/10)
#define FORTY_MILI_SECOND   (ONE_SECOND/25)
#define TWENTY_MILI_SECOND  (ONE_SECOND/50)
#define TEN_MILI_SECOND     (ONE_SECOND/100)
#define FIVE_MILI_SECOND    (ONE_SECOND/200)
#define TWO_MILI_SECOND     (ONE_SECOND/500)
#define ONE_MINUTE          (ONE_SECOND*60)
#define ONE_HOUR            (ONE_MINUTE*60)

#define tickGetDiff(a,b) (a.val - b.val)
#define tickTimeSince(t)    (tickGet() - t.val)

/************************ DATA TYPES *******************************/


/******************************************************************
 // Time unit defined based on IEEE 802.15.4 specification.
 // One tick is equal to one symbol time, or 16us. The Tick structure
 // is four bytes in length and is capable of represent time up to
 // about 19 hours.
 *****************************************************************/
typedef union _TickValue
{
    uint32_t val;
    struct TickBytes
    {
        uint8_t b0;
        uint8_t b1;
        uint8_t b2;
        uint8_t b3;
    } byte;
    uint8_t v[4];
    struct TickWords
    {
        uint16_t w0;
        uint16_t w1;
    } word;
} TickValue;


// Global routine definitions

void initTicker(uint8_t priority);
uint32_t tickGet(void);
void tickISR(void);

/************************ VARIABLES ********************************/

extern volatile uint8_t timerExtension1,timerExtension2;



#endif