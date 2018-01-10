/**
 * @file				BleAtt.c
 * @brief			ble 属性协议
 * @author			fengxun
 * @date			2018年1月2日
*/
#include "../../include/DhGlobalHead.h"

/*
	Requests—PDUs sent to a server by a client, and invoke responses
		If a server receives a request that it does not support, then the server shall
		respond with the Error Response with the Error Code «Request Not Supported», with the Attribute Handle In Error set to 0x0000.

		If the server receives an invalid request–for example, the PDU is the wrong
		length–then the server shall respond with the Error Response with the Error
		Code «Invalid PDU», with the Attribute Handle In Error set to 0x0000.

		If a server does not have sufficient resources to process a request, then the
		server shall respond with the Error Response with the Error Code «Insufficient
		Resources», with the Attribute Handle In Error set to 0x0000.

		If a server cannot process a request because an error was encountered during
		the processing of this request, then the server shall respond with the Error
		Response with the Error Code «Unlikely Error», with the Attribute Handle In
		Error set to 0x0000.
	Commands—PDUs sent to a server by a client
		If a server receives a command that it does not support, indicated by the Command
		Flag of the PDU set to one, then the server shall ignore the Command.


*/
#define ATT_ERR_INVALID_HANDLE						0x01
#define ATT_ERR_READ_NOT_PERMITTED					0x02
#define ATT_ERR_WRITE_NOT_PERMITTED					0x03
#define ATT_ERR_INVALID_PDU							0x04
#define ATT_ERR_INSUFFICIENT_AUTHENTICATION			0x05
#define ATT_ERR_REQUEST_NOT_SUPPORTED				0x06
#define ATT_ERR_INVALID_OFFSET						0x07
#define ATT_ERR_INSUFFICIENT_AUTHORIZATION			0x08
#define ATT_ERR_PREPARE_QUEUE_FULL					0x09
#define ATT_ERR_ATTRIBUTE_NOT_FOUND					0x0A
#define ATT_ERR_ATTRIBUTE_NOT_LONG					0x0B
#define ATT_ERR_INSUFFICIENT_ENCRYPTION_KEYSIZE		0x0C
#define ATT_ERR_INVALID_ATTRIBUTE_VALUE_LEN			0x0D
#define ATT_ERR_UNLIKELY_ERROR						0x0E
#define ATT_ERR_INSUFFICIENT_ENCRYPTION				0x0F
#define ATT_ERR_UNSUPPORTED_GROUP_TYPE				0x10
#define ATT_ERR_INSUFFICIENT_RESOURCES				0x11

#define ATT_OPCODE_ERROR						0x01
#define ATT_OPCODE_MTU_EXCHANGE_REQ				0x02
#define ATT_OPCODE_MTU_EXCHANGE_RSP				0x03
#define ATT_OPCODE_FIND_INFO_REQ				0x04
#define ATT_OPCODE_FIND_INFO_RSP				0x05
#define ATT_OPCODE_FIND_BY_TYPE_VALUE_REQ		0x06
#define ATT_OPCODE_FIND_BY_TYPE_VALUE_RSP		0x07
#define ATT_OPCODE_READ_BY_TYPE_REQ				0x08
#define ATT_OPCODE_READ_BY_TYPE_RSP				0x09
#define ATT_OPCODE_READ_REQ						0x0A
#define ATT_OPCODE_READ_RSP						0x0B
#define ATT_OPCODE_READ_BLOB_REQ				0x0C
#define ATT_OPCODE_READ_BLOB_RSP				0x0D
#define ATT_OPCODE_READ_MULTIPLE_REQ			0x0E
#define ATT_OPCODE_READ_MULTIPLE_RSP			0x0F
#define ATT_OPCODE_READ_BY_GROUP_TYPE_REQ		0x10
#define ATT_OPCODE_READ_BY_GROUP_TYPE_RSP		0x11
#define ATT_OPCODE_WRITE_REQ					0x12
#define ATT_OPCODE_WRITE_RSP					0x13
#define ATT_OPCODE_WRITE_CMD					0x52
#define ATT_OPCODE_SIGNED_WRITE_CMD				0xD2
#define ATT_OPCODE_PREPARE_WRITE_REQ			0x16
#define ATT_OPCODE_PREPARE_WRITE_RSP			0x17
#define ATT_OPCODE_EXECUTE_WRITE_REQ			0x18
#define ATT_OPCODE_EXECUTE_WRITE_RSP			0x19
#define ATT_OPCODE_NOTIFY						0x1B
#define ATT_OPCODE_INDICATION					0x1D
#define ATT_OPCODE_CONFIRMATION					0x1E

#define ATT_GROUP_TYPE_PRIMARY_SERVICE			0x2800
#define ATT_GROUP_TYPE_SECOND_SERVICE			0x2801

#define ATT_INVALID_HANDLE						0x0000
#define ATT_HANDLE_LEN							0x02
#define UUID_FORMAT_16BIT						0x01
#define UUID_FORMAT_128BIT						0x02

#define ATT_ERR_RSP_LEN							0x05
#define ATT_EXCHANGE_MTU_RSP_LEN				0x03

#define ATT_FIND_INFO_REQ_LEN					(4)     /* 去除opcode后的长度 */
#define ATT_READ_BY_GROUP_TYPE_REQ_SHORT		(6)     /* 去除opcode后的长度 */
#define ATT_READ_BY_GROUP_TYPE_REQ_LONG			(20)    /* 去除opcode后的长度 */
#define ATT_READ_BY_TYPE_REQ_SHORT              (6)     /* 去除opcode后的长度 */
#define ATT_READ_BY_TYPE_REQ_LONG               (20)    /* 去除opcode后的长度 */
#define ATT_READ_REQ_LEN                        (2)     /* 去除opcode后的长度 */
static u1 CheckGroupType( u1 *pu1Uuid, u1 uuidType )
{
    u1	pu1PrimarySrvUuid[] = {0x00, 0x28};
    u1	pu1SecondSSrvUuid[] = {0x01, 0x28};

    if( UUID_TYPE_16BIT != uuidType )
    {
        return 0;
    }
    /* 目前group type只支持 0x2800 */
    if( memcmp( pu1Uuid, pu1PrimarySrvUuid, uuidType ) != 0 )
    {
        return 0;
    }

    return 1;
}

static u1 GetUuidFormat( u1 uuidType )
{
    if( UUID_TYPE_128BIT == uuidType )
    {
        return UUID_FORMAT_128BIT;
    }
    return UUID_FORMAT_16BIT;
}

/**
 *@brief: 		AttErrRsp
 *@details:		错误响应
 *@param[in]	reqOpcode		导致错误的请求
 *@param[in]	attHandle		导致错误的att句柄
 *@param[in]	errCode			错误代码

 *@retval:		封包后的长度
 */
__INLINE static u4 AttErrRsp( u1 reqOpcode, u2 attHandle, u1 errCode )
{
    u1	pu1Rsp[ATT_ERR_RSP_LEN];
    u1 index = 0;

    pu1Rsp[index++] = ATT_OPCODE_ERROR;
    pu1Rsp[index++] = reqOpcode;
    pu1Rsp[index++] = attHandle & 0xff;
    pu1Rsp[index++] = attHandle >> 8;
    pu1Rsp[index++] = errCode;

    if( BleL2capDataSend( BLE_L2CAP_ATT_CHANNEL_ID, pu1Rsp, index ) != DH_SUCCESS )
    {
        return ERR_ATT_SEND_RSP_FAILED;
    }

    return DH_SUCCESS;
}

/**
 *@brief: 		AttMtuRsp
 *@details:		exchange mtu rsp
 *@param[in]	MTU
 
 *@retval:		DU_SUCCESS
 */
__INLINE static u4 AttMtuRsp( u2 MTU )
{
    u1 index = 0;
    u1 pu1Rsp[ATT_EXCHANGE_MTU_RSP_LEN];

    pu1Rsp[index++] = ATT_OPCODE_MTU_EXCHANGE_RSP;
    pu1Rsp[index++] = MTU & 0xff;
    pu1Rsp[index++] = MTU >> 8;

    if( BleL2capDataSend( BLE_L2CAP_ATT_CHANNEL_ID, pu1Rsp, index ) != DH_SUCCESS )
    {
        return ERR_ATT_SEND_RSP_FAILED;
    }
    return DH_SUCCESS;
}

/**
 *@brief: 		AttReadByGroupTypeRsp
 *@details:		read by group type rsp
 *@param[in]	eachAttDataLen  响应中的每个 属性句柄-EndGroup句柄-属性值 对的长度
 *@param[in]    pu1AttList      包含一个或多个AttHandle EndGroupHandle AttValue 
 *@param[in]	len             pu1AttList的数据长度

 *@retval:		DH_SUCCESS
 */
__INLINE static u4 AttReadByGroupTypeRsp(u1 eachAttDataLen, u1 *pu1AttList, u2 len )
{
    u2	index = 0;
    u1	pu1Rsp[BLE_ATT_MTU_SIZE];

    if( len > ( BLE_ATT_MTU_SIZE - 2 ) || NULL == pu1AttList )
    {
        return ERR_ATT_INVALID_PARAMS;
    }
    pu1Rsp[index++] = ATT_OPCODE_READ_BY_GROUP_TYPE_RSP;
    pu1Rsp[index++] = eachAttDataLen;
    memcpy( pu1Rsp + index, pu1AttList, len );
    index += len;

    if( BleL2capDataSend( BLE_L2CAP_ATT_CHANNEL_ID, pu1Rsp, index ) != DH_SUCCESS )
    {
        return ERR_ATT_SEND_RSP_FAILED;
    }
    return DH_SUCCESS;
}

/**
 *@brief: 		AttReadByTypeRsp
 *@details:		read by type rsp
 *@param[in]	eachAttDataLen  响应中的每个 属性句柄-属性值 键值对的长度
 *@param[in]    pu1AttList      包含一个或多个AttHandle(2B)-AttValue 键值对
 *@param[in]	len             pu1AttList的数据长度

 *@retval:		DH_SUCCESS
 */
__INLINE static u4 AttReadByTypeRsp(u1 eachAttDataLen, u1 *pu1AttList, u2 len )
{
    u2	index = 0;
    u1	pu1Rsp[BLE_ATT_MTU_SIZE];

    if( len > ( BLE_ATT_MTU_SIZE - 2 ) || NULL == pu1AttList )
    {
        return ERR_ATT_INVALID_PARAMS;
    }
    pu1Rsp[index++] = ATT_OPCODE_READ_BY_TYPE_RSP;
    pu1Rsp[index++] = eachAttDataLen;
    memcpy( pu1Rsp + index, pu1AttList, len );
    index += len;

    if( BleL2capDataSend( BLE_L2CAP_ATT_CHANNEL_ID, pu1Rsp, index ) != DH_SUCCESS )
    {
        return ERR_ATT_SEND_RSP_FAILED;
    }
    return DH_SUCCESS;
}

/**
 *@brief: 		AttFindInfoRsp
 *@details:		find information response
 *@param[in]	format          uuid format
 *@param[in]	pu1InfoRsp      一个或多个 AttHandle-UUID 键值对
 *@param[in]	len             pu1InfoRsp的长度

 *@retval:		DH_SUCCESS
 */
__INLINE static u4 AttFindInfoRsp( u1 format, u1 *pu1InfoRsp, u2 len )
{
    u2  index = 0;
    u1	pu1Rsp[BLE_ATT_MTU_SIZE];

    if( len > ( BLE_ATT_MTU_SIZE - 2 ) || NULL == pu1InfoRsp )
    {
        return ERR_ATT_INVALID_PARAMS;
    }
    pu1Rsp[index++] = ATT_OPCODE_FIND_INFO_RSP;
    pu1Rsp[index++] = format;
    memcpy( pu1Rsp + index, pu1InfoRsp, len );
    index += len;

    if( BleL2capDataSend( BLE_L2CAP_ATT_CHANNEL_ID, pu1Rsp, index ) != DH_SUCCESS )
    {
        return ERR_ATT_SEND_RSP_FAILED;
    }
    return DH_SUCCESS;
}

__INLINE static u4 AttReadRsp(u1 *pu1AttValue, u2 len)
{
    u2  index = 0;
    u1  pu1Rsp[BLE_ATT_MTU_SIZE];

    if( NULL==pu1AttValue || (len+1)>BLE_ATT_MTU_SIZE )
    {
        return ERR_ATT_INVALID_PARAMS;
    }

    pu1Rsp[index++] = ATT_OPCODE_READ_RSP;
    memcpy(pu1Rsp+index, pu1AttValue, len);
    index += len;
    
    if( BleL2capDataSend( BLE_L2CAP_ATT_CHANNEL_ID, pu1Rsp, index ) != DH_SUCCESS )
    {
        return ERR_ATT_SEND_RSP_FAILED;
    }
    return DH_SUCCESS;
}

__INLINE static u4 AttWriteRsp(void)
{
    u1  opcode;

    opcode = ATT_OPCODE_WRITE_RSP;
    if( BleL2capDataSend( BLE_L2CAP_ATT_CHANNEL_ID, &opcode, 1 ) != DH_SUCCESS )
    {
        return ERR_ATT_SEND_RSP_FAILED;
    }
    return DH_SUCCESS;
}

/**
 *@brief: 		AttFindInfoReqHandle
 *@details:		处理 find information请求
 *@param[in]	pu1Req  请求数据，去除了opcode
 *@param[in]	len     请求数据长度

 *@retval:		DH_SUCCESS
 */
static u4 AttFindInfoReqHandle( u1  *pu1Req, u2 len )
{
    BlkBleAttribute *pblkAtt;
    u2	u2StartHandle = ATT_INVALID_HANDLE;
    u2	u2EndHandle = ATT_INVALID_HANDLE;
    u2	u2AttHandle = ATT_INVALID_HANDLE;
    u1	index = 0;
    u4	ret;
    u1	uuidFormat;
    u1	pu1Rsp[BLE_ATT_MTU_SIZE - 2];   /* 去掉opcode和formate所占字节 */
    u1	rspLen = 0;

    if( NULL == pu1Req )
    {
        return ERR_ATT_INVALID_PARAMS;
    }
    u2StartHandle = pu1Req[index++];
    u2StartHandle += ( pu1Req[index++] << 8 ) & 0xff00;
    u2EndHandle = pu1Req[index++];
    u2EndHandle += ( pu1Req[index++] << 8 ) & 0xff00;
    if ( ATT_FIND_INFO_REQ_LEN != len )
    {
        ret = AttErrRsp( ATT_OPCODE_FIND_INFO_REQ, u2StartHandle, ATT_ERR_INVALID_PDU );
        return ret;
    }

    if( ATT_INVALID_HANDLE == u2StartHandle || u2StartHandle > u2EndHandle )
    {
        ret = AttErrRsp( ATT_OPCODE_FIND_INFO_REQ, u2StartHandle, ATT_ERR_INVALID_HANDLE );
        return ret;
    }

    for( u2AttHandle = u2StartHandle; u2AttHandle <= u2EndHandle; u2AttHandle++ )
    {
        BleGattFindAttByHandle( u2AttHandle, &pblkAtt );
        if ( NULL != pblkAtt )
        {   
            /* 第一个att,后续的att uuid必须跟第一个长度一样才能放到响应里 */
            if( u2AttHandle == u2StartHandle )
            {
                uuidFormat = GetUuidFormat( pblkAtt->attType.uuidType );
            }
          
            /* find information 的响应中只能放同一类型的uuid */
            if( GetUuidFormat( pblkAtt->attType.uuidType ) != uuidFormat || ( ( rspLen + pblkAtt->attType.uuidType + ATT_HANDLE_LEN ) > sizeof( pu1Rsp ) ) )
            {
                return AttFindInfoRsp( uuidFormat, pu1Rsp, rspLen );
            }
            
            pu1Rsp[rspLen++] = u2AttHandle & 0xff;
            pu1Rsp[rspLen++] = ( u2AttHandle >> 8 ) & 0xff;
            memcpy( pu1Rsp + rspLen, pblkAtt->attType.pu1Uuid; pblkAtt->attType.uuidType );
            /* uuid的类型实际值就是UUID的长度 */
            rspLen += pblkAtt->attType.uuidType;
        }
        else	/* pblk==NULL */
        {   
            /* 一个都没找到 */
            if ( u2AttHandle == u2StartHandle )
            {
                return AttErrRsp( ATT_OPCODE_FIND_INFO_REQ, u2AttHandle, ATT_ERR_ATTRIBUTE_NOT_FOUND );
            }
            /* 当前的句柄已经找不到att了，那么后续的肯定也找不到了，直接返回已经找到的 */
            return AttFindInfoRsp( uuidFormat, pu1Rsp, rspLen );
        }
    }

    return DH_SUCCESS;
}

/**
 *@brief: 		AttReadByGroupTypeReqHandle
 *@details:		处理 read by group type request
 *@param[in]	pu1Req  请求数据，去除了opcode
 *@param[in]	len     请求数据长度     

 *@retval:		DH_SUCCESS
 */
static u4 AttReadByGroupTypeReqHandle( u1 *pu1Req, u2 len )
{
    BlkBleAttribute *pblkAtt;
    u1	index = 0;
    u1	rspLen = 0;
    u1  eachAttDataLen = 0;
    u1	pu1Rsp[BLE_ATT_MTU_SIZE - 2]; /* 去掉opcode和length所占字节 */
    u1	pu1AttValue[BLE_ATT_MTU_SIZE - 2 - 4]; /* Attribute Data List 响应格式为AttHandle(2B) EndGroupHandle(2B) AttValue */
    u1  u1AttValueLen = 0;
    u2	u2StartHandle = ATT_INVALID_HANDLE;
    u2	u2EndHandle = ATT_INVALID_HANDLE;
    u2	u2AttHandle = ATT_INVALID_HANDLE;
    u2	u2GroupEndHandle = ATT_INVALID_HANDLE;
    u2  u2GroupStartHandle = ATT_INVALID_HANDLE;
    u1	uuidType = 0;
    u1	pu1GroupType[UUID_TYPE_128BIT];

    if( NULL == pu1Req )
    {
        return ERR_ATT_INVALID_PARAMS;
    }
    u2StartHandle = pu1Req[index++];
    u2StartHandle += ( pu1Req[index++] << 8 ) & 0xff00;
    u2EndHandle = pu1Req[index++];
    u2EndHandle += ( pu1Req[index++] << 8 ) & 0xff00;

    if( ATT_READ_BY_GROUP_TYPE_REQ_SHORT != len && ATT_READ_BY_GROUP_TYPE_REQ_LONG != len )
    {
        return AttErrRsp( ATT_OPCODE_READ_BY_GROUP_TYPE_REQ, u2StartHandle, ATT_ERR_INVALID_PDU );
    }
    if( ATT_INVALID_HANDLE == u2StartHandle || u2StartHandle > u2EndHandle )
    {
        return AttErrRsp( ATT_OPCODE_READ_BY_GROUP_TYPE_REQ, u2StartHandle, ATT_ERR_INVALID_HANDLE )
    }
    if( ATT_READ_BY_GROUP_TYPE_REQ_SHORT == len )
    {
        uuidType = UUID_TYPE_16BIT;
    }
    else
    {
        uuidType = UUID_TYPE_128BIT;
    }
    memcpy( pu1GroupType, pu1Req + index, uuidType );
    if( !CheckGroupType( pu1GroupType, uuidType ) )
    {
        return AttErrRsp( ATT_OPCODE_READ_BY_GROUP_TYPE_REQ, u2StartHandle, ATT_ERR_UNSUPPORTED_GROUP_TYPE );
    }
    for( u2AttHandle = u2StartHandle; u2AttHandle < u2EndHandle; u2AttHandle++ )
    {
        BleGattFindAttByHandle( u2AttHandle, *pblkAtt );
        if( NULL != pblkAtt )
        {
            if( pblkAtt->attType.uuidType == uuidType && memcmp(pblkAtt->attType.pu1Uuid, pu1GroupType, uuidType) == 0 )
            {
                if( ATT_INVALID_HANDLE != u2GroupStartHandle )  // 说明找到的不是第一个服务声明
                {
                    /* 找到了后续服务声明，则前面的都属于上一个服务,先保存前一个服务的信息 */
                    u2GroupEndHandle = u2AttHandle - 1;         // 前面一个att属于上一个服务的最后一个att
                    pu1Rsp[rspLen++] = u2GroupStartHandle & 0xff;
                    pu1Rsp[rspLen++] = ( u2GroupStartHandle >> 8 ) & 0xff;
                    pu1Rsp[rspLen++] = u2GroupEndHandle & 0xff;
                    pu1Rsp[rspLen++] = ( u2GroupEndHandle >> 8 ) & 0xff;
                    memcpy( pu1Rsp + rspLen, pu1AttValue, u1AttValueLen );
                    rspLen += u1AttValueLen;
                    /* Attribute Data List 响应格式为AttHandle(2B) EndGroupHandle(2B) AttValue */
                    if( ( rspLen + pblkAtt->attValue.u2CurrentLen + 4 ) > sizeof( pu1Rsp ) || (pblkAtt->attValue.u2CurrentLen+4)!=eachAttDataLen )
                    {
                        /* 数据已经放不下了，或者格式和前面已经保存的不一样，返回当前已经找到的 */
                        return AttReadByGroupTypeRsp(eachAttDataLen, pu1Rsp, rspLen );
                    }
                }
                u2GroupStartHandle = u2AttHandle;
                /* read group 支持查找的是服务声明，其值为UUID 最大16字节而已 */
                if( pblkAtt->attValue.u2CurrentLen < sizeof( pu1AttValue ) )
                {
                    memcpy( pu1AttValue, pblkAtt->attValue.pu1AttValue, pblkAtt->attValue.u2CurrentLen );
                    u1AttValueLen = pblkAtt->attValue.u2CurrentLen;
                }
                else
                {
                    u1AttValueLen = 0;
                }
                eachAttDataLen = 4+pblkAtt->attValue.u2CurrentLen;
            }
        }
        else    /* 已经没有属性了 */
        {
            if( rspLen > 0 )
            {
                return AttReadByGroupTypeRsp(eachAttDataLen, pu1Rsp, rspLen );
            }
            else
            {   
                /* 一个都没找到 */
                return AttErrRsp(ATT_OPCODE_READ_BY_GROUP_TYPE_REQ, u2StartHandle, ATT_ERR_ATTRIBUTE_NOT_FOUND);
            }
        }
    }
    return DH_SUCCESS;
}

/**
 *@brief: 		AttReadByTypeReqHandle
 *@details:		处理 read by type request
 *@param[in]	pu1Req  请求数据，去除了opcode
 *@param[in]	len     请求数据长度      

 *@retval:		DH_SUCCESS
 */
static u4 AttReadByTypeReqHandle( u1 *pu1Req, u2 len )
{
    BlkBleAttribute *pblkAtt;
    u1	index = 0;
    u1	rspLen = 0;
    u1  eachAttDataLen = 0;
    u1	pu1Rsp[BLE_ATT_MTU_SIZE - 2];           // 去掉opcode和length所占字节
    u1	pu1AttValue[BLE_ATT_MTU_SIZE - 2 - 2];  // Attribute Data List 响应格式为AttHandle AttValue
    u1  u1AttValueLen = 0;
    u2	u2StartHandle = ATT_INVALID_HANDLE;
    u2	u2EndHandle = ATT_INVALID_HANDLE;
    u2	u2AttHandle = ATT_INVALID_HANDLE;
    u1	uuidType = 0;
    u1	pu1FindType[UUID_TYPE_128BIT];

    if( NULL == pu1Req )
    {
        return ERR_ATT_INVALID_PARAMS;
    }
    u2StartHandle = pu1Req[index++];
    u2StartHandle += ( pu1Req[index++] << 8 ) & 0xff00;
    u2EndHandle = pu1Req[index++];
    u2EndHandle += ( pu1Req[index++] << 8 ) & 0xff00;
    if( ATT_READ_BY_TYPE_REQ_SHORT != len && ATT_READ_BY_TYPE_REQ_LONG != len )
    {
        return AttErrRsp( ATT_OPCODE_READ_BY_TYPE_REQ, u2StartHandle, ATT_ERR_INVALID_PDU );
    }
    if( ATT_INVALID_HANDLE == u2StartHandle || u2StartHandle > u2EndHandle )
    {
        return AttErrRsp( ATT_OPCODE_READ_BY_TYPE_REQ, u2StartHandle, ATT_ERR_INVALID_HANDLE )
    }
    if( ATT_READ_BY_TYPE_REQ_SHORT == len )
    {
        uuidType = UUID_TYPE_16BIT;
    }
    else
    {
        uuidType = UUID_TYPE_128BIT;
    }
    memcpy( pu1FindType, pu1Req + index, uuidType );
    for( u2AttHandle = u2StartHandle; u2AttHandle < u2EndHandle; u2AttHandle++ )
    {
        BleGattFindAttByHandle( u2AttHandle, *pblkAtt );
        if( NULL != pblkAtt )
        {
            if( pblkAtt->attType.uuidType == uuidType && memcmp(pblkAtt->attType.pu1Uuid, pu1FindType, uuidType) == 0 )
            {
                    /* 这是第一个找到的att，后续找到的att的 value长度需要和第一个保持一致 */
                if( 0 == eachAttDataLen )
                {
                    /* AttHandle(2B) + AttValue */
                    eachAttDataLen = 2+pblkAtt->attValue.u2CurrentLen;  
                }
                    /* Attribute Data List 响应格式为AttHandle(2B)      AttValue */
                if( ( rspLen + pblkAtt->attValue.u2CurrentLen + 2 ) > sizeof( pu1Rsp ) || (pblkAtt->attValue.u2CurrentLen+2)!=eachAttDataLen )
                {
                    /* 数据已经放不下了，或者格式和前面已经保存的不一样，返回当前已经找到的 */
                    return AttReadByTypeRsp(eachAttDataLen, pu1Rsp, rspLen );
                }
                pu1Rsp[rspLen++] = u2AttHandle&0xff;
                pu1Rsp[rspLen++] = (u2AttHandle>>8)&0xff;
                memcpy(pu1Rsp+rspLen, pblkAtt->attValue.pu1AttValue, pblkAtt->attValue.u2CurrentLen);
                rspLen += pblkAtt->attValue.u2CurrentLen;
            }
        }
        else    /* 已经没有属性了 */
        {
            if( rspLen > 0 )
            {
                return AttReadByGroupTypeRsp(eachAttDataLen, pu1Rsp, rspLen );
            }
            else
            {   
                /* 一个都没找到 */
                return AttErrRsp(ATT_OPCODE_READ_BY_TYPE_REQ, u2StartHandle, ATT_ERR_ATTRIBUTE_NOT_FOUND);
            }
        }
    }
    return DH_SUCCESS;
}


static u4 AttReadReqHandle(u1 *pu1Req, u2 len)
{
    BlkBleAttribute *pblkAtt;
    u1  pu1AttValue[BLE_ATT_MTU_SIZE-1];    // 去掉opcode所占字节
    u2  u2AttValueLen = ATT_INVALID_HANDLE;
    u2  u2AttHandle = ATT_INVALID_HANDLE;
    u1  index = 0;
    if( NULL == pu1Req )
    {
        return ERR_ATT_INVALID_PARAMS;
    }

    u2AttHandle = pu1Req[index++];
    u2AttHandle += (pu1Req[index++]<<8)&0xFF00;
    if( ATT_READ_REQ_LEN != len )
    {
        return AttErrRsp( ATT_OPCODE_READ_REQ, u2AttHandle, ATT_ERR_INVALID_PDU);
    }
    
    BleGattFindAttByHandle(u2AttHandle, &pblkAtt);
    if( NULL == pblkAtt )
    {
        AttErrRsp(ATT_OPCODE_READ_REQ, u2AttHandle, ATT_ERR_INVALID_HANDLE);
    }
    /* 只返回ATT_MTU-1 长度的内容，如果有剩余内容，client可以通过Read Blob Request 获取 */
    u2AttValueLen = (pblkAtt->attValue.u2CurrentLen>sizeof(pu1AttValue))?sizeof(pu1AttValue):pblkAtt->attValue.u2CurrentLen;
    memcpy(pu1AttValue, pblkAtt->attValue.pu1AttValue, u2AttValueLen);

    return AttReadRsp(pu1AttValue, u2AttValueLen);
}

static u4 AttWriteCommandHandle(u1 *pu1Req, u2 len)
{
    BlkBleAttribute *pblkAtt = NULL;
    u2  u2AttHandle = ATT_INVALID_HANDLE;
    u2  u2ValueLen;
    u1  index = 0;

    if( NULL==pu1Req )
    {
        return ERR_ATT_INVALID_PARAMS;
    }
    u2ValueLen = len-2;         // 减去handle所占字节
        /* opcode handle 占3字节 */
    if( u2ValueLen > (BLE_ATT_MTU_SIZE-3) )
    {
        /* 忽略该命令，直接返回 */
        return DH_SUCCESS;
    }
    u2AttHandle = pu1Req[index++]&0xff;
    u2AttHandle += (pu1Req[index++]<<8)&0xff00;

    BleGattFindAttByHandle(u2AttHandle, &pblkAtt);
    if( NULL != pblkAtt )
    {
        /* valueLen > attMaxlen，则写请求直接忽略*/
        if( u2ValueLen <= pblkAtt->attValue.u2MaxSize )
        {
            memcpy(pblkAtt->attValue.pu1AttValue, pu1Req+index, u2ValueLen);
            pblkAtt->attValue.u2CurrentLen = u2ValueLen;
        }                                                                               
    }
    return DH_SUCCESS;
}

static u4 AttWriteReqHandle(u1* pu1Req, u2 len)
{
    BlkBleAttribute *pblkAtt = NULL;
    u2  u2AttHandle = ATT_INVALID_HANDLE;
    u2  u2ValueLen;
    u1  index = 0;

    if( NULL==pu1Req )
    {
        return ERR_ATT_INVALID_PARAMS;
    }
    u2ValueLen = len-2;         // 减去handle所占字节
        /* opcode handle 占3字节 */
    if( u2ValueLen > (BLE_ATT_MTU_SIZE-3) )
    {
        /* 忽略该命令，直接返回 */
        return DH_SUCCESS;
    }
    u2AttHandle = pu1Req[index++]&0xff;
    u2AttHandle += (pu1Req[index++]<<8)&0xff00;

    BleGattFindAttByHandle(u2AttHandle, &pblkAtt);
    if( NULL != pblkAtt )
    {
        if( u2ValueLen <= pblkAtt->attValue.u2MaxSize )
        {
            memcpy(pblkAtt->attValue.pu1AttValue, pu1Req+index, u2ValueLen);
            pblkAtt->attValue.u2CurrentLen = u2ValueLen;
        }
        else
        {
            return AttErrRsp(ATT_OPCODE_WRITE_REQ, u2AttHandle, ATT_ERR_INVALID_ATTRIBUTE_VALUE_LEN);
        }
    }
    else
    {
        return AttErrRsp(ATT_OPCODE_WRITE_REQ, u2AttHandle, ATT_ERR_INVALID_HANDLE);
    }
    return AttWriteRsp();
}
/**
 *@brief: 		BleAttHandle
 *@details:		ATT协议请求数据处理，目前不支持带认证签名
 *@param[in]	pu1Data		协议数据
 *@param[in]	len			数据长度
 *@retval:		DH_SUCCESS
 */
u4 BleAttHandle( u1 *pu1Data, u2 len )
{
    u1	opcode;

    opcode = pu1Data[0];
}