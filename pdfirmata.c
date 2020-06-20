/* 
	PDFirmata: Firmata client for Pute Data
    Copyright (C) 2017-2020  Malik Enes Safak

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <string.h>
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

void writeBuffer(t_pdfirmata * x, uint8_t * buffer, uint8_t bytec);
uint8_t serialPort(const char * portName);
void pdfirmata_pinMode(t_pdfirmata * x, t_symbol * s, t_int argc, t_atom * argv);
void pdfirmata_digitalWrite(t_pdfirmata * x, t_floatarg pin, t_floatarg state);
void pdfirmata_analogWrite(t_pdfirmata * x, t_floatarg pin, t_floatarg value);
void pdfirmata_analogIn(t_pdfirmata * x, t_floatarg pin, t_floatarg state);
void pdfirmata_pinRead(t_pdfirmata * x, t_floatarg pin);
void pdfirmata_serial(t_pdfirmata * x, t_symbol * s, t_int argc, t_atom * argv);
void pdfirmata_I2C(t_pdfirmata * x, t_symbol * s, t_int argc, t_atom * argv);
void pdfirmata_servo(t_pdfirmata * x, t_symbol * s, t_int argc, t_atom * argv);
void pdfirmata_encoder(t_pdfirmata * x, t_symbol * s, t_int argc, t_atom * argv);
void pdfirmata_stepper(t_pdfirmata * x, t_symbol * s, t_int argc, t_atom * argv);

void decSysex(t_pdfirmata *x);

// !function prototypes

// Constants

const char * pinModes[] = {
    "INPUT",
    "OUTPUT",
    "ANALOG",
    "PWM",
    "SERVO",
    "SHIFT",
    "I2C",
    "ONEWIRE",
    "STEPPER",
    "ENCODER",
    "SERIAL",
    "PULLUP"
};

const uint8_t pinModesLength = sizeof(pinModes) / sizeof(pinModes[0]);

// !Constants

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

    class_addmethod(pdfirmata_class, (t_method)pdfirmata_onRawData, gensym("rawdata"), A_DEFFLOAT, 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_pinMode, gensym("pinMode"), A_GIMME, 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_digitalWrite, gensym("digitalWrite"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_analogWrite, gensym("analogWrite"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_analogIn, gensym("analogIn"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_pinRead, gensym("pinRead"), A_FLOAT, 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_serial, gensym("serial"), A_GIMME, 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_I2C, gensym("I2C"), A_GIMME, 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_servo, gensym("servo"), A_GIMME, 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_encoder, gensym("encoder"), A_GIMME, 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_stepper, gensym("stepper"), A_GIMME, 0);
}

void writeBuffer(t_pdfirmata * x, uint8_t * buffer, uint8_t bytec){
    uint8_t i = 0;
    while(i < bytec){
        outlet_float(x->rawOut, buffer[i]);
        i++;
    }
}

uint8_t serialPort(const char * portName){
    if(strcmp(portName, "HW0") == 0) return 0;
    else if(strcmp(portName, "HW1") == 0) return 1;
    else if(strcmp(portName, "HW2") == 0) return 2;
    else if(strcmp(portName, "HW3") == 0) return 3;
    else if(strcmp(portName, "HW4") == 0) return 4;
    else if(strcmp(portName, "HW5") == 0) return 5;
    else if(strcmp(portName, "HW6") == 0) return 6;
    else if(strcmp(portName, "HW7") == 0) return 7;
    else if(strcmp(portName, "SW0") == 0) return 8;
    else if(strcmp(portName, "SW1") == 0) return 9;
    else if(strcmp(portName, "SW2") == 0) return 10;
    else if(strcmp(portName, "SW3") == 0) return 11;
    else if(strcmp(portName, "SW4") == 0) return 12;
    else if(strcmp(portName, "SW5") == 0) return 13;
    else if(strcmp(portName, "SW6") == 0) return 14;
    else if(strcmp(portName, "SW7") == 0) return 15;
    else return 255;
}

void pdfirmata_pinMode(t_pdfirmata * x, t_symbol * s, t_int argc, t_atom * argv){
    if(argc > 1){
        int _pin = atom_getfloatarg(0, argc, argv);
        if(_pin < 0 || _pin > 127){
            error("Pin number must between 0-127");
            return;
        }
        uint8_t * buffer = (uint8_t *)malloc(3 * sizeof(uint8_t));
        buffer[0] = 0xF4;
        buffer[1] = _pin;
        buffer[2] = 0xFF;
        uint8_t i;
        const char * modeName = atom_getsymbolarg(1, argc, argv)->s_name;
        for(i = 0; i < pinModesLength; i++){
            if(strcmp(modeName, pinModes[i]) == 0){
                buffer[2] = i;
            }
        }
        if(buffer[2] == 0xFF){
            error("Unknown mode");
        }
        else{
            writeBuffer(x, buffer, 3);
        }
        free(buffer);
    }
}

void pdfirmata_digitalWrite(t_pdfirmata * x, t_floatarg pin, t_floatarg state){
    int _pin = pin;
    int _state = state;
    if(_pin < 0 || _pin > 127){
        error("Pin number must between 0-127");
        return;
    }
    if(_state < 0 || _state > 1){
        error("State must 0 (LOW) or 1 (HIGH)");
        return;
    }
    uint8_t * buffer = (uint8_t *)malloc(3 * sizeof(uint8_t));
    buffer[0] = 0xF5;
    buffer[1] = _pin;
    buffer[2] = _state;
    writeBuffer(x, buffer, 3);
    free(buffer);
}

void pdfirmata_analogWrite(t_pdfirmata * x, t_floatarg pin, t_floatarg value){
    int _pin = pin;
    int _value = value;
    if(_pin < 0 || _pin > 127){
        error("Pin number must between 0-127");
        return;
    }
    if(_pin < 16 && _value < 0x4000){
        uint8_t * buffer = (uint8_t *)malloc(3 * sizeof(uint8_t));
        buffer[0] = 0xE0 + _pin;
        buffer[1] = _value & 0x7F;
        buffer[2] = (_value >> 7) & 0x7F;
        writeBuffer(x, buffer, 3);
        free(buffer);
    }
    else{
        uint8_t * buffer = (uint8_t *)malloc(8 * sizeof(uint8_t));
        buffer[0] = 0xF0;
        buffer[1] = 0x6F;
        buffer[2] = _pin;
        buffer[3] = _value & 127;
        buffer[4] = (_value >> 7) & 127;
        buffer[5] = (_value >> 14) & 127;
        buffer[6] = (_value >> 21) & 127;
        buffer[7] = 0xF7;
        writeBuffer(x, buffer, 8);
        free(buffer);
    }
}

void pdfirmata_analogIn(t_pdfirmata * x, t_floatarg pin, t_floatarg state){
    int _pin = pin;
    int _state = state;
    if(_pin < 0 || _pin > 15){
        error("Analog pin number must between 0-15");
        return;
    }
    if(_state < 0 || _state > 1){
        error("State must 0 (DISABLE) or 1 (ENABLE)");
        return;
    }
    uint8_t * buffer = (uint8_t *)malloc(2 * sizeof(uint8_t));
    buffer[0] = 0xC0 + _pin;
    buffer[1] = _state;
    writeBuffer(x, buffer, 2);
    free(buffer);
}

void pdfirmata_pinRead(t_pdfirmata * x, t_floatarg pin){
    int _pin = pin;
    if(_pin < 0 || _pin > 127){
        error("Pin number must between 0-127");
        return;
    }
    uint8_t * buffer = (uint8_t *)malloc(4 * sizeof(uint8_t));
    buffer[0] = 0xF0;
    buffer[1] = 0x6D;
    buffer[2] = _pin;
    buffer[3] = 0xF7;
    writeBuffer(x, buffer, 4);
    free(buffer);
}

void pdfirmata_serial(t_pdfirmata * x, t_symbol * s, t_int argc, t_atom * argv){
    if(argc > 0){
        const char * cmdName = atom_getsymbolarg(0, argc, argv)->s_name;
        /* serial config PORT BAUD (RX) (TX) */
        if(strcmp(cmdName, "config") == 0){
            if(argc > 2){
                uint8_t port = serialPort(atom_getsymbolarg(1, argc, argv)->s_name);
                if(port == 255){
                    error("Unknown serial port name");
                    return;
                }
                uint32_t baud = atom_getint(argv + 2);
                uint8_t rxPin;
                uint8_t txPin;
                if(argc > 4){
                    rxPin = atom_getint(argv + 3);
                    if(rxPin < 0 || rxPin > 127){
                        error("Pin number must between 0-127");
                        return;
                    }
                    txPin = atom_getint(argv + 4);
                    if(txPin < 0 || txPin > 127){
                        error("Pin number must between 0-127");
                        return;
                    }
                    uint8_t * buffer = (uint8_t *)malloc(9 * sizeof(uint8_t));
                    buffer[0] = 0xF0;
                    buffer[1] = 0x60;
                    buffer[2] = 0x10 + port;
                    buffer[3] = baud & 0x7F;
                    buffer[4] = (baud >> 7) & 0x7F;
                    buffer[5] = (baud >> 14) & 0x7F;
                    buffer[6] = rxPin;
                    buffer[7] = txPin;
                    buffer[8] = 0xF7;
                    writeBuffer(x, buffer, 9);
                    free(buffer);
                }
                else{
                    uint8_t * buffer = (uint8_t *)malloc(7 * sizeof(uint8_t));
                    buffer[0] = 0xF0;
                    buffer[1] = 0x60;
                    buffer[2] = 0x10 + port;
                    buffer[3] = baud & 0x7F;
                    buffer[4] = (baud >> 7) & 0x7F;
                    buffer[5] = (baud >> 14) & 0x7F;
                    buffer[6] = 0xF7;
                    writeBuffer(x, buffer, 7);
                    free(buffer);
                }
            }
        }
        /* serial print PORT SYMBOL */
        if(strcmp(cmdName, "print") == 0){
            if(argc > 2){
                uint8_t port = serialPort(atom_getsymbolarg(1, argc, argv)->s_name);
                if(port == 255){
                    error("Unknown serial port name");
                    return;
                }
                const char ** stringNames = (const char **)malloc((argc - 2) * sizeof(const char **));
                uint8_t stringLengths = 0;
                uint8_t i = 2;
                while(i < argc){
                    stringNames[i - 2] = atom_getsymbolarg(i, argc, argv)->s_name;
                    stringLengths += strlen(stringNames[i - 2]);
                    i++;
                }
                char * stringName = (char *)malloc(stringLengths * sizeof(char));
                strcpy(stringName, stringNames[0]);
                i = 1;
                while(i < argc - 2){
                    strcat(stringName, " ");
                    strcat(stringName, stringNames[i]);
                    i++;
                }
                uint8_t stringLength = strlen(stringName);
                uint8_t * buffer = (uint8_t *)malloc(((stringLength * 2) + 4) * sizeof(uint8_t));
                buffer[0] = 0xF0;
                buffer[1] = 0x60;
                buffer[2] = 0x20 | port;
                for(i = 0; i < stringLength; i++){
                    buffer[3 + (i * 2)] = stringName[i] & 0x7F;
                    buffer[3 + ((i * 2) + 1)] = (stringName[i] >> 7) & 0x7F;
                }
                buffer[(stringLength * 2) + 3] = 0xF7;
                writeBuffer(x, buffer, (stringLength * 2) + 4);
                free(buffer);
                free(stringNames);
                free(stringName);
            }
        }
        /* serial println PORT SYMBOL */
        if(strcmp(cmdName, "println") == 0){
            if(argc > 2){
                uint8_t port = serialPort(atom_getsymbolarg(1, argc, argv)->s_name);
                if(port == 255){
                    error("Unknown serial port name");
                    return;
                }
                const char ** stringNames = (const char **)malloc((argc - 2) * sizeof(const char **));
                uint8_t stringLengths = 0;
                uint8_t i = 2;
                while(i < argc){
                    stringNames[i - 2] = atom_getsymbolarg(i, argc, argv)->s_name;
                    stringLengths += strlen(stringNames[i - 2]);
                    i++;
                }
                char * stringName = (char *)malloc(stringLengths * sizeof(char));
                strcpy(stringName, stringNames[0]);
                i = 1;
                while(i < argc - 2){
                    strcat(stringName, " ");
                    strcat(stringName, stringNames[i]);
                    i++;
                }
                uint8_t stringLength = strlen(stringName);
                uint8_t * buffer = (uint8_t *)malloc(((stringLength * 2) + 5) * sizeof(uint8_t));
                buffer[0] = 0xF0;
                buffer[1] = 0x60;
                buffer[2] = 0x20 | port;
                for(i = 0; i < stringLength; i++){
                    buffer[3 + (i * 2)] = stringName[i] & 0x7F;
                    buffer[3 + ((i * 2) + 1)] = (stringName[i] >> 7) & 0x7F;
                }
                buffer[(stringLength * 2) + 3] = '\n';
                buffer[(stringLength * 2) + 4] = 0xF7;
                writeBuffer(x, buffer, (stringLength * 2) + 4);
                free(buffer);
                free(stringNames);
                free(stringName);
            }
        }
        /* serial write PORT ARG0 ARG1 ARG2 ... */
        if(strcmp(cmdName, "write") == 0){
            if(argc > 2){
                uint8_t port = serialPort(atom_getsymbolarg(1, argc, argv)->s_name);
                if(port == 255){
                    error("Unknown serial port name");
                    return;
                }
                uint8_t * buffer = (uint8_t *)malloc((((argc - 2) * 2) + 4) * sizeof(uint8_t));
                buffer[0] = 0xF0;
                buffer[1] = 0x60;
                buffer[2] = 0x20 | port;
                uint8_t i = 2;
                while(i < argc){
                    buffer[((i - 2) * 2) + 3] = atom_getint(argv + i) & 0x7F;
                    buffer[((i - 2) * 2) + 4] = (atom_getint(argv + i) >> 7) & 0x7F;
                    i++;
                }
                buffer[((argc - 2) * 2) + 3] = 0xF7;
                writeBuffer(x, buffer, ((argc - 2) * 2) + 4);
                free(buffer);
            }
        }
        /* serial read PORT READMODE (MAXBYTES) */
        if(strcmp(cmdName, "read") == 0){
            if(argc > 2){
                uint8_t port = serialPort(atom_getsymbolarg(1, argc, argv)->s_name);
                if(port == 255){
                    error("Unknown serial port name");
                    return;
                }
                uint8_t readMode = atom_getint(argv + 2);
                if(readMode < 0 || readMode > 1){
                    error("Read mode must 0 (Continuous) or 1 (Stop after read)");
                    return;
                }
                if(argc > 4){
                    uint32_t maxBytes = atom_getint(argv + 3);
                    uint8_t * buffer = (uint8_t *)malloc(7 * sizeof(uint8_t));
                    buffer[0] = 0xF0;
                    buffer[1] = 0x60;
                    buffer[2] = 0x30 | port;
                    buffer[3] = readMode;
                    buffer[4] = maxBytes & 0x7F;
                    buffer[5] = (maxBytes >> 7) & 0x7F;
                    buffer[6] = 0xF7;
                    writeBuffer(x, buffer, 7);
                    free(buffer);
                }
                else{
                    uint8_t * buffer = (uint8_t *)malloc(5 * sizeof(uint8_t));
                    buffer[0] = 0xF0;
                    buffer[1] = 0x60;
                    buffer[2] = 0x30 | port;
                    buffer[3] = readMode;
                    buffer[4] = 0xF7;
                    writeBuffer(x, buffer, 5);
                    free(buffer);
                }
            }
        }
        /* serial reply PORT ARG0 (ARG1) (ARG2) (...) */
        if(strcmp(cmdName, "reply") == 0){
            if(argc > 2){
                uint8_t port = serialPort(atom_getsymbolarg(1, argc, argv)->s_name);
                if(port == 255){
                    error("Unknown serial port name");
                    return;
                }
                uint8_t * buffer = (uint8_t *)malloc((((argc - 2) * 2) + 4) * sizeof(uint8_t));
                buffer[0] = 0xF0;
                buffer[1] = 0x60;
                buffer[2] = 0x40 | port;
                uint8_t i = 2;
                while(i < argc){
                    buffer[((i - 2) * 2) + 3] = atom_getint(argv + i) & 0x7F;
                    buffer[((i - 2) * 2) + 4] = (atom_getint(argv + i) >> 7) & 0x7F;
                    i++;
                }
                buffer[((argc - 2) * 2) + 3] = 0xF7;
                writeBuffer(x, buffer, ((argc - 2) * 2) + 4);
                free(buffer);
            }
        }
        /* serial close PORT */
        if(strcmp(cmdName, "close") == 0){
            if(argc > 1){
                uint8_t port = serialPort(atom_getsymbolarg(1, argc, argv)->s_name);
                if(port == 255){
                    error("Unknown serial port name");
                    return;
                }
                uint8_t * buffer = (uint8_t *)malloc(4 * sizeof(uint8_t));
                buffer[0] = 0xF0;
                buffer[1] = 0x60;
                buffer[2] = 0x50 | port;
                buffer[3] = 0xF7;
                writeBuffer(x, buffer, 4);
                free(buffer);
            }
        }
        /* serial flush PORT */
        if(strcmp(cmdName, "flush") == 0){
            if(argc > 1){
                uint8_t port = serialPort(atom_getsymbolarg(1, argc, argv)->s_name);
                if(port == 255){
                    error("Unknown serial port name");
                    return;
                }
                uint8_t * buffer = (uint8_t *)malloc(4 * sizeof(uint8_t));
                buffer[0] = 0xF0;
                buffer[1] = 0x60;
                buffer[2] = 0x60 | port;
                buffer[3] = 0xF7;
                writeBuffer(x, buffer, 4);
                free(buffer);
            }
        }
        /* serial listen PORT */
        if(strcmp(cmdName, "listen") == 0){
            if(argc > 1){
                uint8_t port = serialPort(atom_getsymbolarg(1, argc, argv)->s_name);
                if(port == 255){
                    error("Unknown serial port name");
                    return;
                }
                uint8_t * buffer = (uint8_t *)malloc(4 * sizeof(uint8_t));
                buffer[0] = 0xF0;
                buffer[1] = 0x60;
                buffer[2] = 0x70 | port;
                buffer[3] = 0xF7;
                writeBuffer(x, buffer, 4);
                free(buffer);
            }
        }
    }
}

void pdfirmata_I2C(t_pdfirmata * x, t_symbol * s, t_int argc, t_atom * argv){
    if(argc > 0){
        const char * cmdName = atom_getsymbolarg(0, argc, argv)->s_name;
        /* I2C rw ADDRESS AUTORESTART RWMODE ARG0 (ARG1) (ARG2) (...) */
        /* AUTORESTART = 0 : Stop, 1 : Restart */
        /* RWMODE = wr : Write, ro : Read Only, rc : Read Continuously, rw : Read Write */
        if(strcmp(cmdName, "rw") == 0){
            if(argc > 4){
                int addr = atom_getfloatarg(1, argc, argv);
                int autoRestart = atom_getfloatarg(2, argc, argv);
                const char * rwMode = atom_getsymbolarg(3, argc, argv)->s_name;
                int rwModeInt = 0;
                if(strcmp(rwMode, "wr") == 0){ rwModeInt = 0; }
                else if(strcmp(rwMode, "ro") == 0){ rwModeInt = 1; }
                else if(strcmp(rwMode, "rc") == 0){ rwModeInt = 2; }
                else if(strcmp(rwMode, "rw") == 0){ rwModeInt = 3; }
                else{ error("Unknown rw mode"); return; }
                int LSB = addr & 0x7F;
                int MSB = 0;
                if(addr > 0x7F){ MSB = (addr >> 7) & 0x7; }
                MSB |= (rwModeInt & 0x3) << 2;
                if(addr > 0x7F) MSB |= 0x1 << 4;
                MSB |= (autoRestart & 0x1) << 5;
                uint8_t * buffer = (uint8_t *)malloc((((argc - 4) * 2) + 5) * sizeof(uint8_t));
                buffer[0] = 0xF0;
                buffer[1] = 0x76;
                buffer[2] = LSB & 0x7F;
                buffer[3] = MSB & 0x7F;
                uint8_t i = 4;
                while(i < argc){
                    buffer[((i - 4) * 2) + 4] = atom_getint(argv + i) & 0x7F;
                    buffer[((i - 4) * 2) + 5] = (atom_getint(argv + i) >> 7) & 0x7F;
                    i++;
                }
                buffer[((argc - 4) * 2) + 4] = 0xF7;
                writeBuffer(x, buffer, ((argc - 4) * 2) + 5);
                free(buffer);
            }
        }
        /* I2C reply  */
        if(strcmp(cmdName, "reply") == 0){
            if(argc > 3){
                int addr = atom_getfloatarg(1, argc, argv);
                int registerAddr = atom_getfloatarg(2, argc, argv);
                uint8_t * buffer = (uint8_t *)malloc((((argc - 3) * 2) + 5) * sizeof(uint8_t));
                buffer[0] = 0x70;
                buffer[1] = 0x77;
                buffer[2] = addr & 0x7F;
                buffer[3] = (addr >> 7) & 0x7;
                buffer[4] = registerAddr & 0x7F;
                buffer[5] = (registerAddr >> 7) & 0x7F;
                uint8_t i = 3;
                while(i < argc){
                    buffer[((i - 3) * 2) + 6] = atom_getint(argv + i) & 0x7F;
                    buffer[((i - 3) * 2) + 7] = (atom_getint(argv + i) >> 7) & 0x7F;
                    i++;
                }
                buffer[((argc - 3) * 2) + 4] = 0xF7;
                writeBuffer(x, buffer, ((argc - 3) * 2) + 5);
                free(buffer);
            }
        }
        /* I2C delay DELAY */
        if(strcmp(cmdName, "delay") == 0){
            if(argc > 1){
                int delay = atom_getfloatarg(1, argc, argv);
                uint8_t * buffer = (uint8_t *)malloc(5 * sizeof(uint8_t));
                buffer[0] = 0xF0;
                buffer[1] = 0x78;
                buffer[2] = delay & 0x7F;
                buffer[3] = (delay >> 7) & 0x7F;
                buffer[4] = 0xF7;
                writeBuffer(x, buffer, 5);
                free(buffer);
            }
        }
        if(strcmp(cmdName, "config") == 0){
            if(argc > 1){
                uint8_t * buffer = (uint8_t *)malloc((argc + 3) * sizeof(uint8_t));
                buffer[0] = 0xF0;
                buffer[1] = 0x78;
                uint8_t i = 1;
                while(i < argc){
                    buffer[i + 1] = atom_getint(argv + i) & 0x7F;
                    i++;
                }
                buffer[argc + 2] = 0xF7;
                writeBuffer(x, buffer, argc + 3);
                free(buffer);
            }
        }
    }
}

void pdfirmata_servo(t_pdfirmata * x, t_symbol * s, t_int argc, t_atom * argv){
    if(argc > 0){
        const char * cmdName = atom_getsymbolarg(0, argc, argv)->s_name;
        /* servo config PIN MINPULSE MAXPULSE */
        if(strcmp(cmdName, "config") == 0){
            if(argc > 3){
                int pin = atom_getfloatarg(1, argc, argv);
                int minPulse = atom_getfloatarg(2, argc, argv);
                int maxPulse = atom_getfloatarg(3, argc, argv);
                uint8_t * buffer = (uint8_t *)malloc(8 * sizeof(uint8_t));
                buffer[0] = 0xF0;
                buffer[1] = 0x70;
                buffer[2] = pin & 0x7F;
                buffer[3] = minPulse & 0x7F;
                buffer[4] = (minPulse >> 7) & 0x7F;
                buffer[5] = maxPulse & 0x7F;
                buffer[6] = (maxPulse >> 7) & 0x7F;
                buffer[7] = 0xF7;
                writeBuffer(x, buffer, 8);
                free(buffer);
            }
        }
        /* servo write PIN VALUE */
        if(strcmp(cmdName, "write") == 0){
            if(argc > 2){
                int pin = atom_getfloatarg(1, argc, argv);
                int value = atom_getfloatarg(2, argc, argv);
                if(pin < 16 && value < 0x4000){
                    uint8_t * buffer = (uint8_t *)malloc(3 * sizeof(uint8_t));
                    buffer[0] = 0xE0 + pin;
                    buffer[1] = value & 0x7F;
                    buffer[2] = (value >> 7) & 0x7F;
                    writeBuffer(x, buffer, 3);
                    free(buffer);
                }
                else{
                    uint8_t * buffer = (uint8_t *)malloc(8 * sizeof(uint8_t));
                    buffer[0] = 0xF0;
                    buffer[1] = 0x6F;
                    buffer[2] = pin;
                    buffer[3] = value & 127;
                    buffer[4] = (value >> 7) & 127;
                    buffer[5] = (value >> 14) & 127;
                    buffer[6] = (value >> 21) & 127;
                    buffer[7] = 0xF7;
                    writeBuffer(x, buffer, 8);
                    free(buffer);
                }
            }
        }
    }
}

void pdfirmata_encoder(t_pdfirmata * x, t_symbol * s, t_int argc, t_atom * argv){
    if(argc > 0){
        /* encoder attach ENCODER PINA PINB */
        const char * cmdName = atom_getsymbolarg(0, argc, argv)->s_name;
        if(strcmp(cmdName, "attach") == 0){
            if(argc > 3){
                int encoder = atom_getfloatarg(1, argc, argv);
                int pinA = atom_getfloatarg(2, argc, argv);
                int pinB = atom_getfloatarg(3, argc, argv);
                uint8_t * buffer = (uint8_t *)malloc(7 * sizeof(uint8_t));
                buffer[0] = 0xF0;
                buffer[1] = 0x61;
                buffer[2] = 0x00;
                buffer[3] = encoder & 0x7F;
                buffer[4] = pinA & 0x7F;
                buffer[5] = pinB & 0x7F;
                buffer[6] = 0xF7;
                writeBuffer(x, buffer, 7);
                free(buffer);
            }
        }
        /* encoder read ENCODER */
        if(strcmp(cmdName, "read") == 0){
            if(argc > 1){
                int encoder = atom_getfloatarg(1, argc, argv);
                uint8_t * buffer = (uint8_t *)malloc(5 * sizeof(uint8_t));
                buffer[0] = 0xF0;
                buffer[1] = 0x61;
                buffer[2] = 0x01;
                buffer[3] = encoder & 0x7F;
                buffer[4] = 0xF7;
                writeBuffer(x, buffer, 5);
                free(buffer);
            }
        }
        /* encoder readAll */
        if(strcmp(cmdName, "readAll") == 0){
            uint8_t * buffer = (uint8_t *)malloc(4 * sizeof(uint8_t));
            buffer[0] = 0xF0;
            buffer[1] = 0x61;
            buffer[2] = 0x02;
            buffer[3] = 0xF7;
            writeBuffer(x, buffer, 4);
            free(buffer);
        }
        /* encoder reset ENCODER */
        if(strcmp(cmdName, "reset") == 0){
            if(argc > 1){
                int encoder = atom_getfloatarg(1, argc, argv);
                uint8_t * buffer = (uint8_t *)malloc(5 * sizeof(uint8_t));
                buffer[0] = 0xF0;
                buffer[1] = 0x61;
                buffer[2] = 0x03;
                buffer[3] = encoder & 0x7F;
                buffer[4] = 0xF7;
                writeBuffer(x, buffer, 5);
                free(buffer);
            }
        }
        /* encoder report ENCODER */
        if(strcmp(cmdName, "report") == 0){
            if(argc > 1){
                int enable = atom_getfloatarg(1, argc, argv);
                uint8_t * buffer = (uint8_t *)malloc(5 * sizeof(uint8_t));
                buffer[0] = 0xF0;
                buffer[1] = 0x61;
                buffer[2] = 0x04;
                buffer[3] = enable & 0x1;
                buffer[4] = 0xF7;
                writeBuffer(x, buffer, 5);
                free(buffer);
            }
        }
        /* encoder detach ENCODER */
        if(strcmp(cmdName, "detach") == 0){
            if(argc > 1){
                int encoder = atom_getfloatarg(1, argc, argv);
                uint8_t * buffer = (uint8_t *)malloc(5 * sizeof(uint8_t));
                buffer[0] = 0xF0;
                buffer[1] = 0x61;
                buffer[2] = 0x05;
                buffer[3] = encoder & 0x7F;
                buffer[4] = 0xF7;
                writeBuffer(x, buffer, 5);
                free(buffer);
            }
        }
    }
}

void pdfirmata_stepper(t_pdfirmata * x, t_symbol * s, t_int argc, t_atom * argv){
    error("Not yet implemented");
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
