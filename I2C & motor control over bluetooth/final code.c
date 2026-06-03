#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

/* My project: Phone controls servo over Bluetooth + I measure platform tilt */

volatile uint8_t target_angle = 90;     // Angle the phone sends (0-180)
volatile uint8_t meas_flag   = 0;        // Timer sets this when it's time to measure tilt

/* My own UART functions */
void uart_init(void) {
    UBRR0H = 0;
    UBRR0L = 51;                        // 9600 baud at 8 MHz
    UCSR0B = (1<<RXEN0) | (1<<TXEN0) | (1<<RXCIE0);  // Enable RX, TX and RX interrupt
    UCSR0C = (1<<UCSZ01) | (1<<UCSZ00); // 8-bit data
}

void uart_tx(uint8_t data) {
    while (!(UCSR0A & (1<<UDRE0)));      // Wait until buffer is free
    UDR0 = data;                         // Send the byte
}

/* Runs when phone sends a new angle */
ISR(USART_RX_vect) {
    target_angle = UDR0;                // Save the angle right away
}

/* Servo PWM setup */
void servo_init(void) {
    DDRB |= (1<<PB1);                   // PB1 (D9) as output for servo
    
    ICR1 = 19999;                       // 20 ms period = 50 Hz
    TCCR1A = (1<<COM1A1) | (1<<WGM11);
    TCCR1B = (1<<WGM13) | (1<<WGM12) | (1<<CS11) | (1<<CS10); // prescaler 64
    
    OCR1A = 188;                        // Start at 90° (~1.5 ms pulse)
}

/* Change servo position – called only when new angle arrives */
void servo_set_angle(void) {
    uint16_t pulse = 125 + (uint16_t)target_angle * 125 / 180;  // 125 counts = 1 ms, 250 = 2 ms
    OCR1A = pulse;
}

/* My own I2C functions */
uint8_t i2c_start(void) {
    TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));
    return (TWSR & 0xF8);
}

void i2c_stop(void) {
    TWCR = (1<<TWINT) | (1<<TWSTO) | (1<<TWEN);
}

uint8_t i2c_write(uint8_t data) {
    TWDR = data;
    TWCR = (1<<TWINT) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));
    return (TWSR & 0xF8);
}

uint8_t i2c_read(uint8_t ack) {
    TWCR = (1<<TWINT) | (1<<TWEN) | (ack<<TWEA);
    while (!(TWCR & (1<<TWINT)));
    return TWDR;
}

/* Read X, Y, Z from accelerometer */
void accel_read(int16_t *x, int16_t *y, int16_t *z) {
    i2c_start();
    i2c_write(0x1D << 1);
    i2c_write(0x01);
    i2c_start();
    i2c_write((0x1D << 1) | 1);
    
    uint8_t xh = i2c_read(1);
    uint8_t xl = i2c_read(1);
    uint8_t yh = i2c_read(1);
    uint8_t yl = i2c_read(1);
    uint8_t zh = i2c_read(1);
    uint8_t zl = i2c_read(0);
    
    i2c_stop();
    
    *x = (int16_t)(xh << 8 | xl) >> 4;
    *y = (int16_t)(yh << 8 | yl) >> 4;
    *z = (int16_t)(zh << 8 | zl) >> 4;
}

/* Simple calculation to get pitch angle (0-180) */
uint8_t calc_pitch(int16_t ax, int16_t ay, int16_t az) {
    int32_t denom = (int32_t)ay * ay + (int32_t)az * az;
    if (denom == 0) return 90;
    
    int16_t ratio = (ax * 100L) / (denom >> 10);
    if (ratio > 100) ratio = 100;
    if (ratio < -100) ratio = -100;
    
    return (uint8_t)(90 + (ratio * 57 / 100));
}

/* Timer interrupt – just raises flag for measurement */
ISR(TIMER0_OVF_vect) {
    meas_flag = 1;
}

/* Main loop – only the required infinite loop */
int main(void) {
    servo_init();
    uart_init();
    
    TWSR = 0;
    TWBR = 72;                          // 100 kHz I2C
    TWCR = (1<<TWEN);
    
    /* Turn accelerometer on */
    i2c_start(); i2c_write(0x1D << 1); i2c_write(0x2A); i2c_write(0x00); i2c_stop();
    i2c_start(); i2c_write(0x1D << 1); i2c_write(0x2A); i2c_write(0x01); i2c_stop();
    
    TCCR0B = (1<<CS02) | (1<<CS00);     // prescaler 1024
    TIMSK0 = (1<<TOIE0);
    
    sei();
    
    set_sleep_mode(SLEEP_MODE_IDLE);
    
    while (1) {
        sleep_mode();                   // Sleep until interrupt wakes me
        
        servo_set_angle();              // Update servo if new angle arrived
        
        if (meas_flag) {                // Measure and send tilt if timer flagged
            meas_flag = 0;
            int16_t ax, ay, az;
            accel_read(&ax, &ay, &az);
            uint8_t pitch = calc_pitch(ax, ay, az);
            uart_tx('P');
            uart_tx(pitch);
        }
    }
    
    return 0;
}
