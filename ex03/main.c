#include <avr/io.h>
#include <util/delay.h>

#define BAUD 115200UL
#define BUF_MAX 32

void init_rgb() {
	DDRD |= (1 << PD6) | (1 << PD5) | (1 << PD3);

	// inverting fast PWM: OCR = MAX gives a constant low, so 0 is fully off
	// (non-inverting with OCR = 0 still outputs a 1-tick spike every period)

	//timer0
	TCCR0A = (1 << COM0A1) | (1 << COM0A0) | (1 << COM0B1) | (1 << COM0B0) | (1 << WGM01) | (1 << WGM00);
	TCCR0B = (1 << CS01) | (1 << CS00);

	//timer2
	TCCR2A = (1 << COM2B1) | (1 << COM2B0) | (1 << WGM21) | (1 << WGM20);
	TCCR2B = (1 << CS22);
}

// LEDs look too bright at low duty cycles, square the value to compensate
uint8_t gamma_correct(uint8_t c){
	return ((uint16_t)c * c) / 255;
}

static void set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
	OCR0B = 255 - gamma_correct(r);   // red   PD5
	OCR0A = 255 - gamma_correct(g);   // green PD6
	OCR2B = 255 - gamma_correct(b);   // blue  PD3
}

void uart_init(void) {
    uint16_t ubrr = (F_CPU / (8UL * BAUD)) - 1;   // U2X mode
    UBRR0H = (uint8_t)(ubrr >> 8);					//ubrr > 8 so we set on two 8bit hard reg. here HIGH 
    UBRR0L = (uint8_t)ubrr;							// here LOW
    UCSR0A = (1 << U2X0);                          // also clears leftovers from a bootloader
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);			//transceiver and receiver on.
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);        // 8N1
}

void uart_tx(char c) {
	while (!(UCSR0A & (1 << UDRE0))) {}	// wait until UDR0 is empty
	UDR0 = c;
}

void uart_printstr(const char *str) {
	while (*str)
		uart_tx(*str++);
}

char uart_rx(void) {
	while (!(UCSR0A & (1 << RXC0))) {}   // wait until a byte is received
    return UDR0;
}

uint8_t uart_readstr(char *buf, uint8_t size) {
	uint8_t i = 0;
	while (1) {
		char c = uart_rx();

		if(c == '\r' || c == '\n') {
			if (i == 0) continue;
			break;
		}
		if (c == '\b' || c == 0x7F) {	// backspace / DEL
			if (i > 0) {
				i--;
				uart_printstr("\b \b");	// erase the char on the terminal
			}
			continue;
		}
		if (i < size - 1) {
			buf[i++] = c;
			uart_tx(c);
		}
	}
	buf[i] = '\0';
	return i;
}

uint8_t check_hex(char line[BUF_MAX]) {
	if (line[0] != '#')
		return 0;
	for (int i = 1; i < 7; i++)
		if (!((line[i] >= '0' && line[i] <= '9') ||
			(line[i] >= 'A' && line[i] <= 'F') ||
			(line[i] >= 'a' && line[i] <= 'f')))
			return 0;
	return line[7] == '\0';
}

// convert one hex digit to its value (0-15), e.g. '7' -> 7, 'B' or 'b' -> 11
static uint8_t hexval(char c) {
	if (c >= '0' && c <= '9') return c - '0';			// '0'-'9' -> 0-9
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;	// 'A'-'F' -> 10-15
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;	// 'a'-'f' -> 10-15
	return 0;	// unreachable: check_hex already rejected invalid chars
}

// convert two hex digits to a byte (0-255), e.g. "A3" -> 0xA3 = 163
static uint8_t hexbyte(const char *p) {
	// first digit is the high nibble (shift it up 4 bits), second is the low nibble
	return (hexval(p[0]) << 4) | hexval(p[1]);
}

void hex2rgb(char line[BUF_MAX]) {
	uint8_t r = hexbyte(&line[1]);
	uint8_t g = hexbyte(&line[3]);
	uint8_t b = hexbyte(&line[5]);
	set_rgb(r, g, b);
}

int main () {
	uart_init();
	init_rgb();
	char line[BUF_MAX];
	
	while (1) {
		uart_readstr(line, BUF_MAX);
		uart_printstr("\r\n");
		if (check_hex(line))
			hex2rgb(line);
		else
			uart_printstr("Error: expected #RRGGBB\r\n");
	}
}
