/*
    Program : I2Ctuff (part of I2C_Tiny841_Slave)

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

//------------------------------------------------------------------
void startI2C()
{
  Wire.end();

  Wire.begin(registerStack.address);

  // (Re)Declare the Events.
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

} // startI2C()


//------------------------------------------------------------------
boolean isConnected()
{
  Wire.beginTransmission((uint8_t)0);
  if (Wire.endTransmission() != 0) {
    return (false); // Master did not ACK
  }
  return (true);

} // isConnected()


//------------------------------------------------------------------
void normalizeSettings()
{
  byte modeReg = registerStack.modeSettings;

  if ((modeReg & (1<<STNG_HWROTDIR))) {
    hwRotDir    =  1;
    modeRotDir  =  1;
  } else {
    hwRotDir    = -1;
    modeRotDir  =  1;
  }

  if ((modeReg & (1<<STNG_FLIPMODE))) {
    modeReg &= ~(1 <<STNG_TURNMODE);
  }
  if ((modeReg & (1<<STNG_TURNMODE))) {
    modeReg &= ~(1 <<STNG_FLIPMODE);
  }
  registerStack.modeSettings = modeReg & 0xFF;
  if (registerStack.rotMin < -5000)         registerStack.rotMin = -5000;
  if (registerStack.rotMin >  5000)         registerStack.rotMin =  5000;
  if (registerStack.rotMax < -5000)         registerStack.rotMax = -5000;
  if (registerStack.rotMax >  5000)         registerStack.rotMax =  5000;
  if (registerStack.rotMin > registerStack.rotMax)  
                                            registerStack.rotMin =     0;
  if (registerStack.rotMax < registerStack.rotMin)  
                                            registerStack.rotMax =  5000;
  if (registerStack.rotVal < registerStack.rotMin)  
                                            registerStack.rotVal = registerStack.rotMin;
  if (registerStack.rotVal > registerStack.rotMax)  
                                            registerStack.rotVal = registerStack.rotMax;
  if (registerStack.rotStep > 100)          registerStack.rotStep       =    50;
  if (registerStack.rotStep <   1)          registerStack.rotStep       =     1;
  if (registerStack.rotSpinTime > 100)      registerStack.rotSpinTime   =   100;
  if (registerStack.rotSpinTime <   2)      registerStack.rotSpinTime   =     2;
  if (registerStack.debounceTime > 250)     registerStack.debounceTime  =   250;
  if (registerStack.debounceTime <   5)     registerStack.debounceTime  =     5;
  if (registerStack.midPressTime  < 100)    registerStack.midPressTime  =   100;             
  if (registerStack.midPressTime  > 5000)   registerStack.midPressTime  =  5000;             
  if (registerStack.longPressTime < 300)    registerStack.longPressTime =   300;             
  if (registerStack.longPressTime > 10000)  registerStack.longPressTime = 10000;             
  if (registerStack.midPressTime  > registerStack.longPressTime)             
                    registerStack.midPressTime = 250; 
  if (registerStack.longPressTime < registerStack.midPressTime)             
                    registerStack.longPressTime = (2*registerStack.midPressTime);

} // normalizeSettings()


//------------------------------------------------------------------
void processCommand(byte command)
{
  if ((command & (1<<CMD_WRITECONF))) {
    writeConfig();
  }
  if ((command & (1<<CMD_READCONF))) {
    readConfig();
  }
  //-----> execute reBoot always last!! <-----
  if ((command & (1<<CMD_REBOOT))) {
    reBoot();
  }

} // processCommand()

//------------------------------------------------------------------
//-- The master sends updated info that will be stored in the ------
//-- register(s)
//-- All Setters end up here ---------------------------------------
void receiveEvent(int numberOfBytesReceived)
{
  aliveTimer = millis();
  registerNumber = Wire.read(); //Get the memory map offset from the user

  if (registerNumber == _CMD_REGISTER) {   // eeprom command
    byte command = Wire.read(); // read the command
    processCommand(command);
    return;
  }
  if (registerNumber == _MODESETTINGS) {   // mode Settings
    normalizeSettings();
  }

  //Begin recording the following incoming bytes to the temp memory map
  //starting at the registerNumber (the first byte received)
  for (byte x = 0 ; x < numberOfBytesReceived - 1 ; x++) {
    uint8_t temp = Wire.read();
    if ( (x + registerNumber) < sizeof(registerLayout)) {
      //Store the result into the register map
      registerPointer[registerNumber + x] = temp;
    }
  }
  if (registerNumber == _MODESETTINGS) {   // mode Settings
    normalizeSettings();
  }

} //  receiveEvent()

//------------------------------------------------------------------
//-- The master aks's for the data from registerNumber onwards -----
//-- in the register(s) --------------------------------------------
//-- All getters get there data from here --------------------------
void requestEvent()
{
  aliveTimer = millis();
  //----- write max. 4 bytes starting at registerNumber -------
  for (uint8_t x = 0; ( (x < 4) &&(x + registerNumber) < (sizeof(registerLayout) - 1) ); x++) {
    Wire.write(registerPointer[(x + registerNumber)]);
    if ((registerNumber + x) == 0) { // status register!!!
      registerPointer[registerNumber + x] = 0x00; // reset!
    }
  }
} // requestEvent()


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
