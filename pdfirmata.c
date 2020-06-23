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
    uint8_t  * buffer;
    t_atom   *  abuffer;
    uint32_t rawCounter;
    int32_t  rawType; //rawType 0 nothing, 1 sysex
    t_inlet  * raw;
    t_outlet * rawOut, * decOut;
}t_pdfirmata;

// function protoypes

void writeBuffer(t_pdfirmata * x, uint8_t * buffer, uint8_t bytec);
uint8_t serialPort(const char * portName);

void pdfirmata_onRawData(t_pdfirmata * x, t_floatarg _f);

void pdfirmata_version(t_pdfirmata * x);
void pdfirmata_firmware(t_pdfirmata * x);
void pdfirmata_capability(t_pdfirmata * x);
void pdfirmata_sampling(t_pdfirmata * x, t_floatarg time);
void pdfirmata_pinMode(t_pdfirmata * x, t_symbol * s, t_int argc, t_atom * argv);
void pdfirmata_digitalWrite(t_pdfirmata * x, t_floatarg pin, t_floatarg state);
void pdfirmata_digitalIn(t_pdfirmata * x, t_floatarg port, t_floatarg state);
void pdfirmata_analogMap(t_pdfirmata * x);
void pdfirmata_analogWrite(t_pdfirmata * x, t_floatarg pin, t_floatarg value);
void pdfirmata_analogIn(t_pdfirmata * x, t_floatarg pin, t_floatarg state);
void pdfirmata_pinState(t_pdfirmata * x, t_floatarg pin);
void pdfirmata_serial(t_pdfirmata * x, t_symbol * s, t_int argc, t_atom * argv);
void pdfirmata_I2C(t_pdfirmata * x, t_symbol * s, t_int argc, t_atom * argv);
void pdfirmata_servo(t_pdfirmata * x, t_symbol * s, t_int argc, t_atom * argv);
void pdfirmata_encoder(t_pdfirmata * x, t_symbol * s, t_int argc, t_atom * argv);
void pdfirmata_stepper(t_pdfirmata * x, t_symbol * s, t_int argc, t_atom * argv);
void pdfirmata_multistepper(t_pdfirmata * x, t_symbol * s, t_int argc, t_atom * argv);
void pdfirmata_onewire(t_pdfirmata * x, t_symbol * s, t_int argc, t_atom * argv);
void pdfirmata_scheduler(t_pdfirmata * x, t_symbol * s, t_int argc, t_atom * argv);

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

const char * serialPorts[] = {
    "HW0",
    "HW1",
    "HW2",
    "HW3",
    "HW4",
    "HW5",
    "HW6",
    "HW7",
    "SW0",
    "SW1",
    "SW2",
    "SW3",
    "SW4",
    "SW5",
    "SW6",
    "SW7"
};

const uint8_t serialPortsLength = sizeof(serialPorts) / sizeof(serialPorts[0]);

// !Constants

void * pdfirmata_new(t_floatarg bufferSize){
    t_pdfirmata * x = (t_pdfirmata *)pd_new(pdfirmata_class);

    if(bufferSize < 1) bufferSize = 256;

    x->buffer = (uint8_t *)malloc(bufferSize * sizeof(uint8_t));
    x->abuffer = (t_atom *)malloc(bufferSize * sizeof(t_atom));

    x->raw = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("rawdata"));

    x->decOut = outlet_new(&x->x_obj, &s_list);
    x->rawOut = outlet_new(&x->x_obj, &s_float);

    post("pdfirmata: Firmata client for Pute Data");
    post("Copyright (C) 2017-2020  Malik Enes Safak");
    post("https://github.com/NullMember/PDFirmata");

    return (void *)x;
}

void pdfirmata_free(t_pdfirmata *x){
    free(x->buffer);
    free(x->abuffer);
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
                                A_DEFFLOAT,
                                0);

    class_addmethod(pdfirmata_class, (t_method)pdfirmata_onRawData, gensym("rawdata"), A_DEFFLOAT, 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_version, gensym("version"), 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_firmware, gensym("firmware"), 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_capability, gensym("capability"), 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_sampling, gensym("sampling"), A_FLOAT, 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_pinMode, gensym("pinMode"), A_GIMME, 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_digitalWrite, gensym("digitalWrite"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_digitalIn, gensym("digitalIn"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_analogMap, gensym("analogMap"), 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_analogWrite, gensym("analogWrite"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_analogIn, gensym("analogIn"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_pinState, gensym("pinState"), A_FLOAT, 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_serial, gensym("serial"), A_GIMME, 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_I2C, gensym("I2C"), A_GIMME, 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_servo, gensym("servo"), A_GIMME, 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_encoder, gensym("encoder"), A_GIMME, 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_stepper, gensym("stepper"), A_GIMME, 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_multistepper, gensym("multistepper"), A_GIMME, 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_onewire, gensym("onewire"), A_GIMME, 0);
    class_addmethod(pdfirmata_class, (t_method)pdfirmata_scheduler, gensym("scheduler"), A_GIMME, 0);
}

void writeBuffer(t_pdfirmata * x, uint8_t * buffer, uint8_t bytec){
    uint8_t i = 0;
    while(i < bytec){
        outlet_float(x->rawOut, buffer[i]);
        i++;
    }
}

uint8_t serialPort(const char * portName){
    uint8_t i = 0;
    while(i < serialPortsLength){
        if(strcmp(portName, serialPorts[i]) == 0){
            return i;
        }
    }
    return 255;
}

void pdfirmata_onRawData(t_pdfirmata *x, t_floatarg f){
    /* SysEx End */
    if(f == 0xF7){
        x->rawType = 0;
        decSysex(x);
    }
    /* All bytes received */
    if(x->rawType == 0){
        x->rawCounter = 0;
    }
    /* Read incoming SysEx bytes */
    else if(x->rawType == 1){
        x->buffer[x->rawCounter] = f;
        x->rawCounter++;
    }
    /* Read and output analog read command */
    else if(x->rawType == 2){
        x->buffer[x->rawCounter] = f;
        x->rawCounter++;
        if(x->rawCounter == 3){
            t_atom * buffer = (t_atom *)malloc(3 * sizeof(t_atom));
            uint16_t value = x->buffer[1] & 0x7F;
            value |= (x->buffer[2] & 0x7F) << 7;
            SETSYMBOL(buffer, gensym("analog"));
            SETFLOAT(buffer + 1, x->buffer[0]);
            SETFLOAT(buffer + 2, value);
            outlet_list(x->decOut, &s_list, 3, buffer);
            free(buffer);
            x->rawType = 0;
        }
    }
    /* Read and output digital read command */
    else if(x->rawType == 3){
        x->buffer[x->rawCounter] = f;
        x->rawCounter++;
        if(x->rawCounter == 3){
            t_atom * buffer = (t_atom *)malloc(10 * sizeof(t_atom));
            uint16_t value = x->buffer[1] & 0x7F;
            value |= (x->buffer[2] & 0x1) << 7;
            SETSYMBOL(buffer, gensym("digital"));
            SETFLOAT(buffer + 1, x->buffer[0]);
            SETFLOAT(buffer + 2, value & 0x01);
            SETFLOAT(buffer + 3, (value >> 1) & 0x01);
            SETFLOAT(buffer + 4, (value >> 2) & 0x01);
            SETFLOAT(buffer + 5, (value >> 3) & 0x01);
            SETFLOAT(buffer + 6, (value >> 4) & 0x01);
            SETFLOAT(buffer + 7, (value >> 5) & 0x01);
            SETFLOAT(buffer + 8, (value >> 6) & 0x01);
            SETFLOAT(buffer + 9, (value >> 7) & 0x01);
            outlet_list(x->decOut, &s_list, 3, buffer);
            free(buffer);
            x->rawType = 0;
        }
    }
    /* Major and minor version numbers */
    else if(x->rawType == 4){
        x->buffer[x->rawCounter] = f;
        x->rawCounter++;
        if(x->rawCounter == 3){
            t_atom * buffer = (t_atom *)malloc(3 * sizeof(t_atom));
            uint8_t major = x->buffer[1] & 0x7F;
            uint8_t minor = x->buffer[2] & 0x7F;
            SETSYMBOL(buffer, gensym("version"));
            SETFLOAT(buffer + 1, major);
            SETFLOAT(buffer + 2, minor);
            outlet_list(x->decOut, &s_list, 3, buffer);
            free(buffer);
            x->rawType = 0;
        }
    }
    /* Beginning of SysEx command */
    /* Type 1 */
    if(f == 0xF0){
        x->rawType = 1;
    }
    /* Beginning of analog command */
    /* Type 2 */
    else if(((int)f & 0xF0) == 0xE0){
        x->buffer[x->rawCounter] = (int)f & 0x0F;
        x->rawCounter++;
        x->rawType = 2;
    }
    /* Beginning of digital command */
    /* Type 3 */
    else if(((int)f & 0xF0) == 0x90){
        x->buffer[x->rawCounter] = (int)f & 0x0F;
        x->rawCounter++;
        x->rawType = 3;
    }
    /* Beginning of version number */
    /* Type 4 */
    else if((int)f == 0xF9){
        x->buffer[x->rawCounter] = f;
        x->rawCounter++;
        x->rawType = 4;
    }
}

void pdfirmata_version(t_pdfirmata * x){
    uint8_t buffer = 0xF9;
    writeBuffer(x, &buffer, 1);
}

void pdfirmata_firmware(t_pdfirmata * x){
    uint8_t * buffer = (uint8_t *)malloc(3 * sizeof(uint8_t));
    buffer[0] = 0xF0;
    buffer[1] = 0x79;
    buffer[2] = 0xF7;
    writeBuffer(x, buffer, 3);
    free(buffer);
}

void pdfirmata_capability(t_pdfirmata * x){
    uint8_t * buffer = (uint8_t *)malloc(3 * sizeof(uint8_t));
    buffer[0] = 0xF0;
    buffer[1] = 0x6B;
    buffer[2] = 0xF7;
    writeBuffer(x, buffer, 3);
    free(buffer);
}

void pdfirmata_sampling(t_pdfirmata * x, t_floatarg time){
    int _time = time;
    uint8_t * buffer = (uint8_t *)malloc(5 * sizeof(uint8_t));
    buffer[0] = 0xF0;
    buffer[1] = 0x7A;
    buffer[2] = _time & 0x7F;
    buffer[3] = (_time >> 7) & 0x7F;
    buffer[4] = 0xF7;
    writeBuffer(x, buffer, 5);
    free(buffer);
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

void pdfirmata_digitalIn(t_pdfirmata * x, t_floatarg port, t_floatarg state){
    int _port = port;
    int _state = state;
    if(_port < 0 || _port > 15){
        error("Port number must between 0-15");
        return;
    }
    uint8_t * buffer = (uint8_t *)malloc(2 * sizeof(uint8_t));
    buffer[0] = 0xD0 + _port;
    buffer[1] = _state > 0 ? 1 : 0;
    writeBuffer(x, buffer, 2);
    free(buffer);
}

void pdfirmata_analogMap(t_pdfirmata * x){
    uint8_t * buffer = (uint8_t *)malloc(3 * sizeof(uint8_t));
    buffer[0] = 0xF0;
    buffer[1] = 0x69;
    buffer[2] = 0xF7;
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

void pdfirmata_pinState(t_pdfirmata * x, t_floatarg pin){
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
                    if(rxPin > 127){
                        error("Pin number must between 0-127");
                        return;
                    }
                    txPin = atom_getint(argv + 4);
                    if(txPin > 127){
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
                if(readMode > 1){
                    error("Read mode must 0 (Continuous) or 1 (Stop after read)");
                    return;
                }
                if(argc > 3){
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
        /* I2C config ARG0 (ARG1) (ARG2) ... */
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
        /* encoder report ENABLE */
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
    if(argc > 0){
        const char * cmdName = atom_getsymbolarg(0, argc, argv)->s_name;
        /* stepper config motor interface step enable pin1 pin2 (pin3) (pin4) (enablePin) (invert) */
        if(strcmp(cmdName, "config") == 0){
            if(argc > 6){
                uint8_t i = 1;
                uint8_t motor = atom_getfloatarg(1, argc, argv);
                uint8_t interface = atom_getfloatarg(2, argc, argv);
                uint8_t step = atom_getfloatarg(3, argc, argv);
                uint8_t enable = atom_getfloatarg(4, argc, argv);
                /* interface is driver or two pin */
                if(interface == 1 | interface == 2){
                    uint8_t pin1 = atom_getfloatarg(5, argc, argv);
                    uint8_t pin2 = atom_getfloatarg(6, argc, argv);
                    if(argc > 8){
                        uint8_t enablePin = atom_getfloatarg(7, argc, argv);
                        uint8_t invert = atom_getfloatarg(8, argc, argv);
                        uint8_t * buffer = (uint8_t *)malloc(10 * sizeof(uint8_t));
                        buffer[0] = 0xF0;
                        buffer[1] = 0x62;
                        buffer[2] = 0x00;
                        buffer[3] = motor & 0x7F;
                        buffer[4] = ((interface & 0x7) << 4) | ((step & 0x7) << 1) | (enable & 0x1);
                        buffer[5] = pin1 & 0x7F;
                        buffer[6] = pin2 & 0x7F;
                        buffer[7] = enablePin & 0x7F;
                        buffer[8] = invert & 0x7F;
                        buffer[9] = 0xF7;
                        writeBuffer(x, buffer, 10);
                        free(buffer);
                    }
                    else if(argc > 7){
                        uint8_t enablePinorInvert = atom_getfloatarg(7, argc, argv);
                        uint8_t * buffer = (uint8_t *)malloc(9 * sizeof(uint8_t));
                        buffer[0] = 0xF0;
                        buffer[1] = 0x62;
                        buffer[2] = 0x00;
                        buffer[3] = motor & 0x7F;
                        buffer[4] = ((interface & 0x7) << 4) | ((step & 0x7) << 1) | (enable & 0x1);
                        buffer[5] = pin1 & 0x7F;
                        buffer[6] = pin2 & 0x7F;
                        buffer[7] = enablePinorInvert & 0x7F;
                        buffer[8] = 0xF7;
                        writeBuffer(x, buffer, 9);
                        free(buffer);
                    }
                    else{
                        uint8_t * buffer = (uint8_t *)malloc(8 * sizeof(uint8_t));
                        buffer[0] = 0xF0;
                        buffer[1] = 0x62;
                        buffer[2] = 0x00;
                        buffer[3] = motor & 0x7F;
                        buffer[4] = ((interface & 0x7) << 4) | ((step & 0x7) << 1) | (enable & 0x1);
                        buffer[5] = pin1 & 0x7F;
                        buffer[6] = pin2 & 0x7F;
                        buffer[7] = 0xF7;
                        writeBuffer(x, buffer, 8);
                        free(buffer);
                    }
                }
                /* interface is three pin */
                else if(interface == 3){
                    if(argc > 7){
                        uint8_t pin1 = atom_getfloatarg(5, argc, argv);
                        uint8_t pin2 = atom_getfloatarg(6, argc, argv);
                        uint8_t pin3 = atom_getfloatarg(7, argc, argv);
                        if(argc > 9){
                            uint8_t enablePin = atom_getfloatarg(8, argc, argv);
                            uint8_t invert = atom_getfloatarg(9, argc, argv);
                            uint8_t * buffer = (uint8_t *)malloc(11 * sizeof(uint8_t));
                            buffer[0] = 0xF0;
                            buffer[1] = 0x62;
                            buffer[2] = 0x00;
                            buffer[3] = motor & 0x7F;
                            buffer[4] = ((interface & 0x7) << 4) | ((step & 0x7) << 1) | (enable & 0x1);
                            buffer[5] = pin1 & 0x7F;
                            buffer[6] = pin2 & 0x7F;
                            buffer[7] = pin3 & 0x7F;
                            buffer[8] = enablePin & 0x7F;
                            buffer[9] = invert & 0x7F;
                            buffer[10] = 0xF7;
                            writeBuffer(x, buffer, 11);
                            free(buffer);
                        }
                        else if(argc > 8){
                            uint8_t enablePinorInvert = atom_getfloatarg(8, argc, argv);
                            uint8_t * buffer = (uint8_t *)malloc(10 * sizeof(uint8_t));
                            buffer[0] = 0xF0;
                            buffer[1] = 0x62;
                            buffer[2] = 0x00;
                            buffer[3] = motor & 0x7F;
                            buffer[4] = ((interface & 0x7) << 4) | ((step & 0x7) << 1) | (enable & 0x1);
                            buffer[5] = pin1 & 0x7F;
                            buffer[6] = pin2 & 0x7F;
                            buffer[7] = pin3 & 0x7F;
                            buffer[8] = enablePinorInvert & 0x7F;
                            buffer[9] = 0xF7;
                            writeBuffer(x, buffer, 10);
                            free(buffer);
                        }
                        else{
                            uint8_t * buffer = (uint8_t *)malloc(9 * sizeof(uint8_t));
                            buffer[0] = 0xF0;
                            buffer[1] = 0x62;
                            buffer[2] = 0x00;
                            buffer[3] = motor & 0x7F;
                            buffer[4] = ((interface & 0x7) << 4) | ((step & 0x7) << 1) | (enable & 0x1);
                            buffer[5] = pin1 & 0x7F;
                            buffer[6] = pin2 & 0x7F;
                            buffer[7] = pin3 & 0x7F;
                            buffer[8] = 0xF7;
                            writeBuffer(x, buffer, 9);
                            free(buffer);
                        }
                    }
                }
                else if(interface == 4){
                    if(argc > 8){
                        uint8_t pin1 = atom_getfloatarg(5, argc, argv);
                        uint8_t pin2 = atom_getfloatarg(6, argc, argv);
                        uint8_t pin3 = atom_getfloatarg(7, argc, argv);
                        uint8_t pin4 = atom_getfloatarg(8, argc, argv);
                        if(argc > 10){
                            uint8_t enablePin = atom_getfloatarg(9, argc, argv);
                            uint8_t invert = atom_getfloatarg(10, argc, argv);
                            uint8_t * buffer = (uint8_t *)malloc(12 * sizeof(uint8_t));
                            buffer[0] = 0xF0;
                            buffer[1] = 0x62;
                            buffer[2] = 0x00;
                            buffer[3] = motor & 0x7F;
                            buffer[4] = ((interface & 0x7) << 4) | ((step & 0x7) << 1) | (enable & 0x1);
                            buffer[5] = pin1 & 0x7F;
                            buffer[6] = pin2 & 0x7F;
                            buffer[7] = pin3 & 0x7F;
                            buffer[8] = pin4 & 0x7F;
                            buffer[9] = enablePin & 0x7F;
                            buffer[10] = invert & 0x7F;
                            buffer[11] = 0xF7;
                            writeBuffer(x, buffer, 12);
                            free(buffer);
                        }
                        else if(argc > 9){
                            uint8_t enablePinorInvert = atom_getfloatarg(9, argc, argv);
                            uint8_t * buffer = (uint8_t *)malloc(11 * sizeof(uint8_t));
                            buffer[0] = 0xF0;
                            buffer[1] = 0x62;
                            buffer[2] = 0x00;
                            buffer[3] = motor & 0x7F;
                            buffer[4] = ((interface & 0x7) << 4) | ((step & 0x7) << 1) | (enable & 0x1);
                            buffer[5] = pin1 & 0x7F;
                            buffer[6] = pin2 & 0x7F;
                            buffer[7] = pin3 & 0x7F;
                            buffer[8] = pin4 & 0x7F;
                            buffer[9] = enablePinorInvert & 0x7F;
                            buffer[10] = 0xF7;
                            writeBuffer(x, buffer, 11);
                            free(buffer);
                        }
                        else{
                            uint8_t * buffer = (uint8_t *)malloc(10 * sizeof(uint8_t));
                            buffer[0] = 0xF0;
                            buffer[1] = 0x62;
                            buffer[2] = 0x00;
                            buffer[3] = motor & 0x7F;
                            buffer[4] = ((interface & 0x7) << 4) | ((step & 0x7) << 1) | (enable & 0x1);
                            buffer[5] = pin1 & 0x7F;
                            buffer[6] = pin2 & 0x7F;
                            buffer[7] = pin3 & 0x7F;
                            buffer[8] = pin4 & 0x7F;
                            buffer[9] = 0xF7;
                            writeBuffer(x, buffer, 10);
                            free(buffer);
                        }
                    }
                }
                else{
                    error("unknown interface number");
                }
            }
        }
        if(strcmp(cmdName, "zero") == 0){
            if(argc > 1){
                uint8_t motor = atom_getfloatarg(1, argc, argv);
                uint8_t * buffer = (uint8_t *)malloc(5 * sizeof(uint8_t));
                buffer[0] = 0xF0;
                buffer[1] = 0x62;
                buffer[2] = 0x01;
                buffer[3] = motor & 0x7F;
                buffer[4] = 0xF7;
                writeBuffer(x, buffer, 5);
                free(buffer);
            }
        }
        if(strcmp(cmdName, "step") == 0){
            if(argc > 2){
                uint8_t motor = atom_getfloatarg(1, argc, argv);
                int32_t step = atom_getfloatarg(2, argc, argv);
                uint8_t * buffer = (uint8_t *)malloc(10 * sizeof(uint8_t));
                buffer[0] = 0xF0;
                buffer[1] = 0x62;
                buffer[2] = 0x02;
                buffer[3] = motor & 0x7F;
                buffer[4] = step & 0x7F;
                buffer[5] = (step >> 7) & 0x7F;
                buffer[6] = (step >> 14) & 0x7F;
                buffer[7] = (step >> 21) & 0x7F;
                buffer[8] = (step >> 28) & 0x1F;
                buffer[9] = 0xF7;
                writeBuffer(x, buffer, 10);
                free(buffer);
            }
        }
        if(strcmp(cmdName, "to") == 0){
            if(argc > 2){
                uint8_t motor = atom_getfloatarg(1, argc, argv);
                int32_t position = atom_getfloatarg(2, argc, argv);
                uint8_t * buffer = (uint8_t *)malloc(10 * sizeof(uint8_t));
                buffer[0] = 0xF0;
                buffer[1] = 0x62;
                buffer[2] = 0x03;
                buffer[3] = motor & 0x7F;
                buffer[4] = position & 0x7F;
                buffer[5] = (position >> 7) & 0x7F;
                buffer[6] = (position >> 14) & 0x7F;
                buffer[7] = (position >> 21) & 0x7F;
                buffer[8] = (position >> 28) & 0x1F;
                buffer[9] = 0xF7;
                writeBuffer(x, buffer, 10);
                free(buffer);
            }
        }
        if(strcmp(cmdName, "enable") == 0){
            if(argc > 2){
                uint8_t motor = atom_getfloatarg(1, argc, argv);
                int32_t state = atom_getfloatarg(2, argc, argv);
                uint8_t * buffer = (uint8_t *)malloc(6 * sizeof(uint8_t));
                buffer[0] = 0xF0;
                buffer[1] = 0x62;
                buffer[2] = 0x04;
                buffer[3] = motor & 0x7F;
                buffer[4] = state & 0x7F;
                buffer[5] = 0xF7;
                writeBuffer(x, buffer, 6);
                free(buffer); 
            }
        }
        if(strcmp(cmdName, "stop") == 0){
            if(argc > 1){
                uint8_t motor = atom_getfloatarg(1, argc, argv);
                uint8_t * buffer = (uint8_t *)malloc(5 * sizeof(uint8_t));
                buffer[0] = 0xF0;
                buffer[1] = 0x62;
                buffer[2] = 0x05;
                buffer[3] = motor & 0x7F;
                buffer[4] = 0xF7;
                writeBuffer(x, buffer, 5);
                free(buffer);
            }
        }
        if(strcmp(cmdName, "position") == 0){
            if(argc > 1){
                uint8_t motor = atom_getfloatarg(1, argc, argv);
                uint8_t * buffer = (uint8_t *)malloc(5 * sizeof(uint8_t));
                buffer[0] = 0xF0;
                buffer[1] = 0x62;
                buffer[2] = 0x06;
                buffer[3] = motor & 0x7F;
                buffer[4] = 0xF7;
                writeBuffer(x, buffer, 5);
                free(buffer);
            }
        }
        if(strcmp(cmdName, "limit") == 0){
            error("Not implemented yet");
        }
        if(strcmp(cmdName, "acceleration") == 0){
            error("Not implemented yet");
        }
        if(strcmp(cmdName, "speed") == 0){
            error("Not implemented yet");
        }
    }
}

void pdfirmata_multistepper(t_pdfirmata * x, t_symbol * s, t_int argc, t_atom * argv){
    if(argc > 0){
        const char * cmdName = atom_getsymbolarg(0, argc, argv)->s_name;
        /* multistep config group motor1 motor2 (motor3) (motor4) ... */
        if(strcmp(cmdName, "config") == 0){
            if(argc > 4){
                uint8_t group = atom_getfloatarg(1, argc, argv);
                uint8_t * buffer = (uint8_t *)malloc(((argc - 2) + 5) * sizeof(uint8_t));
                buffer[0] = 0xF0;
                buffer[1] = 0x62;
                buffer[2] = 0x20;
                buffer[3] = group & 0x7F;
                uint8_t i = 2;
                while(i < argc){
                    buffer[i + 2] = atom_getfloatarg(i, argc, argv);
                    i++;
                }
                buffer[i + 2] = 0xF7;
                writeBuffer(x, buffer, ((argc - 2) + 5));
                free(buffer);
            }
        }
        /* multistep to group motor1 (motor2) (motor3) (motor4) ... */
        if(strcmp(cmdName, "to") == 0){
            if(argc > 2){
                uint8_t group = atom_getfloatarg(1, argc, argv);
                uint8_t positionCount = argc - 2;
                int32_t * positions = (int32_t *)malloc(positionCount * sizeof(int32_t));
                uint8_t i = 2;
                while(i < argc){
                    positions[i - 2] = atom_getfloatarg(i, argc, argv);
                    i++;
                }
                uint8_t * buffer = (uint8_t *)malloc(((positionCount * 5) + 5) * sizeof(uint8_t));
                buffer[0] = 0xF0;
                buffer[1] = 0x62;
                buffer[2] = 0x21;
                buffer[3] = group & 0x7F;
                i = 0;
                while(i < positionCount){
                    buffer[((i * 5) + 4)] = positions[i] & 0x7F;
                    buffer[((i * 5) + 4) + 1] = (positions[i] & 0x7F) << 7;
                    buffer[((i * 5) + 4) + 2] = (positions[i] & 0x7F) << 14;
                    buffer[((i * 5) + 4) + 3] = (positions[i] & 0x7F) << 21;
                    buffer[((i * 5) + 4) + 4] = (positions[i] & 0x1F) << 28;
                    i++;
                }
                buffer[(positionCount * 5) + 4] = 0xF7;
                writeBuffer(x, buffer, ((positionCount * 5) + 5));
                free(buffer);
            }
        }
        /* multistep stop group */
        if(strcmp(cmdName, "stop") == 0){
            if(argc > 1){
                uint8_t group = atom_getfloatarg(1, argc, argv);
                uint8_t * buffer = (uint8_t *)malloc(5 * sizeof(uint8_t));
                buffer[0] = 0xF0;
                buffer[1] = 0x62;
                buffer[2] = 0x23;
                buffer[3] = group & 0x7F;
                buffer[4] = 0xF7;
                writeBuffer(x, buffer, 5);
                free(buffer);
            }
        }
    }
}

void pdfirmata_onewire(t_pdfirmata * x, t_symbol * s, t_int argc, t_atom * argv){
    error("Not yet implemented");
}

void pdfirmata_scheduler(t_pdfirmata * x, t_symbol * s, t_int argc, t_atom * argv){
    error("Not yet implemented");
}

void decSysex(t_pdfirmata * x){
    /* Stepper replies */
    if(x->buffer[0] == 0x62){
        /* Stepper position reply */
        if(x->buffer[1] == 0x06){
            uint16_t i = 2;
            uint8_t stepper = x->buffer[i++];
            int32_t position = x->buffer[i++] & 0x7F;
            position |= (x->buffer[i++] & 0x7F) << 7;
            position |= (x->buffer[i++] & 0x7F) << 14;
            position |= (x->buffer[i++] & 0x7F) << 21;
            position |= (x->buffer[i++] & 0x1F) << 28;
            t_atom * buffer = (t_atom *)malloc(4 * sizeof(t_atom));
            SETSYMBOL(buffer, gensym("stepper"));
            SETSYMBOL(buffer + 1, gensym("position"));
            SETFLOAT(buffer + 2, stepper);
            SETFLOAT(buffer + 3, position);
            outlet_list(x->decOut, &s_symbol, 4, buffer);
            free(buffer);
        }
        /* Stepper move complete reply */
        if(x->buffer[1] == 0x0A){
            uint16_t i = 2;
            uint8_t stepper = x->buffer[i++];
            int32_t position = x->buffer[i++] & 0x7F;
            position |= (x->buffer[i++] & 0x7F) << 7;
            position |= (x->buffer[i++] & 0x7F) << 14;
            position |= (x->buffer[i++] & 0x7F) << 21;
            position |= (x->buffer[i++] & 0x1F) << 28;
            t_atom * buffer = (t_atom *)malloc(4 * sizeof(t_atom));
            SETSYMBOL(buffer, gensym("stepper"));
            SETSYMBOL(buffer + 1, gensym("complete"));
            SETFLOAT(buffer + 2, stepper);
            SETFLOAT(buffer + 3, position);
            outlet_list(x->decOut, &s_symbol, 4, buffer);
            free(buffer);
        }
        /* MultiStepper move complete reply */
        if(x->buffer[1] == 0x24){
            uint8_t group = x->buffer[2];
            t_atom * buffer = (t_atom *)malloc(3 * sizeof(t_atom));
            SETSYMBOL(buffer, gensym("multistepper"));
            SETSYMBOL(buffer + 1, gensym("complete"));
            SETFLOAT(buffer + 2, group);
            outlet_list(x->decOut, &s_symbol, 3, buffer);
            free(buffer);
        }
    }
    /* Firmware name and version reply */
    if(x->buffer[0] == 0x79){
        uint16_t i = 1;
        uint16_t counter = 0;
        uint8_t majorVersion = x->buffer[i++];
        uint8_t minorVersion = x->buffer[i++];
        t_atom * buffer = (t_atom *)malloc(4 * sizeof(t_atom));
        uint16_t firmwareChar = 0;
        char * firmwareName = (char *)malloc((((x->rawCounter - 3) / 2) + 1) * sizeof(char));
        SETSYMBOL(buffer, gensym("firmware"));
        SETFLOAT(buffer + 1, majorVersion);
        SETFLOAT(buffer + 2, minorVersion);
        while(i < x->rawCounter){
            if((i - 3) % 2 == 0){
                firmwareChar = x->buffer[i] & 0x7F;
            }
            else{
                firmwareChar |= (x->buffer[i] & 0x7F) << 7;
                firmwareName[counter] = firmwareChar;
                counter++;
            }
            i++;
        }
        firmwareName[counter] = '\0';
        SETSYMBOL(buffer + 3, gensym(firmwareName));
        outlet_list(x->decOut, &s_symbol, 4, buffer);
        free(buffer);
        free(firmwareName);
    }
    /* Firmata string */
    if(x->buffer[0] == 0x71){
        uint16_t i = 1;
        uint16_t counter = 0;
        t_atom * buffer = (t_atom *)malloc(2 * sizeof(t_atom));
        uint16_t stringChar = 0;
        char * stringName = (char *)malloc((((x->rawCounter - 1) / 2) + 1) * sizeof(char));
        SETSYMBOL(buffer, gensym("string"));
        while(i < x->rawCounter){
            if((i - 1) % 2 == 0){
                stringChar = x->buffer[i] & 0x7F;
            }
            else{
                stringChar |= (x->buffer[i] & 0x7F) << 7;
                stringName[counter] = stringChar;
                counter++;
            }
            i++;
        }
        stringName[counter] = '\0';
        SETSYMBOL(buffer + 1, gensym(stringName));
        outlet_list(x->decOut, &s_symbol, 2, buffer);
        free(buffer);
        free(stringName);
    }
    /* Report encoder(s) position */
    if(x->buffer[0] == 0x61){
        uint8_t encoderCount = (x->rawCounter - 1) / 5; // (counter - first byte (0x61)) / report length
        uint16_t i = 0;
        t_atom * buffer = (t_atom *)malloc(3 * sizeof(t_atom));
        while(i < encoderCount){
            uint8_t encoder = x->buffer[(i * 5) + 1] & 0x3F;
            uint8_t direction = (x->buffer[(i * 5) + 1] >> 6) & 0x1;
            int32_t position = (int32_t)x->buffer[(i * 5) + 2] & 0x7F;
            position |= ((int32_t)x->buffer[(i * 5) + 3] & 0x7F) << 7;
            position |= ((int32_t)x->buffer[(i * 5) + 4] & 0x7F) << 14;
            position |= ((int32_t)x->buffer[(i * 5) + 5] & 0x7F) << 21;
            position *= direction == 0 ? 1 : -1;
            SETSYMBOL(buffer, gensym("encoderReply"));
            SETFLOAT(buffer + 1, encoder);
            SETFLOAT(buffer + 2, position);
            outlet_list(x->decOut, &s_list, 3, buffer);
            i++;
        }
        free(buffer);
    }
    /* Analog mapping response */
    if(x->buffer[0] == 0x6A){
        uint8_t analog = 0;
        uint16_t i = 1;
        t_atom * buffer = (t_atom *)malloc(3 * sizeof(t_atom));
        while(i < x->rawCounter){
            analog = x->buffer[i + 1];
            SETSYMBOL(buffer, gensym("analogMap"));
            SETFLOAT(buffer + 1, i - 1);
            SETFLOAT(buffer + 2, analog);
            outlet_list(x->decOut, &s_list, 3, buffer);
            i++;
        }
        free(buffer);
    }
    /* Capabilities response */
    if(x->buffer[0] == 0x6C){
        uint16_t i = 1;
        uint8_t mode = 0;
        uint8_t resolution = 0;
        uint8_t pin = 0;
        uint16_t counter = 0;
        t_atom * buffer = (t_atom *)malloc(4 * sizeof(t_atom));
        SETSYMBOL(buffer, gensym("capability"));
        SETFLOAT(buffer + 1, pin);
        while(i < x->rawCounter){
            if(x->buffer[i] == 0x7F){
                pin++;
                counter = 0;
                SETFLOAT(buffer + 1, pin);
            }
            else{
                if(counter % 2 == 0){
                    mode = x->buffer[i];
                    SETSYMBOL(buffer + 2, gensym(pinModes[mode]));
                }
                else if(counter % 2 == 1){
                    resolution = x->buffer[i];
                    SETFLOAT(buffer + 3, resolution);
                    outlet_list(x->decOut, &s_symbol, 4, buffer);
                }
                counter++;
            }
            i++;
        }
        free(buffer);
    }
    /* Pin state response */
    if(x->buffer[0] == 0x6E){
        uint16_t i = 1;
        uint8_t pin = x->buffer[i++];
        uint8_t mode = x->buffer[i++];
        uint32_t state = 0;
        t_atom * buffer = (t_atom *)malloc(4 * sizeof(t_atom));
        while(i < x->rawCounter){
            state |= (x->buffer[i] & 0x7F) << (7 * (i - 3));
            i++;
        }
        SETSYMBOL(buffer, gensym("pinState"));
        SETFLOAT(buffer + 1, pin);
        SETSYMBOL(buffer + 2, gensym(pinModes[mode]));
        SETFLOAT(buffer + 3, state);
        outlet_list(x->decOut, &s_symbol, 4, buffer);
        free(buffer);
    }
    /* Serial read response */
    if(x->buffer[0] == 0x60){
        uint16_t i = 1;
        uint16_t value = 0;
        uint16_t counter = 0;
        uint8_t port = x->buffer[i++] & 0x0F;
        t_atom * buffer = (t_atom *)malloc((((x->rawCounter - 2) / 2) + 2) * sizeof(t_atom));
        SETSYMBOL(buffer, gensym("serialReply"));
        SETSYMBOL(buffer + 1, gensym(serialPorts[port]));
        while(i < x->rawCounter){
            if((i % 2) == 0){
                value = x->buffer[i] & 0x7F;
            }
            else{
                value |= (x->buffer[i] & 0x7F) << 7;
                SETFLOAT(buffer + counter, value);
                counter++;
            }
            i++;
        }
        outlet_list(x->decOut, &s_symbol, (((x->rawCounter - 2) / 2) + 2), buffer);
        free(buffer);
    }
    /* serial read response converted to string */
    if(x->buffer[0] == 0x60){
        uint16_t i = 1;
        uint16_t value = 0;
        uint16_t counter = 0;
        uint8_t port = x->buffer[i++] & 0x0F;
        t_atom * buffer = (t_atom *)malloc(3 * sizeof(t_atom));
        char * stringBuffer = (char *)malloc((((x->rawCounter - 2) / 2) + 1) * sizeof(char));
        SETSYMBOL(buffer, gensym("serialReplyString"));
        SETSYMBOL(buffer + 1, gensym(serialPorts[port]));
        while(i < x->rawCounter){
            if((i % 2) == 0){
                value = x->buffer[i] & 0x7F;
            }
            else{
                value |= (x->buffer[i] & 0x7F) << 7;
                stringBuffer[counter] = value;
                counter++;
            }
            i++;
        }
        stringBuffer[counter] = '\0';
        SETSYMBOL(buffer + 2, gensym(stringBuffer));
        outlet_list(x->decOut, &s_symbol, 3, buffer);
        free(buffer);
        free(stringBuffer);
    }
    /* I2C reply */
    if(x->buffer[0] == 0x77){
        uint16_t i = 1;
        uint16_t value = 0;
        uint16_t slaveAddress = x->buffer[i++] & 0x7F;
        slaveAddress |= (x->buffer[i++] & 0x7F) << 7;
        uint16_t registerAddress = x->buffer[i++] & 0x7F;
        registerAddress |= (x->buffer[i++] & 0x7F) << 7;
        t_atom * buffer = (t_atom *)malloc((((x->rawCounter - 5) / 2) + 3) * sizeof(t_atom));
        SETSYMBOL(buffer, gensym("I2CReply"));
        SETFLOAT(buffer + 1, slaveAddress);
        SETFLOAT(buffer + 2, registerAddress);
        uint16_t counter = 3;
        while(i < x->rawCounter){
            if((i - 5) % 2 == 0){
                value = x->buffer[i];
            }
            else{
                value |= (x->buffer[i] & 0x7F) << 7;
                SETFLOAT(buffer + counter, value);
                counter++;
            }
            i++;
        }
        outlet_list(x->decOut, &s_symbol, (((x->rawCounter - 5) / 2) + 3), buffer);
        free(buffer);
    }
}
