#ifndef _LORAWAN_WG_H_
#define _LORAWAN_WG_H_

#include <stdint.h>


#include "LoRaMacMessageTypes.h"

#include "LoRaMacCrypto.h"

#include "LoRaMacSerializer.h"

#include "secure-element.h"

#include "LoRaMacParser.h"


#include "LoRaMac.h"
/*!
 * LoRaWAN Frame counter list.
 */
typedef struct 
{
    /*!
     * Uplink frame counter which is incremented with each uplink.
     */
    uint32_t FCntUp;
    /*!
     * Network downlink frame counter which is incremented with each downlink on FPort 0
     * or when the FPort field is missing.
     */
    uint32_t NFCntDown;

    uint32_t AFCntDown;
    /*!
     * In case if the device is connected to a LoRaWAN 1.0 Server,
     * this counter is used for every kind of downlink frame.
     */
    uint32_t FCntDown;
    /*!
     * Multicast downlink counter for index 0
     */
    uint32_t McFCntDown0;
    /*!
     * Multicast downlink counter for index 1
     */
    uint32_t McFCntDown1;
    /*!
     * Multicast downlink counter for index 2
     */
    uint32_t McFCntDown2;
    /*!
     * Multicast downlink counter for index 3
     */
    uint32_t McFCntDown3;
}WG_FCntList_t;

/*
 * LoRaMac Crypto Non Volatile Context structure
 */
typedef struct 
{
    /*
     * Stores the information if the device is connected to a LoRaWAN network
     * server with prior to 1.1.0 implementation.
     */
    Version_t LrWanVersion;
    /*
     * Device nonce is a counter starting at 0 when the device is initially
     * powered up and incremented with every JoinRequest.
     */
    uint16_t DevNonce;
    /*
     * JoinNonce is a device specific counter value (that never repeats itself)
     * provided by the join server and incremented with every JoinAccept message.
     */
    uint32_t JoinNonce;
    /*
     * Frame counter list
     */
    WG_FCntList_t FCntList;
    /*
     * RJcount1 is a counter incremented with every Rejoin request Type 1 frame transmitted.
     */
    uint16_t RJcount1;
    /*
     * LastDownFCnt stores the information which frame counter was used to unsecure the last frame.
     * This information is needed to compute ConfFCnt in B1 block for the MIC.
     */
    uint32_t* LastDownFCnt;
}WG_LoRaMacCryptoNvmCtx_t;

/*
 * LoRaMac Crypto Context structure
 */
typedef struct 
{
    /*
     * RJcount0 is a counter incremented with every Type 0 or 2 Rejoin frame transmitted.
     */
    uint16_t RJcount0;
    /*
     * Non volatile module context structure
     */
    WG_LoRaMacCryptoNvmCtx_t* NvmCtx;
    /*
     * Callback function to notify the upper layer about context change
     */
    LoRaMacCryptoNvmEvent EventCryptoNvmCtxChanged;
}WG_LoRaMacCryptoCtx_t;



void lorawan_wg_init();

/*********************************************************************************************************
** 函数名称: LoRaMacCryptoVerifyJoinRequest
** 功能描述: 处理 JoinRequest 帧，验证 MIC
** 输　入  : obj                   加密对象
**           macMsg                JoinRequest 帧存储地址
** 输  出  : 加密 API 操作状态
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LoRaMacCryptoStatus_t LoRaMacCryptoVerifyJoinRequest(LoRaMacMessageJoinRequest_t* macMsg);


LoRaMacParserStatus_t LoRaMacJoinAcceptToBuff( LoRaMacMessageJoinAccept_t* macMsg );



/*
网关解析接收到的上行数据
**/
LoRaMacParserStatus_t lorawan_wg_rev_data( LoRaMacMessageData_t*  macMsgData);

LoRaMacCryptoStatus_t wg_LoRaMacCryptoVerifyJoinRequest(uint8_t* var_buf, uint32_t buf_size, LoRaMacMessageJoinRequest_t* macMsg);

LoRaMacCryptoStatus_t LoRaMacCryptoPrepareJoinAccept(uint8_t *buf, uint32_t *size,LoRaMacMessageJoinAccept_t* macMsg);


#endif

