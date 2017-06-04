#include "m_pd.h"

static t_class *pdfirmata_class;

typedef struct _pdfirmata{
    t_object x_obj;
    t_int    buffer[128];
    t_int    rawCounter;
    t_int    rawType; //rawType 0 nothing, 1 sysex
    t_atom   abuffer[128];
    t_inlet  *raw;
    t_outlet *rawOut, *decOut;
}t_pdfirmata;

// function protoypes

void clearBuffer(t_pdfirmata *x);
void writeBuffer(t_pdfirmata *x, uint8_t bytec);
t_int serialPort(t_atom *serialPort);
int stringSize(char *stringSize);
void pinMode(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv);
void digitalWrite(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv);
void analogWrite(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv);
void analogIn(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv);
void pinRead(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv);
void serialConfig(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv);
void serialPrint(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv);
void serialPrintln(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv);
void serialRead(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv);
void serialClose(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv);
void serialFlush(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv);
void serialListen(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv);
void I2CRW(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv);
void I2Cconfig(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv);
void servoConfig(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv);
void servoWrite(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv);
void encoderAttach(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv);
void encoderDetach(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv);
void encoderReset(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv);
void encoderRead(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv);
void encoderReadall(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv);
void encoderReport(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv);
void stepperConfig(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv);
void stepperStep(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv);
void decSysex(t_pdfirmata *x);

// function prototypes

void pdfirmata_onBangMsg(t_pdfirmata *x){
    post("Merhaba dunya");
    postfloat(x->buffer[0]);
}

void pdfirmata_onList(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv){
    if(atom_getsymbol(argv) == gensym("pinMode")){
        pinMode(x, s, argc, argv);
    }
    else if(atom_getsymbol(argv) == gensym("digitalWrite")){
        digitalWrite(x, s, argc, argv);
    }
    else if(atom_getsymbol(argv) == gensym("analogWrite")){
        analogWrite(x, s, argc, argv);
    }
    else if(atom_getsymbol(argv) == gensym("analogRead")){
        analogIn(x, s, argc, argv);
    }
    else if(atom_getsymbol(argv) == gensym("protocolVersion")){
        outlet_float(x->rawOut, 0xF9);
    }
    else if(atom_getsymbol(argv) == gensym("pinRead")){
        pinRead(x, s, argc, argv);
    }
    else if(atom_getsymbol(argv) == gensym("Serial")){
        if(atom_getsymbol(argv+1) == gensym("config")){
            serialConfig(x, s, argc, argv);
        }
        else if(atom_getsymbol(argv+1) == gensym("print")){
            serialPrint(x, s, argc, argv);
        }
        else if(atom_getsymbol(argv+1) == gensym("println")){
            serialPrintln(x, s, argc, argv);
        }
        else if(atom_getsymbol(argv+1) == gensym("read")){
            serialRead(x, s, argc, argv);
        }
        else if(atom_getsymbol(argv+1) == gensym("close")){
            serialClose(x, s, argc, argv);
        }
        else if(atom_getsymbol(argv+1) == gensym("flush")){
            serialFlush(x, s, argc, argv);
        }
        else if(atom_getsymbol(argv+1) == gensym("listen")){
            serialListen(x, s, argc, argv);
        }
    }
    else if(atom_getsymbol(argv) == gensym("I2C")){
        if(atom_getsymbol(argv+1) == gensym("RW")){
            I2CRW(x, s, argc, argv);
        }
        else if(atom_getsymbol(argv+1) == gensym("config")){
            I2Cconfig(x, s, argc, argv);
        }
    }
    else if(atom_getsymbol(argv) == gensym("Servo")){
        if(atom_getsymbol(argv+1) == gensym("write")){
            servoWrite(x, s, argc, argv);
        }
        else if(atom_getsymbol(argv+1) == gensym("config")){
            servoConfig(x, s, argc, argv);
        }
    }
    else if(atom_getsymbol(argv) == gensym("Encoder")){
        if(atom_getsymbol(argv+1) == gensym("attach")){
            encoderAttach(x, s, argc, argv); // Servo write is exactly same as analogWrite
        }
        else if(atom_getsymbol(argv+1) == gensym("detach")){
            encoderDetach(x, s, argc, argv);
        }
        else if(atom_getsymbol(argv+1) == gensym("reset")){
            encoderReset(x, s, argc, argv);
        }
        else if(atom_getsymbol(argv+1) == gensym("read")){
            encoderRead(x, s, argc, argv);
        }
        else if(atom_getsymbol(argv+1) == gensym("readall")){
            encoderReadall(x, s, argc, argv);
        }
        else if(atom_getsymbol(argv+1) == gensym("report")){
            encoderReport(x, s, argc, argv);
        }
    }
    else if(atom_getsymbol(argv) == gensym("Stepper")){
        if(atom_getsymbol(argv+1) == gensym("config")){
            stepperConfig(x, s, argc, argv); // Servo write is exactly same as analogWrite
        }
        else if(atom_getsymbol(argv+1) == gensym("step")){
            stepperStep(x, s, argc, argv);
        }
    }
}

void pdfirmata_onRawData(t_pdfirmata *x, t_floatarg f){
    if(f == 0xF7){ //sysex end
        x->rawType = 0;
        decSysex(x);
    }
    if(x->rawType == 2){ //analog read buffer and output
        x->buffer[x->rawCounter] = f;
        x->rawCounter++;
        if(x->rawCounter == 3){
            SETSYMBOL(&x->abuffer[0], gensym("Analog"));
            SETFLOAT(&x->abuffer[1], x->buffer[0] - 0xE0);
            SETFLOAT(&x->abuffer[2], x->buffer[1] + (x->buffer[2] * 128));
            outlet_list(x->decOut, &s_list, 3, x->abuffer);
            x->rawType = 0;
        }
    }
    else if(x->rawType == 1){ //sysex buffer
        x->buffer[x->rawCounter] = f;
        x->rawCounter++;
    }
    else if(x->rawType == 0){ //end
        x->rawCounter = 0;
    }
    if(f == 0xF0){ //sysex begin
        x->rawType = 1;
    }
    else if((f >= 224) && (f <= 239)){ //analog read
        x->buffer[x->rawCounter] = f;
        x->rawCounter++;
        x->rawType = 2;
    }
}

void *pdfirmata_new(){
    t_pdfirmata *x = (t_pdfirmata *)pd_new(pdfirmata_class);

    x->raw = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("rawdata"));

    x->rawOut = outlet_new(&x->x_obj, &s_float);
    x->decOut = outlet_new(&x->x_obj, &s_list);

    return (void *)x;
}

void pdfirmata_free(t_pdfirmata *x){
    inlet_free(x->raw);
    outlet_free(x->rawOut);
    outlet_free(x->decOut);
}

void pdfirmata_setup(void){
    pdfirmata_class = class_new(gensym("pdfirmata"),
                                (t_newmethod)pdfirmata_new,
                                (t_method)pdfirmata_free,
                                sizeof(t_pdfirmata),
                                CLASS_DEFAULT,
                                0);

    class_addlist(pdfirmata_class, (t_method)pdfirmata_onList);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_onRawData, gensym("rawdata"), A_DEFFLOAT, 0);
}

void clearBuffer(t_pdfirmata *x){
    uint8_t i = 0;
    while(i < 128){
        x->buffer[i] = 0;
        i++;
    }
}

void writeBuffer(t_pdfirmata *x, uint8_t bytec){
    uint8_t i = 0;
    while(i < bytec){
        outlet_float(x->rawOut, x->buffer[i]);
        i++;
    }
}

t_int serialPort(t_atom *serialPort){
    if(atom_getsymbol(serialPort) == gensym("HW0")) return 0;
    else if(atom_getsymbol(serialPort) == gensym("HW1")) return 1;
    else if(atom_getsymbol(serialPort) == gensym("HW2")) return 2;
    else if(atom_getsymbol(serialPort) == gensym("HW3")) return 3;
    else if(atom_getsymbol(serialPort) == gensym("HW4")) return 4;
    else if(atom_getsymbol(serialPort) == gensym("HW5")) return 5;
    else if(atom_getsymbol(serialPort) == gensym("HW6")) return 6;
    else if(atom_getsymbol(serialPort) == gensym("HW7")) return 7;
    else if(atom_getsymbol(serialPort) == gensym("SW0")) return 8;
    else if(atom_getsymbol(serialPort) == gensym("SW1")) return 9;
    else if(atom_getsymbol(serialPort) == gensym("SW2")) return 10;
    else if(atom_getsymbol(serialPort) == gensym("SW3")) return 11;
    else if(atom_getsymbol(serialPort) == gensym("SW4")) return 12;
    else if(atom_getsymbol(serialPort) == gensym("SW5")) return 13;
    else if(atom_getsymbol(serialPort) == gensym("SW6")) return 14;
    else if(atom_getsymbol(serialPort) == gensym("SW7")) return 15;
    else return 0;
}

int stringSize(char *stringSize){
    uint8_t i = 0;
    while(stringSize[i] != 0){
        i++;
    }
    return i;
}

void pinMode(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv){
    x->buffer[0] = 0xF4;
    x->buffer[1] = atom_getfloat(argv+1);
    if (atom_getsymbol(argv+2) == gensym("INPUT")) x->buffer[2] = 0;
    else if (atom_getsymbol(argv+2) == gensym("OUTPUT")) x->buffer[2] = 1;
    else if (atom_getsymbol(argv+2) == gensym("ANALOG")) x->buffer[2] = 2;
    else if (atom_getsymbol(argv+2) == gensym("PWM")) x->buffer[2] = 3;
    else if (atom_getsymbol(argv+2) == gensym("SERVO")) x->buffer[2] = 4;
    else if (atom_getsymbol(argv+2) == gensym("SHIFT")) x->buffer[2] = 5;
    else if (atom_getsymbol(argv+2) == gensym("I2C")) x->buffer[2] = 6;
    else if (atom_getsymbol(argv+2) == gensym("ONEWIRE")) x->buffer[2] = 7;
    else if (atom_getsymbol(argv+2) == gensym("STEPPER")) x->buffer[2] = 8;
    else if (atom_getsymbol(argv+2) == gensym("ENCODER")) x->buffer[2] = 9;
    else if (atom_getsymbol(argv+2) == gensym("SERIAL")) x->buffer[2] = 10;
    else if (atom_getsymbol(argv+2) == gensym("PULLUP")) x->buffer[2] = 11;
    else {
        error("Unknown Pin Mode\nModes:\nINPUT\nOUTPUT\nANALOG\nPWM\nSERVO\nSHIFT\nI2C\nONEWIRE\nSTEPPER\nENCODER\nSERIAL\nPULLUP");
        return;
    }
    writeBuffer(x, 3);
}

void digitalWrite(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv){
    x->buffer[0] = 0xF5;
    x->buffer[1] = atom_getfloat(argv+1);
    if (atom_getfloat(argv+2) == 0) x->buffer[2] = 0;
    else if (atom_getfloat(argv+2) == 1) x->buffer[2] = 1;
    else {
        error("Only 1 and 0 allowed.");
        return;
    }
    writeBuffer(x, 3);
}

void analogWrite(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv){
    if ((atom_getfloat(argv+1) < 16) && (atom_getfloat(argv+2) < 16383)){
        x->buffer[0] = 0xE0 + atom_getfloat(argv+1);
        x->buffer[1] = (int)atom_getfloat(argv+2) & 127;
        x->buffer[2] = (int)atom_getfloat(argv+2) >> 7;
        writeBuffer(x, 3);
        return;
    }
    else{
        x->buffer[0] = 0xF0;
        x->buffer[1] = 0x6F;
        x->buffer[2] = atom_getfloat(argv+1);
        if(atom_getfloat(argv+2) < 16384){
            x->buffer[3] = (int)atom_getfloat(argv+2) & 127;
            x->buffer[4] = (int)atom_getfloat(argv+2) >> 7;
            x->buffer[5] = 0xF7;
            writeBuffer(x, 6);
            return;
        }
        else{
            x->buffer[3] = (int)atom_getfloat(argv+2) & 127;
            x->buffer[4] = ((int)atom_getfloat(argv+2) >> 7) & 127;
            x->buffer[5] = ((int)atom_getfloat(argv+2) >> 14) & 127;
            x->buffer[6] = ((int)atom_getfloat(argv+2) >> 21) & 127;
            x->buffer[7] = 0xF7;
            writeBuffer(x, 8);
            return;
        }
    }
}

void analogIn(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv){
    if (atom_getfloat(argv+1) > 15) {
        error("Max 16 analog input");
        return;
    }
    x->buffer[0] = 0xC0 + atom_getfloat(argv+1);
    if (atom_getfloat(argv+2) == 0) x->buffer[1] = 0;
    else if (atom_getfloat(argv+2) == 1) x->buffer[1] = 1;
    else {
        error("Only 1 and 0 allowed.");
        return;
    }
    writeBuffer(x, 2);
}

void pinRead(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv){
    x->buffer[0] = 0xF0;
    x->buffer[1] = 0x6D;
    x->buffer[2] = atom_getfloat(argv+1);
    x->buffer[3] = 0xF7;
    writeBuffer(x, 4);
}

void serialConfig(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv){
    x->buffer[0] = 0xF0;
    x->buffer[1] = 0x60;
    x->buffer[2] = 0x10 | serialPort(argv+2);
    if(atom_getfloat(argv+3) <= 127){
        x->buffer[3] = atom_getfloat(argv+3);
        x->buffer[4] = atom_getfloat(argv+4);
        x->buffer[5] = atom_getfloat(argv+5);
        x->buffer[6] = 0xF7;
        writeBuffer(x, 7);
        return;
    }
    else if(atom_getfloat(argv+3) <= 16383){
        x->buffer[3] = (int)atom_getfloat(argv+3) & 127;
        x->buffer[4] = (int)atom_getfloat(argv+3) >> 7;
        x->buffer[5] = atom_getfloat(argv+4);
        x->buffer[6] = atom_getfloat(argv+5);
        x->buffer[7] = 0xF7;
        writeBuffer(x, 8);
        return;
    }
    else{
        x->buffer[3] = (int)atom_getfloat(argv+3) & 127;
        x->buffer[4] = ((int)atom_getfloat(argv+3) >> 7) & 127;
        x->buffer[5] = ((int)atom_getfloat(argv+3) >> 14) & 127;
        x->buffer[6] = atom_getfloat(argv+4);
        x->buffer[7] = atom_getfloat(argv+5);
        x->buffer[8] = 0xF7;
        writeBuffer(x, 9);
        return;
    }
}

void serialPrint(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv){
    uint8_t counter = 0;
    uint8_t i = 0;
    char stringBuf[64] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    atom_string(argv+3, stringBuf, 64);
    while(i < (stringSize(stringBuf) / 5) + 1){
        x->buffer[0] = 0xF0;
        x->buffer[1] = 0x60;
        x->buffer[2] = 0x20 | serialPort(argv+2);
        uint8_t j = counter + 2;
        while(j < 7 + counter){
            x->buffer[((j - counter) * 2) - 1] = stringBuf[j - 2] & 127;
            x->buffer[(j - counter) * 2] = stringBuf[j - 2] >> 7;
            j++;
        }
        x->buffer[13] = 0xF7;
        writeBuffer(x, 14);
        counter = counter + 5;
        i++;
    }
}

void serialPrintln(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv){
    uint8_t counter = 0;
    uint8_t i = 0;
    char stringBuf[64] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    atom_string(argv+3, stringBuf, 64);
    while(i < (stringSize(stringBuf) / 5) + 1){
        x->buffer[0] = 0xF0;
        x->buffer[1] = 0x60;
        x->buffer[2] = 0x20 | serialPort(argv+2);
        uint8_t j = counter + 2;
        while(j < 7 + counter){
            x->buffer[((j - counter) * 2) - 1] = stringBuf[j - 2] & 127;
            x->buffer[(j - counter) * 2] = stringBuf[j - 2] >> 7;
            j++;
        }
        x->buffer[13] = 0xF7;
        writeBuffer(x, 14);
        counter = counter + 5;
        i++;
    }
    x->buffer[3] = 0xA;
    x->buffer[4] = 0;
    x->buffer[5] = 0xD;
    x->buffer[6] = 0;
    x->buffer[7] = 0xF7;
    writeBuffer(x, 8);
}

void serialRead(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv){
    x->buffer[0] = 0xF0;
    x->buffer[1] = 0x60;
    x->buffer[2] = 0x30 | serialPort(argv+2);
    if(atom_getfloat(argv+3) == 1) x->buffer[3] = 1;
    else if(atom_getfloat(argv+3) == 0) x->buffer[3] = 0;
    else {
        error("Only 1 and 0 allowed.");
        return;
    }
    x->buffer[4] = (int)atom_getfloat(argv+4) & 127;
    x->buffer[5] = (int)atom_getfloat(argv+4) >> 7;
    x->buffer[6] = 0xF7;
    writeBuffer(x, 7);
}

void serialClose(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv){
    x->buffer[0] = 0xF0;
    x->buffer[1] = 0x60;
    x->buffer[2] = 0x50 | serialPort(argv+2);
    x->buffer[3] = 0xF7;
    writeBuffer(x, 4);
}

void serialFlush(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv){
    x->buffer[0] = 0xF0;
    x->buffer[1] = 0x60;
    x->buffer[2] = 0x60 | serialPort(argv+2);
    x->buffer[3] = 0xF7;
    writeBuffer(x, 4);
}

void serialListen(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv){
    x->buffer[0] = 0xF0;
    x->buffer[1] = 0x60;
    x->buffer[2] = 0x70 | serialPort(argv+2);
    x->buffer[3] = 0xF7;
    writeBuffer(x, 4);
}

void I2CRW(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv){
    uint8_t MSB = 0, LSB = 0;
    LSB = (int)atom_getfloat(argv+2) & 127;
    if(atom_getsymbol(argv+3) == gensym("AR1")) MSB = MSB | 64; //bit-6 Auto Restart Function (0 = stop, 1 = Restart)
    if((int)atom_getfloat(argv+2) > 127) MSB = MSB | 32; //bit-5 Address Mode (0 = 7-bit, 1 = 10-bit)
	if(atom_getsymbol(argv+4) == gensym("RO")) MSB = MSB | 8;
	if(atom_getsymbol(argv+4) == gensym("RC")) MSB = MSB | 4; //bits 4-3 read/write, 00 = write, 01 = read once, 10 = read continuously, 11 = stop reading
	if(atom_getsymbol(argv+4) == gensym("RS")) MSB = MSB | 24;
	if((int)atom_getfloat(argv+2) > 127) MSB = MSB | (int)atom_getfloat(argv+2) >> 7; // bits 2-0 10-bit address bits
	x->buffer[0] = 0xF0;
    x->buffer[1] = 0x76;
    x->buffer[2] = LSB;
    x->buffer[3] = MSB;
    uint8_t i = 2;
    while(i < argc){
        x->buffer[i * 2] = (int)atom_getfloat(argv + (i + 3)) & 127;
        x->buffer[(i * 2) + 1] = (int)atom_getfloat(argv + (i + 3)) >> 7;
        i++;
    }
    x->buffer[((argc - 5) * 2) + 4] = 0xF7;
    writeBuffer(x, ((argc - 5) * 2) + 5);
}

void I2Cconfig(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv){
    x->buffer[0] = 0xF0;
    x->buffer[1] = 0x78;
    x->buffer[2] = (int)atom_getfloat(argv+2) & 127;
    x->buffer[3] = (int)atom_getfloat(argv+2) >> 7;
    x->buffer[4] = 0xF7;
    writeBuffer(x, 5);
}

void servoConfig(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv){
    x->buffer[0] = 0xF0;
    x->buffer[1] = 0x70;
    x->buffer[2] = atom_getfloat(argv+2);
    x->buffer[3] = (int)atom_getfloat(argv+3) & 127;
    x->buffer[4] = (int)atom_getfloat(argv+3) >> 7;
    x->buffer[5] = (int)atom_getfloat(argv+4) & 127;
    x->buffer[6] = (int)atom_getfloat(argv+4) >> 7;
    x->buffer[7] = 0xF7;
    writeBuffer(x, 8);
}

void servoWrite(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv){
    if ((atom_getfloat(argv+2) < 16) && (atom_getfloat(argv+3) < 16383)){
        x->buffer[0] = 0xE0 + atom_getfloat(argv+2);
        x->buffer[1] = (int)atom_getfloat(argv+3) & 127;
        x->buffer[2] = (int)atom_getfloat(argv+3) >> 7;
        writeBuffer(x, 3);
        return;
    }
    else{
        x->buffer[0] = 0xF0;
        x->buffer[1] = 0x6F;
        x->buffer[2] = atom_getfloat(argv+2);
        if(atom_getfloat(argv+3) < 16384){
            x->buffer[3] = (int)atom_getfloat(argv+3) & 127;
            x->buffer[4] = (int)atom_getfloat(argv+3) >> 7;
            x->buffer[5] = 0xF7;
            writeBuffer(x, 6);
            return;
        }
        else{
            x->buffer[3] = (int)atom_getfloat(argv+3) & 127;
            x->buffer[4] = ((int)atom_getfloat(argv+3) >> 7) & 127;
            x->buffer[5] = ((int)atom_getfloat(argv+3) >> 14) & 127;
            x->buffer[6] = ((int)atom_getfloat(argv+3) >> 21) & 127;
            x->buffer[7] = 0xF7;
            writeBuffer(x, 8);
            return;
        }
    }
}

void encoderAttach(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv){
    if(atom_getfloat(argv+2) > 4){
        error("Max 5 Encoder Allowed (0-4)");
        return;
    }
    x->buffer[0] = 0xF0;
    x->buffer[1] = 0x61;
    x->buffer[2] = 0x00;
    x->buffer[3] = atom_getfloat(argv+2);
    x->buffer[4] = atom_getfloat(argv+3);
    x->buffer[5] = atom_getfloat(argv+4);
    x->buffer[6] = 0xF7;
    writeBuffer(x, 7);
}

void encoderDetach(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv){
    if(atom_getfloat(argv+2) > 4){
        error("Max 5 Encoder Allowed (0-4)");
        return;
    }
    x->buffer[0] = 0xF0;
    x->buffer[1] = 0x61;
    x->buffer[2] = 0x05;
    x->buffer[3] = atom_getfloat(argv+2);
    x->buffer[4] = 0xF7;
    writeBuffer(x, 5);
}

void encoderReset(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv){
    if(atom_getfloat(argv+2) > 4){
        error("Max 5 Encoder Allowed (0-4)");
        return;
    }
    x->buffer[0] = 0xF0;
    x->buffer[1] = 0x61;
    x->buffer[2] = 0x03;
    x->buffer[3] = atom_getfloat(argv+2);
    x->buffer[4] = 0xF7;
    writeBuffer(x, 5);
}

void encoderRead(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv){
    if(atom_getfloat(argv+2) > 4){
        error("Max 5 Encoder Allowed (0-4)");
        return;
    }
    x->buffer[0] = 0xF0;
    x->buffer[1] = 0x61;
    x->buffer[2] = 0x01;
    x->buffer[3] = atom_getfloat(argv+2);
    x->buffer[4] = 0xF7;
    writeBuffer(x, 5);
}

void encoderReadall(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv){
    if(atom_getfloat(argv+2) > 4){
        error("Max 5 Encoder Allowed (0-4)");
        return;
    }
    x->buffer[0] = 0xF0;
    x->buffer[1] = 0x61;
    x->buffer[2] = 0x02;
    x->buffer[3] = 0xF7;
    writeBuffer(x, 4);
}

void encoderReport(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv){
    if(atom_getfloat(argv+2) > 4){
        error("Max 5 Encoder Allowed (0-4)");
        return;
    }
    x->buffer[0] = 0xF0;
    x->buffer[1] = 0x61;
    x->buffer[2] = 0x04;
    x->buffer[3] = atom_getfloat(argv+2);
    x->buffer[4] = 0xF7;
    writeBuffer(x, 5);
}

void stepperConfig(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv){
    if(atom_getfloat(argv+2) > 5){
        error("Max 6 Stepper Allowed (0-5)");
        return;
    }
    x->buffer[0] = 0xF0;
    x->buffer[1] = 0x72;
    x->buffer[2] = 0x00;
    x->buffer[3] = atom_getfloat(argv+2);
    if(atom_getfloat(argv+3) == 1) x->buffer[4] = 0;
    else if (atom_getfloat(argv+3) == 2) x->buffer[4] = 8;
    if(atom_getfloat(argv+4) == 1) x->buffer[4] = x->buffer[4] | 1;
    else if(atom_getfloat(argv+4) == 2) x->buffer[4] = x->buffer[4] | 2;
    else if(atom_getfloat(argv+4) == 4) x->buffer[4] = x->buffer[4] | 4;
    x->buffer[5] = (int)atom_getfloat(argv+5) & 127;
    x->buffer[6] = (int)atom_getfloat(argv+5) >> 7;
    x->buffer[7] = atom_getfloat(argv+6);
    x->buffer[8] = atom_getfloat(argv+7);
    if(atom_getfloat(argv+4) > 3){
        x->buffer[9] = atom_getfloat(argv+8);
        x->buffer[10] = atom_getfloat(argv+9);
        x->buffer[11] = 0xF7;
        writeBuffer(x, 12);
        return;
    }
    else{
        x->buffer[9] = 0xF7;
        writeBuffer(x, 10);
        return;
    }
}

void stepperStep(t_pdfirmata *x, t_symbol *s, t_int argc, t_atom *argv){
    if(atom_getfloat(argv+2) > 5){
        error("Max 6 Stepper Allowed (0-5)");
        return;
    }
    x->buffer[0] = 0xF0;
	x->buffer[1] = 0x72;
	x->buffer[2] = 0x01;
	x->buffer[3] = atom_getfloat(argv+2);
	x->buffer[4] = atom_getfloat(argv+3);
	x->buffer[5] = (int)atom_getfloat(argv+4) & 127;
	x->buffer[6] = ((int)atom_getfloat(argv+4) >> 7) & 127;
	x->buffer[7] = ((int)atom_getfloat(argv+4) >> 14) & 127;
	x->buffer[8] = (int)atom_getfloat(argv+5) & 127;
	x->buffer[9] = ((int)atom_getfloat(argv+5) >> 7) & 127;
	if (argc > 6){
		x->buffer[10] = (int)atom_getfloat(argv+6) & 127;
		x->buffer[11] = (int)atom_getfloat(argv+6) >> 7;
		x->buffer[12] = (int)atom_getfloat(argv+7) & 127;
		x->buffer[13] = (int)atom_getfloat(argv+7) >> 7;
		x->buffer[14] = 0xF7;
		writeBuffer(x, 15);
		return;
	}
	else{
		x->buffer[10] = 0xF7;
		writeBuffer(x, 11);
	}
}

void decSysex(t_pdfirmata *x){
    if(x->buffer[0] == 0x6E){
        SETSYMBOL(&x->abuffer[0], gensym("Pin"));
        SETFLOAT(&x->abuffer[1], x->buffer[1]);
        if(x->buffer[2] == 0) SETSYMBOL(&x->abuffer[2], gensym("INPUT"));
        else if(x->buffer[2] == 1) SETSYMBOL(&x->abuffer[2], gensym("OUTPUT"));
        else if(x->buffer[2] == 2) SETSYMBOL(&x->abuffer[2], gensym("ANALOG"));
        else if(x->buffer[2] == 3) SETSYMBOL(&x->abuffer[2], gensym("PWM"));
        else if(x->buffer[2] == 4) SETSYMBOL(&x->abuffer[2], gensym("SERVO"));
        else if(x->buffer[2] == 5) SETSYMBOL(&x->abuffer[2], gensym("SHIFT"));
        else if(x->buffer[2] == 6) SETSYMBOL(&x->abuffer[2], gensym("I2C"));
        else if(x->buffer[2] == 7) SETSYMBOL(&x->abuffer[2], gensym("ONEWIRE"));
        else if(x->buffer[2] == 8) SETSYMBOL(&x->abuffer[2], gensym("STEPPER"));
        else if(x->buffer[2] == 9) SETSYMBOL(&x->abuffer[2], gensym("ENCODER"));
        else if(x->buffer[2] == 10) SETSYMBOL(&x->abuffer[2], gensym("SERIAL"));
        else if(x->buffer[2] == 11) SETSYMBOL(&x->abuffer[2], gensym("PULLUP"));
        if(x->rawCounter == 4) SETFLOAT(&x->abuffer[3], x->buffer[3]);
        if(x->rawCounter == 5) SETFLOAT(&x->abuffer[3], x->buffer[3] + (x->buffer[4] * 128));
        if(x->rawCounter == 6) SETFLOAT(&x->abuffer[3], x->buffer[3] + (x->buffer[4] * 128) + (x->buffer[5] * 16384));
        if(x->rawCounter == 7) SETFLOAT(&x->abuffer[3], x->buffer[3] + (x->buffer[4] * 128) + (x->buffer[5] * 16384) + (x->buffer[6] * 2097152));
        outlet_list(x->decOut, &s_list, 4, x->abuffer);
    }
    else if(x->buffer[0] == 0x60){
        SETSYMBOL(&x->abuffer[0], gensym("Serial"));
        if(x->buffer[1] == 0x40) SETSYMBOL(&x->abuffer[1], gensym("HW0"));
        else if(x->buffer[1] == 0x41) SETSYMBOL(&x->abuffer[1], gensym("HW1"));
        else if(x->buffer[1] == 0x42) SETSYMBOL(&x->abuffer[1], gensym("HW2"));
        else if(x->buffer[1] == 0x43) SETSYMBOL(&x->abuffer[1], gensym("HW3"));
        else if(x->buffer[1] == 0x44) SETSYMBOL(&x->abuffer[1], gensym("HW4"));
        else if(x->buffer[1] == 0x45) SETSYMBOL(&x->abuffer[1], gensym("HW5"));
        else if(x->buffer[1] == 0x46) SETSYMBOL(&x->abuffer[1], gensym("HW6"));
        else if(x->buffer[1] == 0x47) SETSYMBOL(&x->abuffer[1], gensym("HW7"));
        else if(x->buffer[1] == 0x48) SETSYMBOL(&x->abuffer[1], gensym("SW0"));
        else if(x->buffer[1] == 0x49) SETSYMBOL(&x->abuffer[1], gensym("SW1"));
        else if(x->buffer[1] == 0x4A) SETSYMBOL(&x->abuffer[1], gensym("SW2"));
        else if(x->buffer[1] == 0x4B) SETSYMBOL(&x->abuffer[1], gensym("SW3"));
        else if(x->buffer[1] == 0x4C) SETSYMBOL(&x->abuffer[1], gensym("SW4"));
        else if(x->buffer[1] == 0x4D) SETSYMBOL(&x->abuffer[1], gensym("SW5"));
        else if(x->buffer[1] == 0x4E) SETSYMBOL(&x->abuffer[1], gensym("SW6"));
        else if(x->buffer[1] == 0x4F) SETSYMBOL(&x->abuffer[1], gensym("SW7"));
        uint8_t i = 1;
        while(i < ((x->rawCounter - 2) / 2)){
            SETFLOAT(&x->abuffer[i + 1], (x->buffer[i * 2] + (x->buffer[(i * 2) + 1] * 128)));
            i++;
        }
        outlet_list(x->decOut, &s_list, ((x->rawCounter - 2) / 2) + 2, x->abuffer);
    }
    else if (x->buffer[0] == 0x77){
        SETSYMBOL(&x->abuffer[0], gensym("I2C"));
        uint8_t i = 1;
        while(i < ((x->rawCounter - 1) / 2)){
            SETFLOAT(&x->abuffer[i], (x->buffer[(i * 2) - 1] + (x->buffer[i * 2] * 128)));
            i++;
        }
        outlet_list(x->decOut, &s_list, ((x->rawCounter - 1) / 2) + 1, x->abuffer);
    }
    else if (x->buffer[0] == 0x61){
        SETSYMBOL(&x->abuffer[0], gensym("Encoder"));
        uint8_t i = 0;
        while(i < ((x->rawCounter - 1) / 5)){
            SETFLOAT(&x->abuffer[1], x->buffer[(i * 5) + 1] & 63);
            x->buffer[(i * 5) + 2] = x->buffer[(i * 5) + 2] + (x->buffer[(i * 5) + 3] << 7);
            x->buffer[(i * 5) + 2] = x->buffer[(i * 5) + 2] + (x->buffer[(i * 5) + 4] << 14);
            x->buffer[(i * 5) + 2] = x->buffer[(i * 5) + 2] + (x->buffer[(i * 5) + 5] << 21);
            if((x->buffer[(i * 5) + 1] >> 6) == 1) x->buffer[(i * 5) + 2] = x->buffer[(i * 5) + 2] * -1;
            SETFLOAT(&x->abuffer[2], x->buffer[(i * 5) + 2]);
            outlet_list(x->decOut, &s_list, 3, x->abuffer);
            i++;
        }
    }
}
