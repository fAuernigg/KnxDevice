//    This file is part of Arduino Knx Bus Device library.

//    The Arduino Knx Bus Device library allows to turn Arduino into "self-made" KNX bus device.
//    Copyright (C) 2014 2015 2016 Franck MARINI (fm@liwan.fr)

//    The Arduino Knx Bus Device library is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.

//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.


// File : StKnxCoupler.h
// Author : Franz Auernigg
// Description : Communication with StKnxCoupler Chip
// Module dependencies : KnxTelegram, KnxComObject

// This library supports both TPUART version 1 and 2
// The Siemens KNX TPUART version 1 datasheet is available at :
// http://www.hqs.sbt.siemens.com/Lowvoltage/gamma_product_data/gamma-b2b/tpuart.pdf
// The Siemens KNX TPUART version 2 datasheet is available at :
// http://www.hqs.sbt.siemens.com/Lowvoltage/gamma_product_data/gamma-b2b/TPUART2_technical-data.pdf

#ifndef STKNX_H
#define STKNX_H

#include "Arduino.h"
#include "KnxTelegram.h"
#include "KnxComObject.h"
#include "KnxBusCoupler.h"

// !!!!!!!!!!!!!!! FLAG OPTIONS !!!!!!!!!!!!!!!!!
// DEBUG :
// #define KNXTPUART_DEBUG_INFO   // Uncomment to activate info traces
// #define KNXTPUART_DEBUG_ERROR  // Uncomment to activate error traces


class StKnxCoupler : public KnxBusCoupler {
    const type_TransmitCallbackFctPtr _extTxCb;
    const word _physicalAddr;                 // Physical address set in the Bus Coupler
    const type_KnxBusCouplerMode _mode;       // Bus Coupler working Mode (Normal/Bus Monitor)
    type_buscoupler_rx _rx;                   // Reception structure
    type_buscoupler_tx _tx;                   // Transmission structure
    type_EventCallbackFctPtr _evtCallbackFct; // Pointer to the EVENTS callback function
    KnxComObject **_comObjectsList;           // Attached list of com objects
    byte _assignedComObjectsNb;               // Nb of assigned com objects
    byte *_orderedIndexTable;                 // Table containing the assigned com objects indexes ordered by increasing @
    byte _stateIndication;                    // Value of the last received state indication

#if defined(KNXTPUART_DEBUG_INFO) || defined(KNXTPUART_DEBUG_ERROR)
    String *_debugStrPtr;
#endif

#ifdef KNXTPUART_DEBUG_INFO
static const char _debugInfoText[];
#endif
#ifdef KNXTPUART_DEBUG_ERROR
static const char _debugErrorText[];
#endif

  public:

  // Constructor / Destructor
    StKnxCoupler(type_TransmitCallbackFctPtr cb, word physicalAddr,
          type_KnxBusCouplerMode _mode);
    ~StKnxCoupler();


  // INLINED functions (see definitions later in this file)

    // Set EVENTs callback function
    // return KNX_BUSCOUPLER_ERROR (255) if the parameter is NULL
    // return KNX_BUSCOUPLER_ERROR_NOT_INIT_STATE (254) if the TPUART is not in Init state
    // else return OK
    // The function must be called prior to Init() execution
    byte SetEvtCallback(type_EventCallbackFctPtr);

    void SetReceivedTelegram(KnxTelegram &telegram);

    // Set ACK callback function
    // return KNX_BUSCOUPLER_ERROR (255) if the parameter is NULL
    // return KNX_BUSCOUPLER_ERROR_NOT_INIT_STATE (254) if the TPUART is not in Init state
    // else return OK
    // The function must be called prior to Init() execution
    byte SetAckCallback(type_AckCallbackFctPtr);

    // Get the value of the last received State Indication
    // NB : every state indication value change is notified by a "BUSCOUPLER_EVENT_STATE_INDICATION" event
    byte GetStateIndication(void) const;

    // Get the reference to the telegram received by the TPUART
    // NB : every received telegram content change is notified by a "BUSCOUPLER_EVENT_RECEIVED_EIB_TELEGRAM" event
    KnxTelegram& GetReceivedTelegram(void);

    // Get the index of the com object targeted by the last received telegram
    byte GetTargetedComObjectIndex(void) const;

    // returns true if there is an activity ongoing (RX/TX) on the TPUART
    // false when there's no activity or when the tpuart is not initialized
    boolean IsActive(void) const;

#if defined(KNXTPUART_DEBUG_INFO) || defined(KNXTPUART_DEBUG_ERROR)
    // Set the string used for debug traces
    void SetDebugString(String *strPtr);
#endif

  // Functions NOT INLINED
    // Reset the Arduino UART port and the TPUART device
    // Return KNX_BUSCOUPLER_ERROR in case of TPUART reset failure
    byte Reset(void);

    // Attach a list of com objects
    // NB1 : only the objects with "communication" attribute are considered by the TPUART
    // NB2 : In case of objects with identical address, the object with highest index only is considered
    // return KNX_BUSCOUPLER_ERROR_NOT_INIT_STATE (254) if the TPUART is not in Init state
    // The function must be called prior to Init() execution
    byte AttachComObjectsList(KnxComObject KnxComObjectsList[], byte listSize);

    byte AttachComObjectsList(KnxComObject** comObjectsList, byte listSize);

    // Init
    // returns ERROR (255) if the TP-UART is not in INIT state, else returns OK (0)
    // Init must be called after every reset() execution
    byte Init(void);

    // Send a KNX telegram
    // returns ERROR (255) if TX is not available or if the telegram is not valid, else returns OK (0)
    // NB : the source address is forced to TPUART physical address value
    byte SendTelegram(KnxTelegram& sentTelegram);

    // Reception task
    // This function shall be called periodically in order to allow a correct reception of the EIB bus data
    // Assuming the TPUART speed is configured to 19200 baud, a character (8 data + 1 start + 1 parity + 1 stop)
    // is transmitted in 0,58ms.
    // In order not to miss any End Of Packets (i.e. a gap from 2 to 2,5ms), the function shall be called at a max period of 0,5ms.
    // Typical calling period is 400 usec.
    void RXTask(void);

    // Transmission task
    // This function shall be called periodically in order to allow a correct transmission of the EIB bus data
    // Assuming the TP-Uart speed is configured to 19200 baud, a character (8 data + 1 start + 1 parity + 1 stop)
    // is transmitted in 0,58ms.
    // Sending one byte of a telegram consists in transmitting 2 characters (1,16ms)
    // Let's wait around 800us between each telegram piece sending so that the 64byte TX buffer remains almost empty.
    // Typical calling period is 800 usec.
    void TXTask(void);

    // Get Bus monitoring data (BUS MONITORING mode)
    // The function returns true if a new data has been retrieved (data pointer in argument), else false
    // It shall be called periodically (max period of 0,5ms) in order to allow correct data reception
    // Typical calling period is 400 usec.
    boolean GetMonitoringData(type_MonitorData&);

    // DEBUG purpose functions
    void DEBUG_SendResetCommand(void);
    void DEBUG_SendStateReqCommand(void);

  private:
  // Private INLINED functions (see definitions later in this file)
#if defined(KNXTPUART_DEBUG_INFO)
    void DebugInfo(const char[]) const;
#endif
#if defined(KNXTPUART_DEBUG_ERROR)
    void DebugError(const char[]) const;
#endif

  // Private NOT INLINED functions
    // Check if the target address points to an assigned com object (i.e. the target address equals a com object address)
    // if yes, then update index parameter with the index (in the list) of the targeted com object and return true
    // else return false
    boolean IsAddressAssigned(word addr, byte &index) const;
};


// ----- Definition of the INLINED functions :  ------------

inline byte StKnxCoupler::SetEvtCallback(type_EventCallbackFctPtr evtCallbackFct)
{
  if (evtCallbackFct == NULL) return KNX_BUSCOUPLER_ERROR;
  if ((_rx.state!=RX_INIT) || (_tx.state!=TX_INIT)) return KNX_BUSCOUPLER_ERROR_NOT_INIT_STATE;
  _evtCallbackFct = evtCallbackFct;
  return KNX_BUSCOUPLER_OK;
}

inline byte StKnxCoupler::SetAckCallback(type_AckCallbackFctPtr ackFctPtr)
{
  if (ackFctPtr == NULL) return KNX_BUSCOUPLER_ERROR;
  if ((_rx.state!=RX_INIT) || (_tx.state!=TX_INIT)) return KNX_BUSCOUPLER_ERROR_NOT_INIT_STATE;
  _tx.ackFctPtr = ackFctPtr;
  return KNX_BUSCOUPLER_OK;
}

inline byte StKnxCoupler::GetStateIndication(void) const { return _stateIndication; }

inline KnxTelegram& StKnxCoupler::GetReceivedTelegram(void)
{ return _rx.receivedTelegram; }


inline byte StKnxCoupler::GetTargetedComObjectIndex(void) const
{ return _rx.addressedComObjectIndex; } // return the index of the adress addressed by the received KNX Telegram


inline boolean StKnxCoupler::IsActive(void) const
{
  if ( _rx.state > RX_IDLE_WAITING_FOR_CTRL_FIELD) return true; // Rx activity
  if ( _tx.state > TX_IDLE) return true; // Tx activity
  return false;
}


#if defined(KNXTPUART_DEBUG_INFO) || defined(KNXTPUART_DEBUG_ERROR)
inline void StKnxCoupler::SetDebugString(String *strPtr)
{
   _debugStrPtr = strPtr;
}
#endif


#if defined(KNXTPUART_DEBUG_INFO)
inline void StKnxCoupler::DebugInfo(const char comment[]) const
{
  if (_debugStrPtr != NULL) *_debugStrPtr += String(_debugInfoText) + String(comment);
}
#endif


#if defined(KNXTPUART_DEBUG_ERROR)
inline void StKnxCoupler::DebugError(const char comment[]) const
{
  if (_debugStrPtr != NULL) *_debugStrPtr += String(_debugErrorText) + String(comment);
}
#endif

#endif // STKNX_H
