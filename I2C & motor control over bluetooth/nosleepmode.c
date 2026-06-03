#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>     // atoi, itoa

/*  DEFINES */
#define MMA8452_ADDR  0x1C
#define REG_CTRL1     0x2A
#define X_MSB         0x01
#define Z_MSB         0x05

/*  GLOBALS */
volatile char data[10];
volatile uint8_t pointer = 0;
volatile uint8_t finish = 0;
volatile int16_t servo_angle = 90;

/* UART */
void uart_send_string(const char *s)
{
    while (*s)
    {
        while (!(UCSR0A & (1 << UDRE0)));
        UDR0 = *s++;
    }
}

void uart_init(void)
{
    UBRR0 = 103;                        // 9600 baud @ 16 MHz
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

/* TWI / I2C  */
void TWI_start(void)
{
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}

void TWI_write(uint8_t tdata)
{
    TWDR = tdata;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}

uint8_t TWI_read_ack(void)
{
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
    while (!(TWCR & (1 << TWINT)));
    return TWDR;
}

uint8_t TWI_read_nack(void)
{
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
    return TWDR;
}

void TWI_stop(void)
{
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

/*  SERVO */
void servo_init(void)
{
    DDRB |= (1 << PB1);                 // OC1A
    TCCR1A = (1 << COM1A1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11); // prescaler 8
    ICR1  = 39999;                      // 50 Hz
    OCR1A = 3000;                       // start ~90°
}

/* Fast integer atan2 approximation (-90°...+90°) */
/* Returns angle in degrees * 10 → 900 = 90.0°    */
static int16_t atan2_int(int16_t y, int16_t x)
{
    if (x == 0 && y == 0) return 0;

    int16_t abs_y = y;
    if (abs_y < 0) abs_y = -abs_y;

    int16_t abs_x = x;
    if (abs_x < 0) abs_x = -abs_x;

    int16_t angle;

    if (x >= abs_y) // |x| >= |y|
    {
        angle = (45 * y) / x;
        if (angle < 0) angle = -angle;
        angle = 450 - angle;            // 45° * 10
    }
    else // |y| > |x|
    {
        angle = (45 * x) / y;
        if (angle < 0) angle = -angle;
        angle = 900 - angle;
    }

    // Fix quadrant
    if (y >= 0)
    {
        if (x < 0) angle = 1800 - angle;
    }
    else
    {
        if (x >= 0) angle = -angle;
        else        angle = -1800 + angle;
    }

    return angle;   // angle in 0.1° units
}

/*  ISR  */
ISR(USART_RX_vect)
{
    char received = UDR0;

    if (received >= '0' && received <= '9')
    {
        if (pointer < 9)
            data[pointer++] = received;
    }
    else if (received == '\r' || received == '\n' || received == ' ')
    {
        if (pointer > 0)
        {
            data[pointer] = '\0';
            finish = 1;
        }
        pointer = 0;
    }
}

/*  MAIN  */
int main(void)
{
    char msg[16];
    uint8_t h_byte, l_byte;
    int16_t x_axis, z_axis;
    int16_t tilt_10deg;

    uart_init();
    servo_init();

    // TWI 100 kHz @ 16 MHz
    TWBR = 72;
    TWCR = (1 << TWEN);

    // MMA8452 → active mode, ±2g
    TWI_start();
    TWI_write(MMA8452_ADDR << 1);
    TWI_write(REG_CTRL1);
    TWI_write(0x01);
    TWI_stop();

    sei();

    while (1)
    {
        if (finish)
        {
            int angle = atoi((char *)data);

            if (angle >= 0 && angle <= 180)
            {
                // Servo pulse: ~544µs .. ~2400µs → 1000..4800 counts @ 2MHz timer
                OCR1A = 1000 + (angle * 21);    // rough but common approximation


                //  Read X
                TWI_start();
                TWI_write(MMA8452_ADDR << 1);
                TWI_write(X_MSB);
                TWI_start();
                TWI_write((MMA8452_ADDR << 1) | 1);
                h_byte = TWI_read_ack();
                l_byte = TWI_read_nack();
                TWI_stop();

                x_axis = ((int16_t)h_byte << 8) | l_byte;
                x_axis >>= 4;
                if (x_axis & 0x0800) x_axis |= 0xF000;

                //  Read Z 
                TWI_start();
                TWI_write(MMA8452_ADDR << 1);
                TWI_write(Z_MSB);
                TWI_start();
                TWI_write((MMA8452_ADDR << 1) | 1);
                h_byte = TWI_read_ack();
                l_byte = TWI_read_nack();
                TWI_stop();

                z_axis = ((int16_t)h_byte << 8) | l_byte;
                z_axis >>= 4;
                if (z_axis & 0x0800) z_axis |= 0xF000;

                // Integer atan2 approximation
                tilt_10deg = atan2_int(x_axis, z_axis);

                // Convert to classic -90°...+90° → 0°...180° servo range
                servo_angle = (tilt_10deg / 10) + 90;

                if (servo_angle < 0)   servo_angle = 0;
                if (servo_angle > 180) servo_angle = 180;

                // Feedback
                itoa(servo_angle, msg, 10);
                uart_send_string("Inclination: ");
                uart_send_string(msg);
                uart_send_string(" deg\r\n");

                finish = 0;
            }
        }
    }

    return 0;
}
