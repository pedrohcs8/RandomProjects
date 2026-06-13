void initRTC();
void readRTC(void);
void writeRTC(uint8_t sec, uint8_t min, uint8_t hour, uint8_t date, uint8_t month, uint8_t year);

extern volatile uint8_t time_buf[7];
extern volatile int counter;