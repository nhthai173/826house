#include <Adafruit_NeoPixel.h>
#define MAXHUE 256 * 6

class nhtNeoPixel
{
private:
    uint16_t _numpixels;
    uint16_t _position = 0;
    uint32_t _hue = 0;
    uint8_t _police_current = 0;
    unsigned long _timer;
    unsigned long _timerInterval = -1;
    void (nhtNeoPixel::*_timerCallback)();
    uint32_t getPixelColorHsv(uint16_t n, uint16_t h, uint8_t s, uint8_t v);
    void _setRainbow();
    void _setColorFull();
    void _setRGB(uint8_t r, uint8_t g, uint8_t b, float a);
    void _setHSV(uint16_t h, uint8_t s, uint8_t v);
    void _setHEX(uint32_t v);
    void _setModepolice();

public:
    Adafruit_NeoPixel _pixels;
    nhtNeoPixel(uint16_t n, int16_t pin = 6,
                 neoPixelType type = NEO_GRB + NEO_KHZ800)
    {
        _numpixels = n;
        _pixels = Adafruit_NeoPixel(n, pin, type);
    }
    void begin(void);
    void setRGB(uint8_t r, uint8_t g, uint8_t b);
    void setRGB(uint8_t r, uint8_t g, uint8_t b, float a);
    void setHSV(uint16_t h, uint8_t s, uint8_t v);
    void setHEX(uint32_t v);
    void setRainbow(void);
    void setRainbow(uint8_t speed);
    void setColorFull(void);
    void setColorFull(uint8_t speed);
    void policeMode(void);
    void policeMode(uint8_t speed);
    void setColor(String str);
    void setColor(uint32_t hue);
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void off(void);
    void run(void);
};