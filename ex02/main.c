#include <avr/io.h>
#include <util/delay.h>

void init_rgb() {
	DDRD |= (1 << PD6) | (1 << PD5) | (1 << PB3);

	//timer0
	TCCR0A = (1 << COM0A1) | (1 << COM0B1) | (1 << WGM01) | (1 << WGM00);
	TCCR0B = (1 << CS01) | (1 << CS00);

	//timer2
	TCCR2A = (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);
	TCCR2B = (1 << CS22);
}

static void set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
	OCR0B = r;   // red   PD5
    OCR0A = g;   // green PD6
    OCR2B = b;   // blue  PD3
}

void wheel(uint8_t pos) {
	pos = 255 - pos;
	if (pos < 85) {
		set_rgb(255 - pos * 3, 0, pos * 3);
	} else if (pos < 170) {
		pos = pos - 85;
		set_rgb(0, pos * 3, 255 - pos * 3);
	} else {
		pos = pos - 170;
		set_rgb(pos * 3, 255 - pos * 3, 0);
	}
}

int main () {
	init_rgb();
	uint8_t i = 0;
	while (1) {
		wheel(i++);
		_delay_ms(10);
	}
}