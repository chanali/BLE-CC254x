/**************************************************************************************************
  Filename:       irtempservice.c
  Revised:        $Date: 2013-08-23 11:45:31 -0700 (Fri, 23 Aug 2013) $
  Revision:       $Revision: 35100 $

  Description:    IR Temperature service.


  Copyright 2012-2013 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED �AS IS� WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "bcomdef.h"
#include "linkdb.h"
#include "hal_flash.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"

#include "irtempservice.h"
#include "st_util.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/* Service configuration values */
#define SENSOR_SERVICE_UUID     IRTEMPERATURE_SERV_UUID
#define SENSOR_DATA_UUID        IRTEMPERATURE_DATA_UUID
#define SENSOR_CONFIG_UUID      IRTEMPERATURE_CONF_UUID
#define SENSOR_PERIOD_UUID      IRTEMPERATURE_PERI_UUID

#define SENSOR_SERVICE          IRTEMPERATURE_SERVICE
#define SENSOR_DATA_LEN         IRTEMPERATURE_DATA_LEN_WITH_TIME

#define SENSOR_DATA_DESCR       "Temp. Data"
#define SENSOR_CONFIG_DESCR     "Temp. Conf."
#define SENSOR_PERIOD_DESCR     "Temp. Period"

// The temperature sensor does not support the 100 ms update rate
#undef SENSOR_MIN_UPDATE_PERIOD
#define SENSOR_MIN_UPDATE_PERIOD  300 // Minimum 300 milliseconds

#define IRTEMP_RINGBUFFER_DEPTH  16 
#define IRTEMP_RINGBUFFER_SIZE ((IRTEMP_RINGBUFFER_DEPTH)*(SENSOR_DATA_LEN))

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

// Service UUID
static CONST uint8 sensorServiceUUID[TI_UUID_SIZE] =
{
  TI_UUID(SENSOR_SERVICE_UUID),
};

// Characteristic UUID: data
static CONST uint8 sensorDataUUID[TI_UUID_SIZE] =
{
  TI_UUID(SENSOR_DATA_UUID),
};

// Characteristic UUID: config
static CONST uint8 sensorCfgUUID[TI_UUID_SIZE] =
{
  TI_UUID(SENSOR_CONFIG_UUID),
};

// Characteristic UUID: period
static CONST uint8 sensorPeriodUUID[TI_UUID_SIZE] =
{
  TI_UUID(SENSOR_PERIOD_UUID),
};


/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

static sensorCBs_t *sensor_AppCBs = NULL;

/*********************************************************************
 * Profile Attributes - variables
 */

// Profile Service attribute
static CONST gattAttrType_t sensorService = { TI_UUID_SIZE, sensorServiceUUID };

// Characteristic Value: data
static uint8 sensorData[SENSOR_DATA_LEN] = { 0, 0, 0, 0};

// Characteristic Properties: data
static uint8 sensorDataProps = GATT_PROP_READ | GATT_PROP_NOTIFY;

// Characteristic Configuration: data
static gattCharCfg_t sensorDataConfig[GATT_MAX_NUM_CONN];

// Characteristic User Description: data
static uint8 sensorDataUserDescr[] = SENSOR_DATA_DESCR;

// Characteristic Properties: configuration
static uint8 sensorCfgProps = GATT_PROP_READ | GATT_PROP_WRITE;

// Characteristic Value: configuration
static uint8 sensorCfg = 0;

// Characteristic User Description: configuration
static uint8 sensorCfgUserDescr[] = SENSOR_CONFIG_DESCR; 

// Characteristic Properties: period
static uint8 sensorPeriodProps = GATT_PROP_READ | GATT_PROP_WRITE;

// Characteristic Value: period
static uint8 sensorPeriod = SENSOR_MIN_UPDATE_PERIOD / SENSOR_PERIOD_RESOLUTION;

// Characteristic User Description: period
static uint8 sensorPeriodUserDescr[] = SENSOR_PERIOD_DESCR;

static uint8 irTempLinkStatus = LINKDB_STATUS_UPDATE_NEW;

/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t sensorAttrTable[] =
{
  {
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&sensorService                   /* pValue */
  },

    // Characteristic Declaration
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &sensorDataProps
    },

      // Characteristic Value "Data"
      {
        { TI_UUID_SIZE, sensorDataUUID },
        GATT_PERMIT_READ,
        0,
        sensorData
      },

      // Characteristic configuration
      {
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8 *)sensorDataConfig
      },

      // Characteristic User Description
      {
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ,
        0,
        sensorDataUserDescr
      },

    // Characteristic Declaration
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &sensorCfgProps
    },

      // Characteristic Value "Configuration"
      {
        { TI_UUID_SIZE, sensorCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        &sensorCfg
      },

      // Characteristic User Description
      {
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ,
        0,
        sensorCfgUserDescr
      },
     // Characteristic Declaration "Period"
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &sensorPeriodProps
    },

      // Characteristic Value "Period"
      {
        { TI_UUID_SIZE, sensorPeriodUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        &sensorPeriod
      },

      // Characteristic User Description "Period"
      {
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ,
        0,
        sensorPeriodUserDescr
      },
};

//#pragma data_alignment=4
//#pragma pack (4)
static uint32 irTempRingBuffer[IRTEMP_RINGBUFFER_SIZE/sizeof(uint32)];
//#pragma
static irTempData_t* irTempRingRecord;
static uint16 irTempRingHead;
static uint16 irTempRingTail;

//static bool irTempFlashValid = FALSE;
//flash pointer in unit of byte
static uint32 irTempFlashBegin;
static uint32 irTempFlashHead;
static uint32 irTempFlashTail;
static uint32 irTempFlashEnd;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static uint8 sensor_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen );
static bStatus_t sensor_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset );
static void sensor_HandleConnStatusCB( uint16 connHandle, uint8 changeType );

static uint8 IRTemp_isNotificationEn(void);
static uint8 IRTempGetLinkStatus(void);
static bStatus_t IRTempReadRecordFromFlash(uint8 *data, uint8 *len);

extern bStatus_t SensorTag_wrFlashInfo(flashRecInfo_t *flashInfo);

/*********************************************************************
 * PROFILE CALLBACKS
 */
// Simple Profile Service Callbacks
static CONST gattServiceCBs_t sensorCBs =
{
  sensor_ReadAttrCB,  // Read callback function pointer
  sensor_WriteAttrCB, // Write callback function pointer
  NULL                // Authorization callback function pointer
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      IRTemp_AddService
 *
 * @brief   Initializes the Sensor Profile service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 *
 * @return  Success or Failure
 */
bStatus_t IRTemp_AddService( uint32 services )
{
  uint8 status = SUCCESS;

  // Register with Link DB to receive link status change callback
  VOID linkDB_Register( sensor_HandleConnStatusCB );
  GATTServApp_InitCharCfg( INVALID_CONNHANDLE, sensorDataConfig );

  if (services & SENSOR_SERVICE )
  {
       // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService( sensorAttrTable,
                                          GATT_NUM_ATTRS( sensorAttrTable ),
                                          &sensorCBs );
  }

  // to save IRTEMP_RINGBUFFER_DEPTH sample data, it's better to align buffer size to flash page size.
  irTempRingRecord = (irTempData_t*)irTempRingBuffer;
  irTempRingHead = irTempRingTail = 0;

  return ( status );
}


/*********************************************************************
 * @fn      IRTemp_RegisterAppCBs
 *
 * @brief   Registers the application callback function. Only call
 *          this function once.
 *
 * @param   callbacks - pointer to application callbacks.
 *
 * @return  SUCCESS or bleAlreadyInRequestedMode
 */
bStatus_t IRTemp_RegisterAppCBs( sensorCBs_t *appCallbacks )
{
  if ( sensor_AppCBs == NULL )
  {
    if ( appCallbacks != NULL )
    {
      sensor_AppCBs = appCallbacks;
    }

    return ( SUCCESS );
  }

  return ( bleAlreadyInRequestedMode );
}

/*********************************************************************
*
*/
void IRTemp_flashInit(flashRecInfo_t *flashInfo)
{
  int pg;
  
  if (1)//(flashRecInfo[0].flashInUse != FLASH_REC_MAGIC)  // flash can't be writen if head/tail not saved and flash not erased.
  {
    for (pg=IRTEMP_FLASH_PAGE_BASE; pg<IRTEMP_FLASH_PAGE_BASE+IRTEMP_FLASH_PAGE_CNT; pg++)
    {
      uint16 offset;
      uint8 tmp;
      bool n_erased = 0;
  	
      HalFlashErase(pg);
      for (offset = 0; offset < HAL_FLASH_PAGE_SIZE; offset ++)
      {
        HalFlashRead(pg, offset, &tmp, 1);
        if (tmp != 0xFF)
        {
          n_erased = 1;
          break;
        }
      }
      if (n_erased) while(1);
    }
    flashInfo[0].flashInUse = FLASH_REC_MAGIC;
    flashInfo[0].head = flashInfo[0].tail = irTempFlashHead = irTempFlashTail = 
      (uint32)IRTEMP_FLASH_PAGE_BASE*(uint32)HAL_FLASH_PAGE_SIZE;
    SensorTag_wrFlashInfo(flashInfo);
  }
  else
  {
    irTempFlashHead = flashInfo[0].head;
    irTempFlashTail = flashInfo[0].tail;
  }
  // successfully to test it
  // write data in unit of byte in ram and in unit of word(4 bytes) in flash
  //HalFlashWrite((uint32)IRTEMP_FLASH_PAGE_BASE*(uint32)HAL_FLASH_PAGE_SIZE/4 /* in flash */, 
  //	(uint8 *)&flash_w_data /* in ram */,1 /* in flash */);
  // read data in unit of byte even for flash.
  //HalFlashRead(IRTEMP_FLASH_PAGE_BASE, 0, (uint8*)&flash_r_data, 4);

  irTempFlashBegin =(uint32)IRTEMP_FLASH_PAGE_BASE*(uint32)HAL_FLASH_PAGE_SIZE;
  irTempFlashEnd = irTempFlashBegin + (uint32)HAL_FLASH_PAGE_SIZE*(uint32)IRTEMP_FLASH_PAGE_CNT;

  return;
}

static uint8 IRTemp_isNotificationEn(void)
{
  if (((sensorDataConfig[0].value)&0x01) == 0x01)
    return 1; // Notification has been enabled
  else
    return 0;
}

static bStatus_t IRTempReadRecordFromRam(uint8 *data, uint8 *pLen)
{
  if (irTempRingHead != irTempRingTail)
  {
    *pLen = SENSOR_DATA_LEN;
    osal_memcpy(data, &irTempRingRecord[irTempRingTail], SENSOR_DATA_LEN);
    irTempRingTail = (irTempRingTail == (IRTEMP_RINGBUFFER_DEPTH-1))?0:(irTempRingTail+1);
  }	
  else
  {
    *pLen = SENSOR_DATA_LEN;
	osal_memset( data, 0, SENSOR_DATA_LEN );
  }

  return SUCCESS;
}

static bStatus_t IRTempReadRecordFromFlash(uint8 *data, uint8 *pLen)
{
  int i;
  
  if ((irTempFlashHead - irTempFlashTail) >= SENSOR_DATA_LEN)
  {
    *pLen = SENSOR_DATA_LEN;
    HalFlashRead(irTempFlashTail/HAL_FLASH_PAGE_SIZE, 
        irTempFlashTail%HAL_FLASH_PAGE_SIZE, data, SENSOR_DATA_LEN);
    irTempFlashTail += SENSOR_DATA_LEN;
  }
  else
  {
    IRTempReadRecordFromRam(data, pLen);
    // if all flash data is read out and no enough flash to write, erase the flash.
    if ((irTempFlashEnd - irTempFlashHead) < SENSOR_DATA_LEN)
    {
      for (i=IRTEMP_FLASH_PAGE_BASE; i<IRTEMP_FLASH_PAGE_BASE+IRTEMP_FLASH_PAGE_CNT; i++)
        HalFlashErase(i);
      irTempFlashHead = irTempFlashTail = irTempFlashBegin = (uint32)IRTEMP_FLASH_PAGE_BASE*(uint32)HAL_FLASH_PAGE_SIZE;
      irTempFlashEnd = irTempFlashBegin + (uint32)HAL_FLASH_PAGE_SIZE*(uint32)IRTEMP_FLASH_PAGE_CNT;
    }
  }

  return SUCCESS;
}

/*
refer to osal_snv.c and flash controller in <CC253x4x User's Guide>
The flash memory is divided into 2048-byte or 1024-byte flash pages. A flash page is the smallest
erasable unit in the memory, whereas a 32-bit word is the smallest writable unit that can be written to the
flash.
*/
static bStatus_t IRTempSaveDataToFlash( uint8 *data, uint16 len )
{
  // for word(4 bytes) alignment and page is 512 words(2048 bytes)
  //uint16 addr = (offset >> 2) + ((uint16)pg << 9);
  //uint16 cnt = len >> 2;
  uint32 addr = irTempFlashHead >> 2;
  uint16 cnt = len >> 2;

  HalFlashWrite((uint16)addr, data, cnt); // in unit of word
  // verify...
  
  return SUCCESS;
}

static bStatus_t IRTempSaveDataToRam(uint8 *data, uint16 len)
{
  if (ringBufferIsFull(irTempRingHead, irTempRingTail, IRTEMP_RINGBUFFER_DEPTH))
  {
    // the current entry is available
    osal_memcpy(&irTempRingRecord[irTempRingHead], data, len);
    if ((irTempFlashEnd - irTempFlashHead) >= IRTEMP_RINGBUFFER_SIZE)
    {
      IRTempSaveDataToFlash((uint8 *)irTempRingBuffer, IRTEMP_RINGBUFFER_SIZE);
      irTempFlashHead += IRTEMP_RINGBUFFER_SIZE;
    }  
    irTempRingHead = irTempRingTail = 0;  // reuse all ring buffer
  }
  else
  {
    osal_memcpy(&irTempRingRecord[irTempRingHead], data, len);
    // move to available entry
    irTempRingHead = (irTempRingHead == (IRTEMP_RINGBUFFER_DEPTH-1))?0:(irTempRingHead+1);
  }
  
  return SUCCESS;
}

/*********************************************************************
 * @fn      IRTemp_SetParameter
 *
 * @brief   Set a parameter.
 *
 * @param   param - Profile parameter ID
 * @param   len - length of data to right
 * @param   value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t IRTemp_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;

  switch ( param )
  {
    case SENSOR_DATA:
    if ( len == SENSOR_DATA_LEN )
    {
      /* copy to attribution table */
      VOID osal_memcpy( sensorData, value, SENSOR_DATA_LEN );
      // See if Notification has been enabled
      /* write data "01 00" to "Client Characteristic Configuration" 
      to enable notification of GATTServApp_ProcessCharCfg() */ 
      if ((IRTempGetLinkStatus() != LINKDB_STATUS_UPDATE_REMOVED) && IRTemp_isNotificationEn())
      {
        GATTServApp_ProcessCharCfg( sensorDataConfig, sensorData, FALSE,
                                 sensorAttrTable, GATT_NUM_ATTRS( sensorAttrTable ),
                                 INVALID_TASK_ID );
      }  
      else
      {
        IRTempSaveDataToRam(sensorData, SENSOR_DATA_LEN);
      }
    }
    else
    {
      ret = bleInvalidRange;
    }
    break;

    case SENSOR_CONF:
      if ( len == sizeof ( uint8 ) )
      {
        sensorCfg = *((uint8*)value);
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;

    case SENSOR_PERI:
      if ( len == sizeof ( uint8 ) )
      {
        sensorPeriod = *((uint8*)value);
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;

    default:
      ret = INVALIDPARAMETER;
      break;
  }

  return ( ret );
}

/*********************************************************************
 * @fn      IRTemp_GetParameter
 *
 * @brief   Get a Sensor Profile parameter.
 *
 * @param   param - Profile parameter ID
 * @param   value - pointer to data to put.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t IRTemp_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;

  switch ( param )
  {
    case SENSOR_DATA:
      VOID osal_memcpy( value, sensorData, SENSOR_DATA_LEN );
      break;

    case SENSOR_CONF:
      *((uint8*)value) = sensorCfg;
      break;

    case SENSOR_PERI:
      *((uint8*)value) = sensorPeriod;
      break;

    default:
      ret = INVALIDPARAMETER;
      break;
  }

  return ( ret );
}

/*********************************************************************
 * @fn          sensor_ReadAttrCB
 *
 * @brief       Read an attribute.
 *
 * @param       connHandle - connection message was received on
 * @param       pAttr - pointer to attribute
 * @param       pValue - pointer to data to be read
 * @param       pLen - length of data to be read
 * @param       offset - offset of the first octet to be read
 * @param       maxLen - maximum length of data to be read
 *
 * @return      Success or Failure
 */
static uint8 sensor_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen )
{
  uint16 uuid;
  bStatus_t status = SUCCESS;

  // If attribute permissions require authorization to read, return error
  if ( gattPermitAuthorRead( pAttr->permissions ) )
  {
    // Insufficient authorization
    return ( ATT_ERR_INSUFFICIENT_AUTHOR );
  }

  // Make sure it's not a blob operation (no attributes in the profile are long)
  if ( offset > 0 )
  {
    return ( ATT_ERR_ATTR_NOT_LONG );
  }

  if (utilExtractUuid16(pAttr,&uuid) == FAILURE) {
    // Invalid handle
    *pLen = 0;
    return ATT_ERR_INVALID_HANDLE;
  }

  switch ( uuid )
  {
    // No need for "GATT_SERVICE_UUID" or "GATT_CLIENT_CHAR_CFG_UUID" cases;
    // gattserverapp handles those reads
    case SENSOR_DATA_UUID:
      if (1)
      {
        IRTempReadRecordFromFlash(pValue, pLen);
      }
      else
      {
        *pLen = SENSOR_DATA_LEN;
        VOID osal_memcpy( pValue, pAttr->pValue, SENSOR_DATA_LEN );
      }
      break;

    case SENSOR_CONFIG_UUID:
    case SENSOR_PERIOD_UUID:
      *pLen = 1;
      pValue[0] = *pAttr->pValue;
      break;

    default:
      *pLen = 0;
      status = ATT_ERR_ATTR_NOT_FOUND;
      break;
    }

  return ( status );
}

/*********************************************************************
* @fn      sensor_WriteAttrCB
*
* @brief   Validate attribute data prior to a write operation
*
* @param   connHandle - connection message was received on
* @param   pAttr - pointer to attribute
* @param   pValue - pointer to data to be written
* @param   len - length of data
* @param   offset - offset of the first octet to be written
*
* @return  Success or Failure
*/
static bStatus_t sensor_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                           uint8 *pValue, uint8 len, uint16 offset )
{
  bStatus_t status = SUCCESS;
  uint8 notifyApp = 0xFF;
  uint16 uuid;

  // If attribute permissions require authorization to write, return error
  if ( gattPermitAuthorWrite( pAttr->permissions ) )
  {
    // Insufficient authorization
    return ( ATT_ERR_INSUFFICIENT_AUTHOR );
  }

  if (utilExtractUuid16(pAttr,&uuid) == FAILURE) {
    // Invalid handle
    return ATT_ERR_INVALID_HANDLE;
  }

  switch ( uuid )
  {
    case SENSOR_DATA_UUID:
      // Should not get here
      break;

    case SENSOR_CONFIG_UUID:
      // Validate the value
      // Make sure it's not a blob oper
      if ( offset == 0 )
      {
        if ( len != 1 )
        {
          status = ATT_ERR_INVALID_VALUE_SIZE;
        }
      }
      else
      {
        status = ATT_ERR_ATTR_NOT_LONG;
      }

      // Write the value
      if ( status == SUCCESS )
      {
        uint8 *pCurValue = (uint8 *)pAttr->pValue;

        *pCurValue = pValue[0];

        if( pAttr->pValue == &sensorCfg )
        {
          notifyApp = SENSOR_CONF;
        }
      }
      break;

    case SENSOR_PERIOD_UUID:
      // Validate the value
      // Make sure it's not a blob oper
      if ( offset == 0 )
      {
        if ( len != 1 )
        {
          status = ATT_ERR_INVALID_VALUE_SIZE;
        }
      }
      else
      {
        status = ATT_ERR_ATTR_NOT_LONG;
      }
      // Write the value
      if ( status == SUCCESS )
      {
        if (pValue[0]>=(SENSOR_MIN_UPDATE_PERIOD/SENSOR_PERIOD_RESOLUTION))
        {

          uint8 *pCurValue = (uint8 *)pAttr->pValue;
          *pCurValue = pValue[0];

          if( pAttr->pValue == &sensorPeriod )
          {
            notifyApp = SENSOR_PERI;
          }
        }
        else
        {
           status = ATT_ERR_INVALID_VALUE;
        }
      }
      break;

    case GATT_CLIENT_CHAR_CFG_UUID:
      status = GATTServApp_ProcessCCCWriteReq( connHandle, pAttr, pValue, len,
                                              offset, GATT_CLIENT_CFG_NOTIFY );
      break;

    default:
      // Should never get here!
      status = ATT_ERR_ATTR_NOT_FOUND;
      break;
  }

  // If a charactersitic value changed then callback function to notify application of change
  if ( (notifyApp != 0xFF ) && sensor_AppCBs && sensor_AppCBs->pfnSensorChange )
  {
    sensor_AppCBs->pfnSensorChange( notifyApp );
  }

  return ( status );
}

/*********************************************************************
 * @fn          sensor_HandleConnStatusCB
 *
 * @brief       Sensor Profile link status change handler function.
 *
 * @param       connHandle - connection handle
 * @param       changeType - type of change
 *
 * @return      none
 */
static void sensor_HandleConnStatusCB( uint16 connHandle, uint8 changeType )
{
  // Make sure this is not loopback connection
  if ( connHandle != LOOPBACK_CONNHANDLE )
  {
    // Reset Client Char Config if connection has dropped
    if ( ( changeType == LINKDB_STATUS_UPDATE_REMOVED )      ||
         ( ( changeType == LINKDB_STATUS_UPDATE_STATEFLAGS ) &&
           ( !linkDB_Up( connHandle ) ) ) )
    {
      GATTServApp_InitCharCfg( connHandle, sensorDataConfig );
      irTempLinkStatus = LINKDB_STATUS_UPDATE_REMOVED;
    }
    else
	{
      irTempLinkStatus = LINKDB_STATUS_UPDATE_NEW;
    }
  }
}

//if link is down, client will not issue R/W command.
static uint8 IRTempGetLinkStatus(void)
{
  return irTempLinkStatus;
}

/*********************************************************************
*********************************************************************/
