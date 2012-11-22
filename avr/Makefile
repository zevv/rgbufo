
NAME	= ufo

SRC	= main.c uart.c printd.c adc.c pwm.c

#############################################################################

OBJS	= $(subst .c,.o, $(SRC))
ELF	= $(NAME).elf
FHEX	= $(NAME).hex
EHEX	= $(NAME)-eeprom.hex

CFLAGS  += -mmcu=atmega8 -Wall -Werror -Os -g -I.
CFLAGS	+= -DF_CPU=16000000 
LDFLAGS += -mmcu=atmega8 -g
ADFLAGS += -p m8 -c avrispv2 -P usb

CC 	= avr-gcc
LD 	= avr-gcc
OBJCOPY = avr-objcopy
SIZE	= avr-size
AD	= sudo avrdude

all: $(ELF) $(FHEX) 

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(ELF): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) 
	$(SIZE) $(ELF)
	
$(FHEX) $(EHEX): $(ELF) 
	$(OBJCOPY) -j .text -j .data -O ihex $(ELF) $(FHEX)
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O ihex $(ELF) $(EHEX)

$(OBJS) $(ELF): Makefile

install: $(FHEX) $(EHEX) tx
	$(AD) $(ADFLAGS) -y -e -V -U flash:w:$(FHEX):i -U eeprom:w:$(EHEX):i 

install_fuses: $(FHEX) $(EHEX)
	$(AD) $(ADFLAGS) -U lfuse:w:0xef:m 

clean:	
	rm -f $(OBJS) $(ELF) $(EHEX) $(FHEX)  dump tx

# End

