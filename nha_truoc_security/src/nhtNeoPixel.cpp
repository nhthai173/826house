#include "nhtNeoPixel.h"




/**
 * @brief Get pixel color hsv
 *
 * @param n Position
 * @param h Hue
 * @param s Saturation
 * @param v Value (brightness)
 * @return uint32_t : color to set
 */
uint32_t nhtNeoPixel::getPixelColorHsv(uint16_t n, uint16_t h, uint8_t s, uint8_t v)
{
    uint8_t r, g, b;

    if (!s)
    {
        // Monochromatic, all components are V
        r = g = b = v;
    }
    else
    {
        uint8_t sextant = h >> 8;
        if (sextant > 5)
            sextant = 5; // Limit hue sextants to defined space

        g = v; // Top level

        // Perform actual calculations

        /*
           Bottom level:
           --> (v * (255 - s) + error_corr + 1) / 256
        */
        uint16_t ww; // Intermediate result
        ww = v * (uint8_t)(~s);
        ww += 1;       // Error correction
        ww += ww >> 8; // Error correction
        b = ww >> 8;

        uint8_t h_fraction = h & 0xff; // Position within sextant
        uint32_t d;                    // Intermediate result

        if (!(sextant & 1))
        {
            // r = ...slope_up...
            // --> r = (v * ((255 << 8) - s * (256 - h)) + error_corr1 + error_corr2) / 65536
            d = v * (uint32_t)(0xff00 - (uint16_t)(s * (256 - h_fraction)));
            d += d >> 8; // Error correction
            d += v;      // Error correction
            r = d >> 16;
        }
        else
        {
            // r = ...slope_down...
            // --> r = (v * ((255 << 8) - s * h) + error_corr1 + error_corr2) / 65536
            d = v * (uint32_t)(0xff00 - (uint16_t)(s * h_fraction));
            d += d >> 8; // Error correction
            d += v;      // Error correction
            r = d >> 16;
        }

        // Swap RGB values according to sextant. This is done in reverse order with
        // respect to the original because the swaps are done after the
        // assignments.
        if (!(sextant & 6))
        {
            if (!(sextant & 1))
            {
                uint8_t tmp = r;
                r = g;
                g = tmp;
            }
        }
        else
        {
            if (sextant & 1)
            {
                uint8_t tmp = r;
                r = g;
                g = tmp;
            }
        }
        if (sextant & 4)
        {
            uint8_t tmp = g;
            g = b;
            b = tmp;
        }
        if (sextant & 2)
        {
            uint8_t tmp = r;
            r = b;
            b = tmp;
        }
    }
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}




/**
 * @brief rainbow mode loop
 *
 */
void nhtNeoPixel::_setRainbow()
{
    for (int i = 0; i < _numpixels; i++)
    {
        _pixels.setPixelColor(
            (i + _position) % _numpixels,
            getPixelColorHsv(
                i,
                _hue,
                255,
                _pixels.gamma8(i * (255 / _numpixels))));
    }
    _pixels.show();
    _position++;
    _position %= _numpixels;
    _hue += 2;
    _hue %= MAXHUE;
}




/**
 * @brief colorfull mode loop
 *
 */
void nhtNeoPixel::_setColorFull()
{
    for (int i = 0; i < _numpixels; i++)
        _pixels.setPixelColor(
            i,
            getPixelColorHsv(
                i,
                _hue,
                255,
                _pixels.gamma8(255)));
    _pixels.show();
    _hue += 2;
    _hue %= MAXHUE;
}




/**
 * @brief police mode loop
 * 
 */
void nhtNeoPixel::_setModepolice(){
    if(_police_current  % 2 == 0)
        nhtNeoPixel::_setRGB(255, 0, 0, 1.0);
    else
        nhtNeoPixel::_setRGB(0, 0, 255, 1.0);
    _police_current++;
    if(_police_current > 200)
        _police_current = 0;
}




/**
 * @brief Set RGB color all pixels (0 - 255)
 *
 * @param r red
 * @param g green
 * @param b blue
 * @param a adjust (0 - 1)
 */
void nhtNeoPixel::_setRGB(uint8_t r, uint8_t g, uint8_t b, float a = 1.0){
    a = a > 1 ? 1 : a;
    r *= a;
    g *= a;
    b *= a;
    for(uint8_t i=0; i<_numpixels; i++){
        _pixels.setPixelColor(i, _pixels.Color((uint8_t)r, (uint8_t)g, (uint8_t)b));
        _pixels.show();
    }
}




/**
 * @brief Set HSV color all pixels
 *
 * @param h Hue (0 - 65535)
 * @param s Saturation (0 - 255)
 * @param v Value (0 - 255)
 */
void nhtNeoPixel::_setHSV(uint16_t h, uint8_t s = (uint8_t)255U, uint8_t v = (uint8_t)255U){
    for(uint8_t i=0; i<_numpixels; i++){
        _pixels.setPixelColor(i, _pixels.ColorHSV(h, s, v));
        _pixels.show();
    }
}




/**
 * @brief 
 * 
 * @param v 32bit integer value
 */
void nhtNeoPixel::_setHEX(uint32_t v){
    for(uint8_t i=0; i<_numpixels; i++){
        _pixels.setPixelColor(i, v);
        _pixels.show();
    }
}




/**
 * @brief Initial
 *
 */
void nhtNeoPixel::begin()
{
    _pixels.begin();
}




/**
 * @brief Set RGB color all pixels (0 - 255)
 *
 * @param r red
 * @param g green
 * @param b blue
 * @param a adjust (0-1)
 */
void nhtNeoPixel::setRGB(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, float a = 1.0){
    _timerInterval = -1;
    nhtNeoPixel::_setRGB(r, g, b, a);
}




/**
 * @brief Set RGB color all pixels (0 - 255)
 *
 * @param r red
 * @param g green
 * @param b blue
 */
void nhtNeoPixel::setRGB(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0){
    _timerInterval = -1;
    nhtNeoPixel::_setRGB(r, g, b, 1.0);
}




/**
 * @brief Set HSV color all pixels
 *
 * @param h Hue (0 - 65535)
 * @param s Saturation (0 - 255)
 * @param v Value (0 - 255)
 */
void nhtNeoPixel::setHSV(uint16_t h = 0, uint8_t s = (uint8_t)255U, uint8_t v = (uint8_t)255U){
    _timerInterval = -1;
    nhtNeoPixel::_setHSV(h, s, v);
}




/**
 * @brief 
 * 
 * @param v 32bit integer value
 */
void nhtNeoPixel::setHEX(uint32_t v){
    _timerInterval = -1;
    nhtNeoPixel::_setHEX(v);
}




/**
 * @brief Set rainbow mode
 *
 */
void nhtNeoPixel::setRainbow()
{
    nhtNeoPixel::setRainbow(5);
}




/**
 * @brief Set rainbow mode with speed
 *
 * @param speed speed
 */
void nhtNeoPixel::setRainbow(uint8_t speed = (uint8_t)5U)
{
    if (1 <= speed && speed <= 20)
        _timerInterval = speed * 10;
    else if (speed < 1)
        _timerInterval = 50;
    else
        _timerInterval = 50;
    _timerCallback = &nhtNeoPixel::_setRainbow;
}




/**
 * @brief Set colorfull mode
 *
 */
void nhtNeoPixel::setColorFull()
{
    nhtNeoPixel::setColorFull(5);
}




/**
 * @brief Set colorfull mode with speed
 *
 * @param speed speed
 */
void nhtNeoPixel::setColorFull(uint8_t speed = (uint8_t)5U)
{
    if (1 <= speed && speed <= 20)
        _timerInterval = speed * 10;
    else if (speed < 1)
        _timerInterval = 50;
    else
        _timerInterval = 50;
    _timerCallback = &nhtNeoPixel::_setColorFull;
}




/**
 * @brief police animate
 * 
 */
void nhtNeoPixel::policeMode()
{
    nhtNeoPixel::policeMode(4);
}




/**
 * @brief police animate with speed
 * 
 * @param speed speed
 */
void nhtNeoPixel::policeMode(uint8_t speed = (uint32_t)4)
{
    if (1 <= speed && speed <= 20)
        _timerInterval = speed * 50;
    else if (speed < 1)
        _timerInterval = 200;
    else
        _timerInterval = 200;
    _police_current = 0;
    _timerCallback = &nhtNeoPixel::_setModepolice;
}




/**
 * @brief Set color all pixels
 * 
 * @param str `red`,`green`,`blue`
 */
void nhtNeoPixel::setColor(String str){
    uint8_t c1 = str.indexOf(",");
    if(c1 > 0){
        uint8_t r = str.substring(0, c1).toInt();
        uint8_t c2 = str.indexOf(",", c1+1);
        if(c2 > c1){
            uint8_t g = str.substring(c1+1, c2).toInt();
            uint8_t b = str.substring(c2+1).toInt();
            if (!isnan(r) && !isnan(g) && !isnan(b) && str.indexOf(",", c2+1) == -1)
                nhtNeoPixel::setRGB(r, g, b, 1.0);
        }
    }else if(!isnan(str.toInt()))
        nhtNeoPixel::setHSV(str.toInt());
}




/**
 * @brief Set color hue
 * 
 * @param hue 
 */
void nhtNeoPixel::setColor(uint32_t hue){
    if(!isnan(hue))
        nhtNeoPixel::setHSV(hue);
}




/**
 * @brief Set color RGB
 * 
 * @param r red
 * @param g green
 * @param b blue
 */
void nhtNeoPixel::setColor(uint8_t r, uint8_t g, uint8_t b)
{
    if (!isnan(r) && !isnan(g) && !isnan(b))
        nhtNeoPixel::setRGB(r, g, b, 1.0);
}



/**
 * @brief Turn off all pixels
 *
 */
void nhtNeoPixel::off()
{
    _timerInterval = -1;
    nhtNeoPixel::setRGB(0, 0, 0, 1.0);
}




/**
 * @brief this function must be called inside loop()
 *
 */
void nhtNeoPixel::run()
{
    if (_timerInterval > 0 && ((millis() - _timer >= _timerInterval) ||
                               (millis() - _timer < 0)))
    {
        (this->*_timerCallback)();
        _timer = millis();
    }
}