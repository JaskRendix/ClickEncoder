// -----------------------------------------------------------------------------
// BoundedMenu
//
// Uses setBounds()/wrap-around to turn the encoder into a wrapping menu
// selector -- rotating past the last item wraps to the first, and vice
// versa. Clicking "confirms" the highlighted item.
//
// This is one of the most common real-world uses of a rotary encoder
// (settings menus, mode selectors, etc.), so it's a good template to
// copy from directly.
// -----------------------------------------------------------------------------

#include <ClickEncoder.h>

static constexpr uint8_t ENCODER_PIN_A = 2;
static constexpr uint8_t ENCODER_PIN_B = 3;
static constexpr int16_t ENCODER_PIN_BTN = 4;

const char *const MENU_ITEMS[] = {
    "Brightness", "Contrast", "Volume", "Bluetooth", "About",
};
constexpr int16_t MENU_COUNT = sizeof(MENU_ITEMS) / sizeof(MENU_ITEMS[0]);

ClickEncoder *encoder;
int16_t lastShown = -1;

void printMenu() {
  int16_t idx = encoder->getCurrentValue();
  Serial.print("> ");
  Serial.println(MENU_ITEMS[idx]);
}

void setup() {
  Serial.begin(115200);

  ClickEncoder::Config cfg{ENCODER_PIN_A, ENCODER_PIN_B, ENCODER_PIN_BTN};
  encoder = new ClickEncoder(cfg);

  // Menu indices 0..MENU_COUNT-1, wrapping past either end.
  encoder->setBounds(0, MENU_COUNT - 1, /* wrapAround = */ true);
  encoder->setCurrentValue(0);

  Serial.println("BoundedMenu ready. Rotate to browse, click to select.");
  printMenu();
  lastShown = encoder->getCurrentValue();
}

void loop() {
  encoder->service();

  // getValue() also advances/clamps/wraps trackAbsoluteValue internally
  // when bounds are set, so just polling it each loop is enough to keep
  // getCurrentValue() in sync -- no manual arithmetic needed here.
  encoder->getValue();

  int16_t idx = encoder->getCurrentValue();
  if (idx != lastShown) {
    lastShown = idx;
    printMenu();
  }

  if (encoder->getButton() == ClickEncoder::ButtonState::Clicked) {
    Serial.print("Selected: ");
    Serial.println(MENU_ITEMS[idx]);
  }
}
