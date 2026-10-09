#include <avr/io.h>
#include <util/delay.h>


static void set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
	PORTD &= ~((1 << PD6) | (1 << PD5) | (1 << PD3));
    if (r) PORTD |= (1 << PD5);
    if (g) PORTD |= (1 << PD6);
    if (b) PORTD |= (1 << PD3);
}


int main () {
	DDRD |= (1 << PD6) | (1 << PD5) | (1 << PD3);
	while (1) {
		set_rgb(255, 0, 0);
		_delay_ms(1000);
		set_rgb(0, 255, 0);
		_delay_ms(1000);
		set_rgb(0, 0, 255);
		_delay_ms(1000);

		set_rgb(255, 255, 0);
		_delay_ms(1000);
		set_rgb(0, 255, 255);
		_delay_ms(1000);
		set_rgb(255, 0, 255);
		_delay_ms(1000);

		set_rgb(255, 255, 255);
		_delay_ms(1000);
	}
}