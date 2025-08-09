#include "passcode.h"

const char secretPasscode[PASSCODE_LENGTH] = {'1', '2', '3', '4'};
char inputPasscode[PASSCODE_LENGTH];
byte inputPasscodeLength = 0;

void displayPasscode()
{
  esp_rom_printf("input(%d): ", inputPasscodeLength);
  for (byte i = 0; i < inputPasscodeLength; i++)
  {
    esp_rom_printf("%c", inputPasscode[i]);
  }
  esp_rom_printf("\n");
}

void appendPasscode(char chr)
{
  // check if character is a digit
  if (!(chr >= '0' && chr <= '9'))
  {
    return;
  }

  // check if input passcode is not complete
  if (inputPasscodeLength == PASSCODE_LENGTH)
  {
    return;
  }

  // add character to passcode and increase it's length
  inputPasscode[inputPasscodeLength++] = chr;

  // print passcode
  displayPasscode();
}

void popPasscode()
{
  // check that the inputPasscode isn't empty
  if (inputPasscodeLength)
    // go back one character
    --inputPasscodeLength;

  // print passcode
  displayPasscode();
}

void onValidPasscode()
{
  // reset input passcode
  inputPasscodeLength = 0;

  esp_rom_printf("Valid passcode!\n");
}
void onInvalidPasscode()
{
  // reset input passcode
  inputPasscodeLength = 0;

  esp_rom_printf("Invalid passcode!\n");
}

bool validatePasscode()
{
  // ensure the passcode is complete
  if (inputPasscodeLength < PASSCODE_LENGTH)
  {
    onInvalidPasscode();

    return false;
  }

  for (byte i = 0; i < PASSCODE_LENGTH; i++)
  {
    if (inputPasscode[i] != secretPasscode[i])
    {
      onInvalidPasscode();
      return false;
    }
  }

  onValidPasscode();
  return true;
}