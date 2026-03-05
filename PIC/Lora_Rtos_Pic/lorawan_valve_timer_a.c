// ============================================================
// slave_A.c  —  CCS RTOS port  —  SLAVE A CHIP
// Target : PIC18F25K22 @ 20 MHz
// ============================================================

#include <18F25K22.h>
#device ADC=10
#INCLUDE <stdlib.h> 

#fuses HSM,PUT,NOPROTECT,NOLVP,NOWDT,NOBROWNOUT,NOPLLEN,NOPBADEN
#fuses IESO
#use delay(xtal=20MHz, clock=20MHz)
#byte clock_source=getenv("clock")

#use rs232(stream=COM_1,baud=9600,parity=N,xmit=PIN_C6,rcv=PIN_C7,bits=8)
#include "internal_eeprom.c"

#use rtos(timer=1, minor_cycle=10ms)

#define V1      PIN_B0
#define V2      PIN_B1
#define V3      PIN_B2
#define V4      PIN_B3
#define V5      PIN_B4
#define V6      PIN_B5
#define V7      PIN_B6
#define V8      PIN_B7
#define SIGLED  PIN_C3

#define EEP_HR  40
#define EEP_MIN 41

#byte RCSTA = 0xFAB
#bit  OERR  = RCSTA.1
#bit  FERR  = RCSTA.2
#bit  CREN  = RCSTA.4

int8 SEM_RX_READY  = 0;
int8 MTX_RX_BUFFER = 1;
int8 MTX_VALVE_ST  = 1;
int8 MTX_UART      = 1;
int8 MTX_CLOCK     = 1;
int8 MTX_EEPROM    = 1;

// ============================================================
// CHANGE 1 OF 2 — THIS CHIP'S ID
// Change this one line only when programming Slave B chip:
//   #define MY_ID  'B'
// ============================================================
#define MY_ID  'A'

#define RXBUF_SIZE 50
volatile char RxBuffer[RXBUF_SIZE];

volatile char state1='0', state2='0', state3='0', state4='0';
volatile char state5='0', state6='0', state7='0', state8='0';

volatile int8 hr=0, min=0, sec=0;
volatile int8 bwminv1=0, bwminv2=0;

int1 vanoflg1=0, vanoflg2=0, vanoflg3=0, vanoflg4=0;
int1 vanoflg5=0, vanoflg6=0, vanoflg7=0, vanoflg8=0;

volatile int1 txflg = 0;

char vno1,vno2,vno3,vno4,vno5,vno6,vno7,vno8;
char TimeBuffer[9];

int8 clock_tick_count = 0;
#define TICKS_PER_SEC  100

#define ST_IDLE      0
#define ST_DOLLAR    1
#define ST_M         2
#define ST_STAR1     3
#define ST_SID       4
#define ST_PAYLOAD   5

volatile int8 isr_state = ST_IDLE;
volatile int8 isr_idx   = 0;

// ============================================================
// ISR — accepts ONLY MY_ID packets, drops everything else
// ============================================================
#int_RDA
void RDA_isr(void)
{
    if (OERR) { CREN = 0; CREN = 1; isr_state = ST_IDLE; return; }
    if (FERR) { getc(COM_1); isr_state = ST_IDLE; return; }

    char b = getc(COM_1);

    switch(isr_state)
    {
        case ST_IDLE:
            if(b == '$') isr_state = ST_DOLLAR;
            break;

        case ST_DOLLAR:
            if     (b == 'M') isr_state = ST_M;
            else if(b == '$') isr_state = ST_DOLLAR;
            else              isr_state = ST_IDLE;
            break;

        case ST_M:
            if(b == '*') isr_state = ST_STAR1;
            else          isr_state = ST_IDLE;
            break;

        case ST_STAR1:
            // -- CHANGE 2 OF 2 ----------------------------
            // Only accept MY_ID — all other slave packets
            // are silently ignored from this point forward.
            // For Slave B chip: MY_ID='B' so 'A' is ignored.
            if(b == MY_ID) isr_state = ST_SID;
            else            isr_state = ST_IDLE;  // not my packet
            break;

        case ST_SID:
            if(b == '*') { isr_idx = 0; isr_state = ST_PAYLOAD; }
            else           isr_state = ST_IDLE;
            break;

        case ST_PAYLOAD:
            if(b == '#')
            {
                RxBuffer[isr_idx] = '\0';
                rtos_signal(SEM_RX_READY);
                isr_state = ST_IDLE;
            }
            else if(b == '$')
            {
                isr_state = ST_DOLLAR;
            }
            else
            {
                if(isr_idx < RXBUF_SIZE - 1)
                    RxBuffer[isr_idx++] = b;
            }
            break;

        default:
            isr_state = ST_IDLE;
            break;
    }
}

// ============================================================
// parse_packet() — unchanged
// ============================================================
void parse_packet(void)
{
    vno1   = RxBuffer[0];
    state1 = RxBuffer[2];

    TimeBuffer[0] = RxBuffer[6];
    TimeBuffer[1] = RxBuffer[7];
    TimeBuffer[2] = RxBuffer[8];
    TimeBuffer[3] = RxBuffer[9];
    TimeBuffer[4] = RxBuffer[10];
    TimeBuffer[5] = RxBuffer[11];
    TimeBuffer[6] = RxBuffer[12];
    TimeBuffer[7] = RxBuffer[13];
    TimeBuffer[8] = '\0';

    vno2 = RxBuffer[15]; state2 = RxBuffer[17];
    vno3 = RxBuffer[19]; state3 = RxBuffer[21];
    vno4 = RxBuffer[23]; state4 = RxBuffer[25];
    vno5 = RxBuffer[27]; state5 = RxBuffer[29];
    vno6 = RxBuffer[31]; state6 = RxBuffer[33];
    vno7 = RxBuffer[35]; state7 = RxBuffer[37];
    vno8 = RxBuffer[39]; state8 = RxBuffer[41];
}

// ============================================================
// TASK 1 — Task_RxProcess
// SlaveID variable removed — this chip only ever sees MY_ID
// ============================================================
#undef max
#task(rate=10ms, max=5ms)
void Task_RxProcess(void)
{
    rtos_wait(SEM_RX_READY);

    rtos_wait(MTX_RX_BUFFER);
    parse_packet();
    rtos_signal(MTX_RX_BUFFER);

    rtos_wait(MTX_UART);
    // MY_ID used directly — no SlaveID variable needed
    fprintf(COM_1, "\r\n[RX] Slave:%c Time:%s\r\n", MY_ID, TimeBuffer);
    fprintf(COM_1, "V1:%c S:%c  V2:%c S:%c  V3:%c S:%c  V4:%c S:%c\r\n",
            vno1,state1, vno2,state2, vno3,state3, vno4,state4);
    fprintf(COM_1, "V5:%c S:%c  V6:%c S:%c  V7:%c S:%c  V8:%c S:%c\r\n",
            vno5,state5, vno6,state6, vno7,state7, vno8,state8);
    rtos_signal(MTX_UART);

    rtos_wait(MTX_VALVE_ST);

    if(vno1 == '1') {
        if(vanoflg1==0 && state1=='1') { output_high(V1); vanoflg1=1; delay_ms(2000); }
        if(vanoflg1==1 && state1=='0') { output_low(V1);  vanoflg1=0; }
    }
    if(vno2 == '2') {
        if(vanoflg2==0 && state2=='1') { output_high(V2); vanoflg2=1; delay_ms(2000); }
        if(vanoflg2==1 && state2=='0') { output_low(V2);  vanoflg2=0; }
    }
    if(vno3 == '3') {
        if(vanoflg3==0 && state3=='1') { output_high(V3); vanoflg3=1; delay_ms(2000); }
        if(vanoflg3==1 && state3=='0') { output_low(V3);  vanoflg3=0; }
    }
    if(vno4 == '4') {
        if(vanoflg4==0 && state4=='1') { output_high(V4); vanoflg4=1; delay_ms(2000); }
        if(vanoflg4==1 && state4=='0') { output_low(V4);  vanoflg4=0; }
    }
    if(vno5 == '5') {
        if(vanoflg5==0 && state5=='1') { output_high(V5); vanoflg5=1; delay_ms(2000); }
        if(vanoflg5==1 && state5=='0') { output_low(V5);  vanoflg5=0; }
    }
    if(vno6 == '6') {
        if(vanoflg6==0 && state6=='1') { output_high(V6); vanoflg6=1; delay_ms(2000); }
        if(vanoflg6==1 && state6=='0') { output_low(V6);  vanoflg6=0; }
    }
    if(vno7 == '7') {
        if(vanoflg7==0 && state7=='1') { output_high(V7); vanoflg7=1; delay_ms(2000); }
        if(vanoflg7==1 && state7=='0') { output_low(V7);  vanoflg7=0; }
    }
    if(vno8 == '8') {
        if(vanoflg8==0 && state8=='1') { output_high(V8); vanoflg8=1; delay_ms(2000); }
        if(vanoflg8==1 && state8=='0') { output_low(V8);  vanoflg8=0; }
    }

    rtos_signal(MTX_VALVE_ST);

    txflg = 1;
    rtos_yield();
}

// ============================================================
// TASK 2 — Task_TxValve
// No SlaveID snapshot needed — MY_ID is a compile-time constant
// ============================================================
#undef max
#task(rate=10ms, max=8ms)
void Task_TxValve(void)
{
    if(txflg == 0)
    {
        rtos_yield();
        return;
    }

    txflg = 0;

    rtos_wait(MTX_VALVE_ST);
    char s1=state1, s2=state2, s3=state3, s4=state4;
    char s5=state5, s6=state6, s7=state7, s8=state8;
    rtos_signal(MTX_VALVE_ST);

    rtos_wait(MTX_UART);
    output_high(SIGLED);
    // MY_ID used directly — no disable_interrupts needed
    fprintf(COM_1, "$%c,1,%c,2,%c,3,%c,4,%c,5,%c,6,%c,7,%c,8,%c#\r\n",
            MY_ID, s1, s2, s3, s4, s5, s6, s7, s8);
    
    rtos_signal(MTX_UART);

    delay_ms(500);
    output_low(SIGLED);
}

// ============================================================
// TASK 3 — Task_TimeClock — unchanged
// ============================================================
#undef max
#task(rate=10ms, max=2ms)
void Task_TimeClock(void)
{
    clock_tick_count++;

    if(clock_tick_count < TICKS_PER_SEC)
    {
        rtos_yield();
        return;
    }

    clock_tick_count = 0;

    rtos_wait(MTX_CLOCK);

    sec++;
    if(sec >= 60)
    {
        sec = 0;
        min++;
        bwminv1++; if(bwminv1 >= 11) bwminv1 = 0;
        bwminv2++; if(bwminv2 >= 11) bwminv2 = 0;

        rtos_wait(MTX_EEPROM);
        write_eeprom(EEP_HR,  hr);
        write_eeprom(EEP_MIN, min);
        rtos_signal(MTX_EEPROM);

        if(min >= 60) { min = 0; hr++; }
        if(hr  >= 24)   hr = 0;
    }

    rtos_signal(MTX_CLOCK);
}

// ============================================================
// MAIN — unchanged
// ============================================================
void main(void)
{
    output_b(0x00);
    set_tris_a(0xFF);
    set_tris_b(0xCF);
    set_tris_c(0xB8);

    output_low(V1); output_low(V2); output_low(V3); output_low(V4);
    output_low(V5); output_low(V6); output_low(V7); output_low(V8);
    output_low(SIGLED);

    hr  = read_eeprom(EEP_HR);
    min = read_eeprom(EEP_MIN);
    sec = 0;
    if(hr  >= 24) hr  = 0;
    if(min >= 60) min = 0;

    enable_interrupts(INT_RDA);
    enable_interrupts(GLOBAL);

    rtos_run();
}
