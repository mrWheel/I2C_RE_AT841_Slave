/*
    Program : EepromStuff (part of I2C_Tiny84_Slave)

    Copyright (C) 2019 Willem Aandewiel

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

//--------------------------------------------------------------------------
static void readConfig()
{
  registerLayout registersSaved;

  analogWrite(_GRNLED,  _LED_OFF);
  analogWrite(_BLUELED, _LED_OFF);
  analogWrite(_REDLED,  _LED_OFF);
  eeprom_read_block(&registersSaved, 0, sizeof(registersSaved));
  //--- the registerStack will not change in the same Major Version --
  if ( registersSaved.majorRelease == _MAJOR_VERSION ) {
    registerStack = registersSaved;
    analogWrite(_GRNLED, _LED_ON);

  //--- the Major version has changed and there is no way of -----
  //--- knowing the registerStack has not changed, so rebuild ----
  } else {  
    registerStack.address       = _I2C_DEFAULT_ADDRESS;
    registerStack.majorRelease  = _MAJOR_VERSION;
    registerStack.minorRelease  = _MINOR_VERSION;
    registerStack.rotVal        =     0;
    registerStack.rotStep       =     1;
    registerStack.rotMin        =     0;
    registerStack.rotMax        =   255;
    registerStack.rotSpinTime   =    10;
    registerStack.pwmRed        =     0;
    registerStack.pwmGreen      =     0;
    registerStack.pwmBlue       =     0;
    registerStack.debounceTime  =    10;
    registerStack.midPressTime  =   500;
    registerStack.longPressTime =  1500;
    registerStack.modeSettings  =     0;

    analogWrite(_REDLED, _LED_ON);
    writeConfig();
  }

  normalizeSettings();
  newRotVal = registerStack.rotVal;

} // readConfig()

//--------------------------------------------------------------------------
static void writeConfig()
{

  eeprom_update_block(&registerStack, 0, sizeof(registerStack));
//eeprom_write_block(&registerStack, 0, sizeof(registerStack));

  analogWrite(_GRNLED, _LED_ON);

} // writeConfig()


/***************************************************************************
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to permit
* persons to whom the Software is furnished to do so, subject to the
* following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
* OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
* THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
***************************************************************************/
