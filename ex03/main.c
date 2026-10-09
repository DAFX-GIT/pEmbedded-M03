#include <avr/io.h>
#include <util/delay.h>

#define BAUD 115200UL
#define BUF_MAX 32

void init_rgb() {
	DDRD |= (1 << PD6) | (1 << PD5) | (1 << PD3);

	//timer0
	TCCR0A = (1 << COM0A1) | (1 << COM0B1) | (1 << WGM01) | (1 << WGM00);
	TCCR0B = (1 << CS01) | (1 << CS00);

	//timer2
	TCCR2A = (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);
	TCCR2B = (1 << CS22);
}

uint8_t gamma_correct(uint8_t c){
	return ((uint16_t)c * c) / 255;
}

static void set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
	OCR0B = gamma_correct(r);   // red   PD5
    OCR0A = gamma_correct(g);   // green PD6
    OCR2B = gamma_correct(b);   // blue  PD3
}

void uart_init(void) {
    uint16_t ubrr = (F_CPU / (8UL * BAUD)) - 1;   // U2X mode
    UBRR0H = (uint8_t)(ubrr >> 8);					//ubrr > 8 so we set on two 8bit hard reg. here HIGH 
    UBRR0L = (uint8_t)ubrr;							// here LOW
    UCSR0A = (1 << U2X0);                          // also clears leftovers from a bootloader
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);			//transciver and receiver on.
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);        // 8N1
}

void uart_tx(char c) {
	while (!(UCSR0A & (1 << UDRE0))) {}	// wait until UDR0 is empty
	UDR0 = c;
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

static uint8_t hexval(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	return 0;
}

static uint8_t hexbyte(const char *p) {
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
		if (check_hex(line))
			hex2rgb(line);
		uart_tx('\n');
		uart_tx('\r');
	}
}
