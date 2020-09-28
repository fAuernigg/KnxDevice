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


// File : KnxBusCoupler.h
// Author : Franz Auernigg
// Description : Interface two select between TpUart and StKnxCoupler Chip
// Module dependencies : KnxTelegram, KnxComObject

#ifndef KNXBUSCOUPLER_H
#define KNXBUSCOUPLER_H

#include "Arduino.h"
#include "KnxTelegram.h"
#include "KnxComObject.h"



// Definition of the TP-UART events sent to the application layer
enum e_KnxBusCouplerEvent {
  BUSCOUPLER_EVENT_RESET = 0,                    // reset received from the bus coupler device
  BUSCOUPLER_EVENT_RECEIVED_EIB_TELEGRAM,        // a new addressed EIB Telegram has been received
  BUSCOUPLER_EVENT_EIB_TELEGRAM_RECEPTION_ERROR, // a new addressed EIB telegram reception failed
  BUSCOUPLER_EVENT_STATE_INDICATION              // new bus coupler state indication received
 };


// Acknowledge values following a telegram sending
enum e_BusCouplerTxAck {
   ACK_RESPONSE = 0,     // bus coupler received an ACK following telegram sending
   NACK_RESPONSE,        // bus coupler received a NACK following telegram sending (1+3 attempts by default)
   NO_ANSWER_TIMEOUT,    // No answer (Data_Confirm) received from the bus coupler
   BUSCOUPLER_RESET_RESPONSE // bus coupler RESET before we get any ACK
};

// --- Typdef for BUS MONITORING mode data ----
typedef struct {
  boolean isEOP;  // True if the data is an End Of Packet
  byte dataByte;  // Last data retrieved on the bus (valid when isEOP is false)
} type_MonitorData;



// Definition of the TP-UART working modes
enum type_KnxBusCouplerMode { NORMAL,
                          BUS_MONITOR };

// Typedef for events callback function
typedef void (*type_EventCallbackFctPtr) (e_KnxBusCouplerEvent);
// Typedef for TX acknowledge callback function
typedef void (*type_AckCallbackFctPtr) (e_BusCouplerTxAck);

// Typedef for external transmit callback function
typedef unsigned char (*type_TransmitCallbackFctPtr) (KnxTelegram *telegram);


class KnxBusCoupler {
  public:
    virtual ~KnxBusCoupler() {};

    virtual byte SetEvtCallback(type_EventCallbackFctPtr) = 0;
    virtual void SetReceivedTelegram(KnxTelegram &telegram) = 0;
    virtual byte SetAckCallback(type_AckCallbackFctPtr) = 0;
    virtual byte GetStateIndication(void) const = 0;
    virtual KnxTelegram& GetReceivedTelegram(void) = 0;
    virtual byte GetTargetedComObjectIndex(void) const = 0;
    virtual boolean IsActive(void) const = 0;

#if defined(KNXTPUART_DEBUG_INFO) || defined(KNXTPUART_DEBUG_ERROR)
    // Set the string used for debug traces
    virtual void SetDebugString(String *strPtr) = 0;
#endif

    virtual byte Reset(void) = 0;
    virtual byte AttachComObjectsList(KnxComObject KnxComObjectsList[],
      byte listSize) = 0;
    virtual byte AttachComObjectsList(KnxComObject** comObjectsList,
      byte listSize) = 0;
    virtual byte Init(void) = 0;

    virtual byte SendTelegram(KnxTelegram& sentTelegram) = 0;

    virtual void RXTask(void) = 0;
    virtual void TXTask(void) = 0;

    virtual boolean GetMonitoringData(type_MonitorData&) = 0;

    virtual void DEBUG_SendResetCommand(void) = 0;
    virtual void DEBUG_SendStateReqCommand(void) = 0;
};



// --- Definitions for the RECEPTION part ----
// RX states
enum e_BusCouplerRxState {
  RX_RESET = 0,                             // The RX part is awaiting reset execution
  RX_STOPPED,                               // TPUART reset event received, RX activity is stopped
  RX_INIT,                                  // The RX part is awaiting init execution
  RX_IDLE_WAITING_FOR_CTRL_FIELD,           // Idle, no reception ongoing
  RX_EIB_TELEGRAM_RECEPTION_STARTED,        // Telegram reception started (address evaluation not done yet)
  RX_EIB_TELEGRAM_RECEPTION_ADDRESSED,      // Addressed telegram reception ongoing
  RX_EIB_TELEGRAM_RECEPTION_LENGTH_INVALID, // The telegram being received is too long
  RX_EIB_TELEGRAM_RECEPTION_NOT_ADDRESSED   // Tegram reception ongoing but not addressed
};


typedef struct {
  e_BusCouplerRxState state;        // Current TPUART RX state
  KnxTelegram receivedTelegram; // Where each received telegram is stored (the content is overwritten on each telegram reception)
                                // A BUSCOUPLER_EVENT_RECEIVED_EIB_TELEGRAM event notifies each content change
  byte addressedComObjectIndex; // Where the index to the targeted com object is stored (the value is overwritten on each telegram reception)
                                // A BUSCOUPLER_EVENT_RECEIVED_EIB_TELEGRAM event notifies each content change
} type_buscoupler_rx;


// --- Definitions for the TRANSMISSION  part ----
// Transmission states
enum e_BusCouplerTxState {
  TX_RESET = 0,                // The TX part is awaiting reset execution
  TX_STOPPED,                  // Bus coupler reset event received, TX activity is stopped
  TX_INIT,                     // The TX part is awaiting init execution
  TX_IDLE,                     // Idle, no transmission ongoing
  TX_TELEGRAM_SENDING_ONGOING, // EIB telegram transmission ongoing
  TX_WAITING_ACK               // Telegram transmitted, waiting for ACK/NACK
};


typedef struct buscoupler_tx {
  e_BusCouplerTxState state;            // Current TPUART TX state
  KnxTelegram *sentTelegram;        // Telegram being sent
  type_AckCallbackFctPtr ackFctPtr; // Pointer to callback function for TX ack
  byte nbRemainingBytes;            // Nb of bytes remaining to be transmitted
  byte txByteIndex;                 // Index of the byte to be sent
} type_buscoupler_tx;


// Values returned by the KnxTpUart member functions :
#define KNX_BUSCOUPLER_OK                            0
#define KNX_BUSCOUPLER_ERROR                       255
#define KNX_BUSCOUPLER_ERROR_NOT_INIT_STATE        254
#define KNX_BUSCOUPLER_ERROR_NULL_EVT_CALLBACK_FCT 253
#define KNX_BUSCOUPLER_ERROR_NULL_ACK_CALLBACK_FCT 252



#endif // KNXBUSCOUPLER_H
