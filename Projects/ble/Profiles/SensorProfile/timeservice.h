/**************************************************************************************************
  Filename:       timeservice.h
  Revised:        $Date: 2014-08-04
  Revision:       $Revision: 35100 $

  Description:    Time service definitions and prototypes
**************************************************************************************************/

#ifndef TIMESERVICE_H
#define TIMESERVICE_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#include "st_util.h"
  
/*********************************************************************
 * CONSTANTS
 */

// Service UUID
#define TIME_SERV_UUID         0xBB00  // F000AA00-0451-4000-B000-00000000-0000
#define TIME_DATA_UUID         0xBB01
//#define TIME_CONF_UUID         0xBB02
//#define TIME_PERI_UUID         0xBB03

// Sensor Profile Services bit fields
#define TIME_SERVICE           0x00010000

// Length of sensor data in bytes
#define TIME_DATA_LEN          4

#define TIME_MS_PER_TICK 100 // 100ms

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * API FUNCTIONS
 */


/*
 * Time_AddService- Initializes the Sensor GATT Profile service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 */
extern bStatus_t Time_AddService( uint32 services );

/*
 * Time_RegisterAppCBs - Registers the application callback function.
 *                    Only call this function once.
 *
 *    appCallbacks - pointer to application callbacks.
 */
extern bStatus_t Time_RegisterAppCBs( sensorCBs_t *appCallbacks );

/*
 * Time_SetParameter - Set a Sensor GATT Profile parameter.
 *
 *    param - Profile parameter ID
 *    len   - length of data to write
 *    value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 */
extern bStatus_t Time_SetParameter( uint8 param, uint8 len, void *value );

/*
 * Time_GetParameter - Get a Sensor GATT Profile parameter.
 *
 *    param - Profile parameter ID
 *    value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 */
extern bStatus_t Time_GetParameter( uint8 param, void *value );


/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* TIMESERVICE_H */

