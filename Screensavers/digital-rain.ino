#include "Arduino.h"
#include "Arduino_GFX_Library.h"

#define GFX_DEV_DEVICE LILYGO_T_DISPLAY_S3
#define GFX_EXTRA_PRE_INIT()              \
    {                                     \
        pinMode(15 /* PWD */, OUTPUT);    \
        digitalWrite(15 /* PWD */, HIGH); \
    }
#define GFX_BL 38
Arduino_DataBus *bus = new Arduino_ESP32PAR8Q(
    7 /* DC */, 6 /* CS */, 8 /* WR */, 9 /* RD */,
    39 /* D0 */, 40 /* D1 */, 41 /* D2 */, 42 /* D3 */, 45 /* D4 */, 46 /* D5 */, 47 /* D6 */, 48 /* D7 */);
Arduino_GFX *gfx = new Arduino_ST7789(bus, 5 /* RST */, 0 /* rotation */, true /* IPS */, 170 /* width */, 320 /* height */, 35 /* col offset 1 */, 0 /* row offset 1 */, 35 /* col offset 2 */, 0 /* row offset 2 */);


int32_t w, h, n, n1, cx, cy, cx1, cy1, cn, cn1;
uint8_t tsa, tsb, tsc, ds;

// Vars
unsigned char pixels[53][21];       // Matrix of pixels
unsigned char pixelLifetime[53];    // Overall column lifetime
unsigned char colours[53];          // Column colours
bool flag = false;
unsigned int columnTotal[53];       // Total value of column

void setup()
{

    GFX_EXTRA_PRE_INIT();

#ifdef GFX_BL
    pinMode(GFX_BL, OUTPUT);
    digitalWrite(GFX_BL, HIGH);
#endif

    Serial.begin(115200);
    Serial.println("Hello T-Display-S3");

    gfx->begin();
    gfx->setRotation(1);

    w = gfx->width();
    h = gfx->height();
    n = min(w, h);
    n1 = n - 1;
    cx = w / 2;
    cy = h / 2;
    cx1 = cx - 1;
    cy1 = cy - 1;
    cn = min(cx1, cy1);
    cn1 = cn - 1;
    tsa = ((w <= 176) || (h <= 160)) ? 1 : (((w <= 240) || (h <= 240)) ? 2 : 3); // text size A
    tsb = ((w <= 272) || (h <= 220)) ? 1 : 2;                                    // text size B
    tsc = ((w <= 220) || (h <= 220)) ? 1 : 2;                                    // text size C
    ds = (w <= 160) ? 9 : 12;                                                    // digit size

    memset(pixelLifetime, 0, sizeof(pixelLifetime));
    memset(columnTotal, 0, sizeof(columnTotal));
}

static inline uint32_t micros_start() __attribute__((always_inline));
static inline uint32_t micros_start()
{
    uint8_t oms = millis();
    while ((uint8_t)millis() == oms)
        ;
    return micros();
}


void loop(void)
{
    // Reset Screen
    // Swap with blanking out an old pixel
    gfx->fillScreen(0x0);

    // Loop through Rows
    for (char i = 0 ; i < 53 ; i++) {
        // Have we hit a pixel yet
        flag = false;

        // Empty Column -> set new lifetime and write first pixel
        if (columnTotal[i] == 0 && (rand() % 25 == 0)) {
            pixelLifetime[i] = 3 + (rand() % 7);
            pixels[i][0] = pixelLifetime[i];
            columnTotal[i] += pixelLifetime[i];
        } else {
            // Loop through Columns
            for (char j = 0 ; j < 21 ; j++) {
                if ((pixels[i][j] == 0) && flag) {
                    // This probably needs a timer as well so it doesn't just keep going
                    // Well this should be tied to frame rate I guess
                    // If we have a blank pixel after seeing a non blank pixel add a new pixel and exit
                    pixels[i][j] = pixelLifetime[i];
                    columnTotal[i] += pixelLifetime[i];
                    break;
                } else if (pixels[i][j] != 0) {
                    // Otherwise if we have a pixel draw and update it
                    drawPixel(i, j, colours[i], pixels[i][j]);
                    pixels[i][j] -= 1;
                    columnTotal[i] -= 1;
                    flag = true;
                }
            } 
        }
    }

    // Sync with Screen
    delay(60 * 1L);
}

uint16_t editBrightness(uint16_t colour, uint16_t value, uint16_t maxValue) {
    
    uint16_t red = (((colour & 0xF800) >> 11) / maxValue) * value;
    uint16_t green = (((colour & 0x7E0) >> 5) / maxValue) * value;
    uint16_t blue = ((colour & 0x1F) / maxValue) * value;

    colour = (red << 11) | (green << 5) | blue;

    return colour;
}

// Draw pixel
void drawPixel(unsigned char x, unsigned char y, unsigned char inputColour, unsigned char value) {
    // Adjust brightness
    uint16_t colour = editBrightness(0x79F, value, pixelLifetime[x]);

    // Character needs to be randomised
    unsigned char character = (char) (rand() % 255);

    // Calculate x,y positions
    uint16_t xPosition = 2 + (x * 6);
    uint16_t yPosition = 1 + (y * 8);

    // Use graphics libraries
    gfx->drawChar(xPosition, yPosition, character, colour, 0);
}

void serialOut(const __FlashStringHelper *item, int32_t v, uint32_t d, bool clear)
{
#ifdef CANVAS
    gfx->flush();
#endif
    Serial.print(item);
    if (v < 0) {
        Serial.println(F("N/A"));
    } else {
        Serial.println(v);
    }
    delay(d);
    if (clear) {
        gfx->fillScreen(BLACK);
    }
}