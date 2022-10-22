
#include <math.h>
#include <string.h>
#include <stdbool.h>

#include "lorawan_wg.h"
#include "LoRaMacMessageTypes.h"

#include "LoRaMacCrypto.h"

#include "LoRaMacSerializer.h"

#include "secure-element.h"


#include "cmac.h"

/*!
 * Indicates if LoRaWAN 1.1.x crypto scheme is enabled
 */
#define USE_LRWAN_1_1_X_CRYPTO                      0

/*!
 * Indicates if a random devnonce must be used or not
 */
#define USE_RANDOM_DEV_NONCE                        1

/*
 * Frame direction definition for uplink communications
 */
#define UPLINK                          0

/*
 * Frame direction definition for downlink communications
 */
#define DOWNLINK                        1

/*
 * CMAC/AES Message Integrity Code (MIC) Block B0 size
 */
#define MIC_BLOCK_BX_SIZE               16

/*
 * Size of JoinReqType is field for integrity check
 */
#define JOIN_REQ_TYPE_SIZE              1

/*
 * Size of DevNonce is field for integrity check
 */
#define DEV_NONCE_SIZE                  2

/*
 * Number of security context entries
 */
#define NUM_OF_SEC_CTX                  5

/*
 * Size of the module context
 */
#define CRYPTO_CTX_SIZE                 sizeof( WG_LoRaMacCryptoCtx_t )

/*
 * Size of the module non volatile context
 */
#define CRYPTO_NVM_CTX_SIZE             sizeof( WG_LoRaMacCryptoNvmCtx_t )

/*
 * Maximum size of the message that can be handled by the crypto operations
 */
#define CRYPTO_MAXMESSAGE_SIZE          256

/*
 * Maximum size of the buffer for crypto operations
 */
#define CRYPTO_BUFFER_SIZE              CRYPTO_MAXMESSAGE_SIZE + MIC_BLOCK_BX_SIZE

/*
 * MIC computaion offset
 */
#define CRYPTO_MIC_COMPUTATION_OFFSET   JOIN_REQ_TYPE_SIZE + LORAMAC_JOIN_EUI_FIELD_SIZE + DEV_NONCE_SIZE + LORAMAC_MHDR_FIELD_SIZE



void lorawan_wg_init(){


    return ;
}

LoRaMacCryptoStatus_t wg_LoRaMacCryptoVerifyJoinRequest(uint8_t* var_buf, uint32_t buf_size, LoRaMacMessageJoinRequest_t* macMsg){
    KeyIdentifier_t micComputationKeyID = NWK_KEY;
    uint32_t mic = 0;
    uint32_t idx = 0;
    // Compute mic
    if( SecureElementComputeAesCmac( NULL, var_buf, ( LORAMAC_JOIN_REQ_MSG_SIZE - LORAMAC_MIC_FIELD_SIZE ), micComputationKeyID, &mic ) != SECURE_ELEMENT_SUCCESS )
    {
        return LORAMAC_CRYPTO_ERROR_SECURE_ELEMENT_FUNC;
    }
 
    if((uint32_t)((var_buf[19]) | (var_buf[20] << 8) |(var_buf[21] << 16) | (var_buf[22] << 24)) != mic){
        return LORAMAC_CRYPTO_ERROR;
    }
   
    macMsg->MHDR.Value = var_buf[0];
    idx +=1;
    memcpyr(macMsg->JoinEUI, var_buf+idx, 8);

    idx +=8;
    memcpyr(macMsg->DevEUI, var_buf+idx, 8);

    idx +=8;
    macMsg->DevNonce = (uint16_t)((var_buf[idx]) | (var_buf[idx+1] << 8));
    return LORAMAC_CRYPTO_SUCCESS;    
}


/*********************************************************************************************************
** 函数名称: LoRaMacCryptoVerifyJoinRequest
** 功能描述: 处理 JoinRequest 帧，验证 MIC
** 输　入  : obj                   加密对象
**           macMsg                JoinRequest 帧存储地址
** 输  出  : 加密 API 操作状态
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LoRaMacCryptoStatus_t LoRaMacCryptoVerifyJoinRequest(LoRaMacMessageJoinRequest_t* macMsg)
{
    uint32_t mic = 0;
    if( macMsg == 0 )
    {
        return LORAMAC_CRYPTO_ERROR_NPE;
    }
    KeyIdentifier_t micComputationKeyID = NWK_KEY;
    // Serialize message
    if( LoRaMacSerializerJoinRequest( macMsg ) != LORAMAC_SERIALIZER_SUCCESS )
    {
        return LORAMAC_CRYPTO_ERROR_SERIALIZER;
    }

    // Compute mic
    if( SecureElementComputeAesCmac( NULL, macMsg->Buffer, ( LORAMAC_JOIN_REQ_MSG_SIZE - LORAMAC_MIC_FIELD_SIZE ), micComputationKeyID, &mic ) != SECURE_ELEMENT_SUCCESS )
    {
        return LORAMAC_CRYPTO_ERROR_SECURE_ELEMENT_FUNC;
    }

    if(mic == macMsg->MIC){
        return LORAMAC_CRYPTO_SUCCESS;
    }

    return LORAMAC_CRYPTO_ERROR;
}




/*********************************************************************************************************
** 函数名称: LoRaMacCryptoPrepareJoinAccept
** 功能描述: JoinAccept 加密并组帧，先计算 MIC 再加密 数据
** 输　入  : obj                   加密对象
**           macMsg                JoinReq 帧存储地址
** 输  出  : 加密 API 操作状态
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LoRaMacCryptoStatus_t LoRaMacCryptoPrepareJoinAccept(uint8_t *buf, uint32_t *size,LoRaMacMessageJoinAccept_t* macMsg){
    if( ( macMsg == 0 ) )
    {
        return LORAMAC_CRYPTO_ERROR_NPE;
    }
    LoRaMacCryptoStatus_t retval = LORAMAC_CRYPTO_ERROR;
    KeyIdentifier_t micComputationKeyID;
    KeyIdentifier_t encryptionKeyID;
    uint8_t micComputationOffset = 0;
#if( USE_LRWAN_1_1_X_CRYPTO == 1 )
    uint8_t* devNonceForKeyDerivation = ( uint8_t* ) &CryptoCtx.NvmCtx->DevNonce;
#endif
    if(LoRaMacJoinAcceptToBuff(macMsg) != LORAMAC_PARSER_SUCCESS){
        return LORAMAC_CRYPTO_ERROR;
    }



    // Determine decryption key and DevNonce for key derivation
    encryptionKeyID = NWK_KEY;
    micComputationOffset = CRYPTO_MIC_COMPUTATION_OFFSET;

    if( SecureElementComputeAesCmac( NULL, macMsg->Buffer , macMsg->BufSize, encryptionKeyID, &(macMsg->MIC) )!= SECURE_ELEMENT_SUCCESS)
    {
        return LORAMAC_CRYPTO_ERROR;
    }

    //add mic
    // Serialize message
    if( LoRaMacJoinAcceptToBuff( macMsg ) != LORAMAC_PARSER_SUCCESS )
    {
        return LORAMAC_CRYPTO_ERROR;
    }
    //test
    printf("orig:");
    for(int i=0; i<macMsg->BufSize; i++){
        printf("[%02X]", macMsg->Buffer[i]);
    }
    printf("\n");
    // Decrypt header, skip MHDR
    uint8_t procBuffer[CRYPTO_MAXMESSAGE_SIZE + CRYPTO_MIC_COMPUTATION_OFFSET];
    memset1( procBuffer, 0, ( macMsg->BufSize + micComputationOffset ) ); 


    if( SecureElementAesDecrypt( macMsg->Buffer + LORAMAC_MHDR_FIELD_SIZE, ( macMsg->BufSize - LORAMAC_MHDR_FIELD_SIZE ), encryptionKeyID, ( procBuffer  ) ) != SECURE_ELEMENT_SUCCESS )
    {
        return LORAMAC_CRYPTO_ERROR_SECURE_ELEMENT_FUNC;
    }

    printf("buf:");
    for(int i=0; i < macMsg->BufSize - LORAMAC_MHDR_FIELD_SIZE; i++){

        printf("[%02X]", procBuffer[i]);
    }
    printf("\n");

    buf[0] = macMsg->MHDR.Value;
    memcpy1(buf+1, procBuffer, macMsg->BufSize - LORAMAC_MHDR_FIELD_SIZE);
    *size = macMsg->BufSize;
    return LORAMAC_CRYPTO_SUCCESS;
}



LoRaMacParserStatus_t LoRaMacJoinAcceptToBuff( LoRaMacMessageJoinAccept_t* macMsg )
{
    if( ( macMsg == 0 ) || ( macMsg->Buffer == 0 ) )
    {
        return LORAMAC_PARSER_ERROR_NPE;
    }

    uint16_t bufItr = 0;

   
    macMsg->Buffer[bufItr++] = macMsg->MHDR.Value;

    memcpy1( &macMsg->Buffer[bufItr], macMsg->JoinNonce, 3 );
    bufItr = bufItr + 3;


    memcpy1( &macMsg->Buffer[bufItr], macMsg->NetID, 3 );
    bufItr = bufItr + 3;

    uint8_t t_num = 0;
    


    macMsg->Buffer[bufItr++] = (uint8_t)(macMsg->DevAddr & 0xff);
    macMsg->Buffer[bufItr++] = (uint8_t)((macMsg->DevAddr >> 8) & 0xff);
    macMsg->Buffer[bufItr++] = (uint8_t)((macMsg->DevAddr >> 16) & 0xff);
    macMsg->Buffer[bufItr++] = (uint8_t)((macMsg->DevAddr >> 24) & 0xff);

    macMsg->Buffer[bufItr++] = macMsg->DLSettings.Value;

    macMsg->Buffer[bufItr++] = macMsg->RxDelay;

    if(macMsg->CFList[0]){
        memcpy1( &macMsg->Buffer[bufItr], macMsg->CFList,  LORAMAC_C_FLIST_FIELD_SIZE );
        bufItr = bufItr + LORAMAC_C_FLIST_FIELD_SIZE;
    }


    macMsg->Buffer[bufItr++] = (uint8_t)(macMsg->MIC & 0xff);
    macMsg->Buffer[bufItr++] = (uint8_t)((macMsg->MIC >> 8) & 0xff);
    macMsg->Buffer[bufItr++] = (uint8_t)((macMsg->MIC >> 16) & 0xff);
    macMsg->Buffer[bufItr++] = (uint8_t)((macMsg->MIC >> 24) & 0xff);

    macMsg->BufSize = bufItr;

    return LORAMAC_PARSER_SUCCESS;
}


/*
网关解析接收到的上行数据
**/
LoRaMacParserStatus_t lorawan_wg_rev_data( LoRaMacMessageData_t*  macMsgData){
    if(macMsgData == 0){
        return LORAMAC_PARSER_FAIL;
    }
    uint32_t address = 0;

    if( LORAMAC_PARSER_SUCCESS != LoRaMacParserData( macMsgData ) )
    {
        return LORAMAC_PARSER_FAIL;
    }
    uint32_t fCntDown = 1;

    if(LORAMAC_CRYPTO_SUCCESS != wg_LoRaMacCryptoUnsecureMessage( MULTICAST_0_ADDR, address, FCNT_UP,  fCntDown, macMsgData ))
    {
        return LORAMAC_PARSER_FAIL;
    }
    

    return LORAMAC_PARSER_SUCCESS;
}


//下发指令到终端设备
LoRaMacParserStatus_t lorawan_wg_send_data( McpsReq_t* mcpsRequest){
    LoRaMacMessageData_t*  macMsgData = 0;
    LoRaMacStatus_t status = LORAMAC_STATUS_SERVICE_UNKNOWN;
    LoRaMacHeader_t macHdr;
    uint8_t fPort = 0;
    void* fBuffer;
    uint16_t fBufferSize;

    bool readyToSend = false;

    switch( mcpsRequest->Type )
    {
        case MCPS_UNCONFIRMED:
        {
            readyToSend = true;
            macHdr.Bits.MType = FRAME_TYPE_DATA_UNCONFIRMED_UP;
            fPort = mcpsRequest->Req.Unconfirmed.fPort;
            fBuffer = mcpsRequest->Req.Unconfirmed.fBuffer;
            fBufferSize = mcpsRequest->Req.Unconfirmed.fBufferSize;
            break;
        }
        case MCPS_CONFIRMED:
        {
            readyToSend = true;
            macHdr.Bits.MType = FRAME_TYPE_DATA_CONFIRMED_UP;
            fPort = mcpsRequest->Req.Confirmed.fPort;
            fBuffer = mcpsRequest->Req.Confirmed.fBuffer;
            fBufferSize = mcpsRequest->Req.Confirmed.fBufferSize;
            break;
        }
        case MCPS_PROPRIETARY:
        {
            readyToSend = true;
            macHdr.Bits.MType = FRAME_TYPE_PROPRIETARY;
            fBuffer = mcpsRequest->Req.Proprietary.fBuffer;
            fBufferSize = mcpsRequest->Req.Proprietary.fBufferSize;
            break;
        }
        default:
            break;
    }

    wg_LoRaMacCryptoSecureMessage(0, macMsgData);

    return LORAMAC_PARSER_SUCCESS;
}
